#ifndef GFX_TEXTURE_H_
#define GFX_TEXTURE_H_
#include "raylib.h"
#include <string>
#include <unordered_map>

namespace nccu::engine::render {

namespace detail { class TextureCache; }  // 每張快取材質的唯一擁有者

/**
 * @file Texture.h
 * @brief raylib GPU 材質的 move-only RAII 控制代碼，搭配程序級材質快取（載入一次、共享檢視）。
 */

/**
 * @brief raylib GPU 材質的 move-only RAII 控制代碼。
 *
 * 所有權模型：一個 Texture 不是擁有者（owns_==true：解構時 ::UnloadTexture 該 GPU
 * id），就是非擁有的檢視（owns_==false：解構為 no-op）。兩個「擁有」的 Texture 絕不
 * 可共享同一 ::Texture2D id，否則該 id 會被 Unload 兩次而崩潰——但任意數量的「檢視」
 * 可安全別名同一擁有者的 id，因為檢視永不 Unload。
 *
 * Load() 不再每次都打 GPU，而是經由程序生命週期的快取（detail::TextureCache）：
 * 對某路徑的首次 Load 自磁碟讀取並上傳一次、由快取「擁有」該材質；該次與之後對同檔
 * 的每次 Load 都回傳一個廉價的非擁有檢視。因此實體的 sprite_／View 的圖條都是檢視，
 * 其解構絕不觸碰共享 GPU id（無 double-free、無 use-after-free），而重開遊戲重建
 * World／View 時命中暖快取（無磁碟卡頓）。
 *
 * 快取比每個實體／View 都長壽（為 function-local static），並由 main.cpp 在 GL
 * context 仍存活時呼叫 ShutdownTextureCache() 明確拆除——與 Font.h 相同的紀律
 * （ShutdownFont 須早於 ~Window／CloseWindow）。
 *
 * 缺檔合約（不可更動）：對不存在／無效檔案呼叫 ::LoadTexture 會回傳 {0}（LoadImage
 * 資料為 NULL 時 raylib 回 Texture2D{0}），故快取存入一筆無效（id==0、owns==false）
 * 項目、Load() 回傳無效檢視——IsValid()==false、不畫任何東西、不崩潰。空資源／無頭
 * 自動遊玩路徑因而保持乾淨 no-op。
 */
class Texture {
public:
    /**
     * @brief 載入一次：命中快取回傳已上傳材質的非擁有檢視，未命中則上傳一次（快取保留擁有者）。
     * @param[in] path 材質檔路徑。
     * @return 指向快取材質的非擁有檢視（缺檔時為無效檢視）。
     */
    static Texture Load(const std::string& path);

    Texture(const Texture&)            = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(Texture&& o) noexcept : tex_(o.tex_), owns_(o.owns_) {
        o.tex_ = {};
        o.owns_ = false;
    }
    Texture& operator=(Texture&& o) noexcept {
        if (this != &o) {
            if (owns_) ::UnloadTexture(tex_);
            tex_ = o.tex_;
            owns_ = o.owns_;
            o.tex_ = {};
            o.owns_ = false;
        }
        return *this;
    }

    ~Texture() { if (owns_) ::UnloadTexture(tex_); }

    /** @brief 材質寬度（像素）。 */
    int  Width()   const noexcept { return tex_.width;  }
    /** @brief 材質高度（像素）。 */
    int  Height()  const noexcept { return tex_.height; }
    /** @brief 材質是否有效（id != 0）。@return 有效回傳 true。 */
    bool IsValid() const noexcept { return tex_.id != 0; }

    /** @brief 內部存取器：僅供渲染層的 Renderer::Texture 使用。@return 底層 raylib ::Texture2D。 */
    const ::Texture2D& Raw() const noexcept { return tex_; }

private:
    // owning==true 取得 t 的所有權（解構時 Unload）；owning==false 建立別名 t、
    // 但永不 Unload 的非擁有檢視。
    explicit Texture(::Texture2D t, bool owning) noexcept
        : tex_(t), owns_(owning) {}

