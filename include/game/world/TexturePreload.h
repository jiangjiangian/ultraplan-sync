#ifndef GFX_TEXTURE_PRELOAD_H_
#define GFX_TEXTURE_PRELOAD_H_
#include "engine/render/Texture.h"
#include "game/gfx/Decorations.h"          // nccu::game::gfx::kDecorations — ambient strips
#include "game/entities/Personas.h"   // kPersonas — player + preview sheets
#include "game/world/Buildings.h"          // kAll — building art
#include "game/world/Obstacles.h"          // kBuildingCollisionSkip
#include "game/vendor/VendorSprite.h"      // kVendorFallbackSprites
#include "game/quest/PipoyaRoster.h"       // PipoyaRoster() — varied NPC art
#include <algorithm>
#include <array>
#include <string>
#include <string_view>

/**
 * @file TexturePreload.h
 * @brief 在進入遊戲前預熱貼圖快取，消除開場時讀檔與 GPU 上傳造成的單幀卡頓。
 *
 * 在玩家進入遊戲前，以 World／View 即將請求的貼圖預熱行程貼圖快取（engine/render/
 * Texture.h），使 World 建構（實體 LoadSprite）與 View 建構（底圖、建築、裝飾條）命
 * 中已預熱的快取，而非在開場當下才逐檔讀盤並上傳 GPU。重新開始亦受益：重新進入會建
 * 立全新的 World／View，但快取已預熱，不會再次卡頓。
 *
 * 這是純粹的 gfx 關注點：只讀取靜態美術表（建築矩形、角色圖、裝飾條、sprite 名冊）以
 * 得知哪些「檔案」存在，從不觸及任何遊戲／任務「狀態」，因此維持 MVC 乾淨。其路徑與
 * View.cpp／World.cpp／CharacterSelect.cpp 傳給 Texture::Load 的完全一致，且取自同一
 * 份正規表，使清單不致默默走樣。
 *
 * 檔案缺失時每次預熱都是廉價的快取未命中（全新 clone／無頭測試環境的 resources/ 為
 * 空），因此在無頭路徑上呼叫幾乎零成本；至於執行期間顯示的載入畫面，則由呼叫端
 * （main.cpp）僅在真人路徑開啟（無頭流程會略過標題／選角畫面以維持輸出一致）。
 */
namespace nccu::game::world {

/// @brief 世界底圖貼圖路徑（最大的單一上傳）；與 View 傳給 Texture::Load 的路徑一致。
inline constexpr std::string_view kWorldmapBasePath =
    "resources/assets/maps/worldmap_base.png";

// 在任務生成表與角色／攤販名冊中硬編、但不在 kPersonas／kVendorFallbackSprites 內的
// 精選劇情 NPC／攤販圖；列於此使預熱集合在全新 clone 上仍完整（PIPOYA 美術包存在時由
// PipoyaRoster() 補上其餘）。不新增檔案：這些都已隨附或被引用，缺檔僅是廉價未命中。
inline constexpr std::array<std::string_view, 16> kCuratedSprites{{
    "resources/assets/sprites/npc/shop_auntie.png",
    "resources/assets/sprites/npc/suit_senior.png",
    "resources/assets/sprites/npc/ta.png",
    "resources/assets/sprites/school_uniform_3/male_01.png",
    "resources/assets/sprites/school_uniform_3/male_02.png",
    "resources/assets/sprites/school_uniform_3/male_03.png",
    "resources/assets/sprites/school_uniform_3/male_04.png",
    "resources/assets/sprites/school_uniform_3/male_05.png",
    "resources/assets/sprites/school_uniform_3/female_01.png",
    "resources/assets/sprites/school_uniform_3/female_02.png",
    "resources/assets/sprites/school_uniform_3/female_03.png",
    "resources/assets/sprites/school_uniform_3/female_04.png",
    "resources/assets/sprites/school_uniform_3/female_05.png",
    "resources/assets/sprites/school_uniform_3/female_06.png",
    "resources/assets/sprites/school_uniform_3/female_07.png",
    "resources/assets/sprites/school_uniform_3/female_10.png",
}};

/**
 * @brief 依建築名稱組出其美術貼圖路徑，與 View 的組法一致。
 * @param name 建築名稱。
 * @return "resources/assets/buildings_3d_trimmed/<name>.png"。
 */
inline std::string BuildingTexturePath(std::string_view name) {
    std::string p = "resources/assets/buildings_3d_trimmed/";
    p += std::string(name);
    p += ".png";
    return p;
}

/**
 * @brief 以 World／View 即將請求的全部貼圖預熱快取。
 *
 * 冪等（每個路徑至多上傳一次）且在全新 clone 上廉價（未命中為無操作）。可於啟動時呼
 * 叫一次；之後的重新開始只會再次命中已預熱的快取。必須在 InitWindow 之後呼叫（需 GPU
 * 存取）——呼叫端以視窗存在與否守衛，與字型載入相同。
 */
inline void PreloadGameTextures() {
    // 1. 世界底圖（單一大貼圖）。
    nccu::engine::render::PreloadTexture(std::string(kWorldmapBasePath));

    // 2. 建築美術——比照 View 的略過集合與路徑組法。
    const auto& skip = obstacles::kBuildingCollisionSkip;
    for (const auto& b : buildings::kAll) {
        if (std::find(skip.begin(), skip.end(), b.name) != skip.end())
            continue;                       // 開放地面已烘焙進底圖
        nccu::engine::render::PreloadTexture(BuildingTexturePath(b.name));
    }

    // 3. 環境裝飾條（純裝飾）——與 View 載入的定義相同。
    for (const auto& d : nccu::game::gfx::kDecorations)
        nccu::engine::render::PreloadTexture(std::string(d.stripPath));

    // 4. 五張角色圖（玩家 sprite 與選角預覽）。
    for (const auto& p : kPersonas)
        nccu::engine::render::PreloadTexture(std::string(p.spritePath));

    // 5. 攤販後備名冊（全新 clone 時的攤位 sprite）。
    for (std::size_t i = 0; i < kVendorFallbackCount; ++i)
        nccu::engine::render::PreloadTexture(std::string(kVendorFallbackSprites[i]));

    // 6. 精選劇情 NPC／額外制服圖。
    for (const auto& s : kCuratedSprites)
        nccu::engine::render::PreloadTexture(std::string(s));

    // 7. 多樣 NPC 名冊（PIPOYA 包）——以目錄列舉，故全新 clone 上為空（此迴圈為無操
    // 作），可選包存在時則預熱整批群眾。
    for (const auto& s : PipoyaRoster())
        nccu::engine::render::PreloadTexture(s);
}

} // namespace nccu::game::world

#endif // GFX_TEXTURE_PRELOAD_H_
