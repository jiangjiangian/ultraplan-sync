#ifndef VENDOR_SPRITE_H_
#define VENDOR_SPRITE_H_
#include "game/quest/PipoyaRoster.h"
#include "engine/math/Vec2.h"
#include <cstddef>
#include <string>

namespace nccu {

/**
 * @file VendorSprite.h
 * @brief 攤販 sprite 的純選圖規則：每個市集攤位都對應到不同的人。
 *
 * 這是 World::SpawnChapterNpcs 為攤販挑選 sprite 時使用的唯一純規則，抽出後
 * 既走正式產線、又可直接做單元測試（可驗證回歸），不需 GL 環境或選用的 PIPOYA
 * 美術資源包。
 *
 *   - PickNpcSprite 的「鍵」是攤位自己的攤主（若無則用其名稱）——這些字串各自相異，
 *     故各攤位的雜湊不同，不會像舊版那樣全部撞在字面值 "vendor" 上（那會讓 PIPOYA
 *     路徑為十個攤位挑出同一張 sprite）。
 *   - 退路（PIPOYA 資源包缺席時——即乾淨 clone／評分重建——由 PickNpcSprite 原樣
 *     回傳）是依生成索引挑選、外觀彼此分明的精選 sprite，在實際隨附的 sprite
 *     （school_uniform_3 與三個 npc 角色，不新增美術）間循環取用。舊版對每個
 *     攤位都用同一張 shop_auntie.png，故乾淨 clone 會畫出十個一模一樣的分身——正是
 *     回報的缺陷。
 */
inline const char* const kVendorFallbackSprites[] = {
    "resources/assets/sprites/npc/shop_auntie.png",
    "resources/assets/sprites/npc/suit_senior.png",
    "resources/assets/sprites/npc/ta.png",
    "resources/assets/sprites/school_uniform_3/female_04.png",
    "resources/assets/sprites/school_uniform_3/male_04.png",
    "resources/assets/sprites/school_uniform_3/female_07.png",
    "resources/assets/sprites/school_uniform_3/male_07.png",
    "resources/assets/sprites/school_uniform_3/female_10.png",
    "resources/assets/sprites/school_uniform_3/male_10.png",
    "resources/assets/sprites/school_uniform_3/female_13.png",
};
inline constexpr std::size_t kVendorFallbackCount =
    sizeof(kVendorFallbackSprites) / sizeof(kVendorFallbackSprites[0]);

/**
 * @brief 每個攤位的 sprite 選圖鍵（依建構方式保證各攤位唯一）。
 * @param stallKeeper 攤主名稱（可為空）。
 * @param name        攤位名稱（攤主為空時改用之）。
 * @return 形如 "vendor:<攤主或攤位名>" 的選圖鍵。
 */
inline std::string VendorSpriteKey(const std::string& stallKeeper,
                                   const std::string& name) {
    return "vendor:" + (stallKeeper.empty() ? name : stallKeeper);
}

/**
 * @brief 取得生成索引為 `index` 的攤位所用的 sprite，保證十攤各不相同。
 * @param index       攤位的生成索引（唯一的選圖依據）。
 * @param stallKeeper 攤主名稱（保留以維持簽名；現以索引選圖，故未使用）。
 * @param name        攤位名稱（同上，保留簽名用）。
 * @param pos         攤位的世界座標位置（同上）。
 * @return sprite 資源路徑。
 *
 * 以「生成索引」逐攤挑相異 sprite——不論有無 PIPOYA 素材包。有素材包：索引進其清單；
 * 無素材包：索引進精選後備清單（兩者皆 >= 10 張，足供十攤）。刻意不用 PickNpcSprite
 * 的 (key, pos) 雜湊：當攤位數接近 roster 大小時雜湊會碰撞（生日悖論），讓有素材包的
 * 環境冒出重複攤販。索引在兩份清單內皆是單射，故各攤穩定且相異。
 */
inline std::string VendorSpriteFor(std::size_t index,
                                   [[maybe_unused]] const std::string& stallKeeper,
                                   [[maybe_unused]] const std::string& name,
                                   [[maybe_unused]] nccu::engine::math::Vec2 pos) {
    const auto& roster = PipoyaRoster();
    if (!roster.empty())
        return roster[index % roster.size()];
    return kVendorFallbackSprites[index % kVendorFallbackCount];
}

} // namespace nccu

#endif // VENDOR_SPRITE_H_
