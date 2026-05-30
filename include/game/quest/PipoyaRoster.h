#ifndef PIPOYA_ROSTER_H_
#define PIPOYA_ROSTER_H_
#include "engine/math/Vec2.h"
#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

/**
 * @file PipoyaRoster.h
 * @brief 為每個 NPC／Vendor 挑選多樣的角色 sprite，讓校園人潮不再是十個一模一樣
 *        的複製人，且在素材缺席時自動退化為無作用。
 *
 * PIPOYA 32x32 素材包提供 Male 與 Female 兩組 sheet，每張皆為 96x128（3x4）排版，
 * 與手工挑選的 NPC 美術完全相同；NPC::LoadSprite 以同樣方式切片，因此任一張 sheet
 * 都能直接套用，無須改動任何渲染邏輯。
 *
 * 此素材包放在被 gitignore 的目錄下，全新 clone 或評分用的重建環境不會有它：當清單
 * 為空時，呼叫端原本手選的 spritePath 會原封不動回傳。換言之，有素材時純粹增添多樣
 * 性，沒素材時則為 no-op。挑選對 (npcId, position) 而言是確定性的，使同一個生成點在
 * 重生／跨幀之間維持穩定外觀，不同生成點則各異——與每個環境音效各自的 PRNG seed 同理。
 */
namespace nccu {

/**
 * @brief 掃描 PIPOYA 素材包並回傳排序後的可用 sprite 路徑清單（首次呼叫時建構並快取）。
 * @return 全部可用 sprite 路徑；素材包不存在時為空向量。
 *
 * 以函式區域 static 做惰性初始化，掃描只進行一次。最後對結果排序，確保跨次執行的順序
 * 穩定（目錄列舉順序本身不保證），讓確定性挑選的結果可重現。
 */
inline const std::vector<std::string>& PipoyaRoster() {
    static const std::vector<std::string> roster = [] {
        namespace fs = std::filesystem;
        std::vector<std::string> out;
        const char* base =
            "resources/assets/PIPOYA FREE RPG Character Sprites 32x32/";
        for (const char* sub : {"Male", "Female"}) {
            std::error_code ec;
            const fs::path dir = fs::path(base) / sub;
            if (!fs::is_directory(dir, ec)) continue;
            for (const auto& e : fs::directory_iterator(dir, ec)) {
                if (ec) break;
                if (e.path().extension() == ".png")
                    out.push_back(e.path().string());
            }
        }
        std::sort(out.begin(), out.end());   // 確保跨次執行的順序穩定
        return out;
    }();
    return roster;
}

/**
 * @brief 為某個生成點挑一張確定性的 sprite；素材包缺席時退回 fallbackPath。
 * @param npcId        NPC 識別字串，作為雜湊輸入之一。
 * @param pos          生成點世界座標，與 npcId 一起雜湊以區分同類不同位置的生成。
 * @param fallbackPath 素材包不存在時原樣回傳的後備路徑（呼叫端手選的 sprite）。
 * @return 確定性選出的 sprite 路徑，或在無素材時為 fallbackPath。
 *
 * 把 npcId 與整數化後的座標混入雜湊，使同一生成點固定對應同一張外觀，不同生成點則
 * 分散開來。素材包缺席（清單為空）時原樣回傳 fallbackPath，讓乾淨 checkout 仍正確。
 */
inline std::string PickNpcSprite(const std::string& npcId,
                                 nccu::engine::math::Vec2 pos,
                                 const std::string& fallbackPath) {
    const auto& roster = PipoyaRoster();
    if (roster.empty()) return fallbackPath;
    std::size_t h = std::hash<std::string>{}(npcId);
    h ^= std::hash<int>{}(static_cast<int>(pos.x)) + 0x9e3779b9 +
         (h << 6) + (h >> 2);
    h ^= std::hash<int>{}(static_cast<int>(pos.y)) + 0x9e3779b9 +
         (h << 6) + (h >> 2);
    return roster[h % roster.size()];
}

} // namespace nccu

#endif // PIPOYA_ROSTER_H_