    // 既有（快取擁有）材質的非擁有檢視。別名其 id／尺寸供繪製；解構為 no-op，
    // 故絕不會 double-free 共享 id。
    static Texture View(const ::Texture2D& t) noexcept {
        return Texture{t, /*owning=*/false};
    }

    ::Texture2D tex_{};
    bool        owns_{false};

    // 快取以 View() 建立非擁有檢視、以 (Texture2D,bool) 建構子建立擁有者，故需存取兩者。
    friend class detail::TextureCache;
};

namespace detail {

/**
 * @brief 程序生命週期的材質倉庫：以路徑為鍵，每筆值為該檔的唯一擁有者材質。
 *
 * 每個檔案至多上傳一次。為 function-local static，故在 InitWindow 之後（首次 Load）
 * 惰性建立；由於「static 物件在 main() 之後才解構」的風險，不倚賴它自我解構——而由
 * ShutdownTextureCache() 在 GL 仍存活時明確清空。move-only 的 Texture 值可正常存放
 * 於此（節點式 map）。
 */
class TextureCache {
public:
    /**
     * @brief 找出或載入路徑，回傳快取擁有者的非擁有檢視。
     * @param[in] path 材質檔路徑。
     * @return 指向快取材質的非擁有檢視。
     *
     * 缺檔會快取一個無效擁有者（{0}）並回傳無效檢視，使 no-op 合約成立，且不會每次
     * 都重新 stat 該檔。
     */
    Texture Acquire(const std::string& path) {
        auto it = store_.find(path);
        if (it == store_.end()) {
            // 由磁碟／GPU 載入建構唯一擁有者。owning 取自 id!=0，使缺檔的 {0}
            // 擁有者不會 Unload 任何東西。
            ::Texture2D raw = ::LoadTexture(path.c_str());
            it = store_.emplace(path,
                                Texture{raw, /*owning=*/(raw.id != 0)}).first;
        }
        return ViewOf(it->second);
    }

    /**
     * @brief 預載某路徑而不回傳控制代碼。
     * @param[in] path 材質檔路徑。
     *
     * 即一次丟棄檢視的 Acquire——擁有者留在 map 中。
     */
    void Warm(const std::string& path) { (void)Acquire(path); }

    /** @brief 卸載所有快取材質並清空 map；須在 GL context 仍存活時執行。 */
    void Clear() noexcept { store_.clear(); }

    /** @brief 目前快取的材質筆數。 */
    std::size_t Size() const noexcept { return store_.size(); }

private:
    static Texture ViewOf(const Texture& owner) noexcept {
        return Texture::View(owner.Raw());
    }
    std::unordered_map<std::string, Texture> store_;
};

inline TextureCache& Textures() {
    static TextureCache cache;
    return cache;
}

} // namespace detail

// ---- 程序材質快取的自由函式 --------------------------------------------

/**
 * @brief 預先暖快取一張材質（preload）。
 * @param[in] path 材質檔路徑。
 *
 * 可重複呼叫；同一路徑至多上傳一次。檔案不存在時為廉價 no-op（無頭／空資源）。
 */
inline void PreloadTexture(const std::string& path) {
    detail::Textures().Warm(path);
}

/**
 * @brief 拆除材質快取。
 *
 * 須於 main() 中、在 ~Window／::CloseWindow 之前（GL 仍存活時）呼叫——與
 * ShutdownFont 同一紀律。具冪等性（清空空 map 為 no-op），故從未預載的正常結束
 * 路徑亦無妨。
 */
inline void ShutdownTextureCache() noexcept { detail::Textures().Clear(); }

/** @brief 測試／診斷用：目前快取了多少個不同路徑。@return 快取筆數。 */
inline std::size_t TextureCacheSize() noexcept {
    return detail::Textures().Size();
}

inline Texture Texture::Load(const std::string& path) {
    return detail::Textures().Acquire(path);
}

} // namespace nccu::engine::render

#endif // GFX_TEXTURE_H_
