#ifndef VIEW_H_
#define VIEW_H_
#include "engine/render/RaylibRenderer.h"
#include "engine/render/Camera2D.h"
#include "engine/render/Texture.h"
#include "engine/math/Vec2.h"
#include "engine/math/Rect.h"
#include "ui/ChapterCard.h"
#include "game/state/SemesterState.h"
#include <cstddef>
#include <optional>
#include <vector>

/**
 * @file View.h
 * @brief 渲染層（MVC 的 View）：持有具體 renderer、跟隨相機與世界地圖貼圖，
 *        把唯讀的 World 模型轉成像素。
 */

class GameObject; // 全域命名空間的模型物件，僅透過 IRenderer 繪製

namespace nccu {

class World; // 交給 Draw() 的唯讀模型

/**
 * @brief 渲染層：持有具體 renderer、跟隨相機與世界地圖貼圖。
 *
 * Draw() 是「遊戲狀態唯一被轉成像素」的地方：依模型擺好自己的相機、貼上地圖、
 * 透過 IRenderer 繪製每個存活物件，最後畫 HUD。全程以 const 讀取 World，絕不
 * 變更它（MVC 紅線——View 只讀模型、只渲染，不查詢也不寫入）。
 */
class View {
public:
    View(int windowWidth, int windowHeight);

    void Draw(const World& world);

private:
    /// @brief 一棟已擺放的建築：對應哪張已載入貼圖、落在世界座標何處，以及
    ///        用於深度排序的地面線（sprite 底緣）。
    struct BuildingSprite {
        std::size_t     texIndex;
        nccu::engine::math::Rect dest;
        float           baseY;
        bool            flipX;
        bool            flipY;
    };
    /// @brief 每幀繪製順序清單（依 `y` 做深度排序）中的一筆。`kind` 決定
    ///        payload：Object 時 `obj` 是該 GameObject；Building／Decoration
    ///        時 `index` 分別指向 buildings_／decorations_。裝飾物併入「同一
    ///        次排序」，使環境裝飾（如石像）能對 NPC／建築正確 walk-behind。
    enum class DrawKind { Object, Building, Decoration };
    struct DrawRef {
        float               y;
        DrawKind            kind;
        const ::GameObject* obj;     ///< 僅 kind == Object 時有效
        std::size_t         index;   ///< 其餘情況為 buildings_／decorations_ 索引
    };
    /// @brief 一條已載入的環境裝飾條：對應哪個 kDecorations[] 定義，以及它
    ///        載入的貼圖。在建構子建立；若某定義的 PNG 缺檔則直接跳過（不建
    ///        條目），故缺資產時這條什麼都不畫。這些純屬 View 側裝飾——絕非
    ///        GameObject、不進 World::Objects()，因此不影響自動跑流程的存檔。
    struct DecorationSprite {
        std::size_t        defIndex;   ///< 指向 nccu::game::gfx::kDecorations
        nccu::engine::render::Texture texture;    ///< 已載入的條圖（move-only）
    };

    /// @name 每幀渲染分段
    /// Draw() 為分派器；每個 helper 只負責「單一關注點」，把原本龐大的單一
    /// 方法拆成可審閱的片段。各 helper 直接存取 View 的成員狀態（camera_、
    /// 貼圖、裝飾時鐘、章節字卡等），不額外傳參。
    ///
    ///   UpdateChapterCardTransition — 偵測 SemesterState 邊界並武裝「傘又掉了
    ///     ／找到傘了」書檔字卡；每幀都跑（在結局提前返回之前），讓字卡狀態保
    ///     持最新。
    ///   RenderEnding — 結局畫面分支：把整張 framebuffer 清為不透明黑、組出
    ///     EndingSummary DTO 並呼叫 DrawEndingCard。僅在 FSM 處於結局狀態時呼
    ///     叫，呼叫端隨即返回。
    ///   RenderWorld — 在 CameraScope 內：世界地圖底圖、操場跑道地貼、繪製順序
    ///     掃描（建築＋裝飾＋存活物件）、任務給予者「!」浮標、過場出口標記。
    ///   RenderHud — 螢幕座標：操場圈速環、左上狀態面板（業力／金幣／雨量／章
    ///     節）、雨壓 vignette、任務目標面板。
    ///   RenderOverlays — HUD 訊息、對話框、物品欄、M 選單提示、暫停選單、說明
    ///     疊層、章節書檔字卡。順序很重要：章節字卡「最後」畫，使其疊在最上層。
    ///@{
    void UpdateChapterCardTransition(SemesterState st);
    void RenderEnding(const World& world, SemesterState st);
    void RenderWorld(const World& world, SemesterState st);
    void RenderHud(const World& world, SemesterState st);
    void RenderOverlays(const World& world);
    ///@}

    nccu::engine::render::RaylibRenderer        renderer_;
    nccu::engine::render::Camera2D              camera_;
    nccu::engine::render::Texture               worldmap_;
    nccu::engine::math::Vec2                  screenCenter_;
    nccu::engine::math::Vec2                  worldSize_;
    nccu::engine::math::Vec2                  viewportSize_;
    std::vector<nccu::engine::render::Texture>  buildingTextures_;
    std::vector<BuildingSprite>      buildings_;
    std::vector<DecorationSprite>    decorations_;  ///< 環境裝飾條（純裝飾）
    std::vector<DrawRef>             drawOrder_;  ///< 每幀暫存的繪製順序
    float                            endingAlpha_ = 0.0f;  ///< 結局字卡淡入進度
    /// 乒乓裝飾動畫的渲染時鐘（秒）：每幀繪製世界時累加 Time::DeltaSeconds()。
    /// 非模擬狀態（純 View 裝飾，與 interludeMarkerPhase_／endingAlpha_ 同性
    /// 質），故不影響自動跑流程的時間軸與存檔。
    double                           decorationClock_ = 0.0;
    /// 過場出口地面標記的動畫相位（世界像素）。玩家身處過場期間，每幀以
    /// DeltaSeconds × 速度累加，使虛線由西向東掃動。非模擬一部分——純 View 視
    /// 覺裝飾。
    float                            interludeMarkerPhase_ = 0.0f;
    /// 章節書檔大字卡。View 每幀比對 SemesterState（與 lastSemester_）的變化，
    /// 為「傘又掉了」（章節開始）／「找到傘了」（章節清關）節拍武裝 chapterCard_
    /// ——完全由 View 本就讀取的 FSM 驅動，故不新增事件／發布，存檔逐位元不變。
    /// 字卡計時由 Time::DeltaSeconds()（自動跑流程下為固定步長）推進，故具決定
    /// 性。lastSemester_ 在首次 Draw 前為 std::nullopt，使開場進入 Chapter1 時
    /// 能在第 0 幀觸發其開場字卡。
    std::optional<nccu::SemesterState> lastSemester_;
    ChapterCardState                   chapterCard_;
};

} // namespace nccu

#endif // VIEW_H_
