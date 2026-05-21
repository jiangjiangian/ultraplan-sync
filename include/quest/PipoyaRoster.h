#ifndef PIPOYA_ROSTER_H_
#define PIPOYA_ROSTER_H_
#include "gfx/Vec2.h"
#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

// Picks a varied character sprite per NPC/Vendor so the campus crowd is
// not ten identical clones (the playtest's "人物請多樣性"). The PIPOYA
// 32x32 pack ships Male/ + Female/ sheets, each a 96x128 (3x4) sheet
// exactly like the hand-curated NPC art — NPC::LoadSprite slices it the
// same way, so any sheet drops in with no render change.
//
// The pack lives under a gitignored directory, so a fresh clone / the
// grading rebuild will not have it: when the roster comes up empty the
// caller's existing hand-picked spritePath is returned unchanged, i.e.
// this is purely additive variety where the assets exist and a no-op
// where they do not. The pick is DETERMINISTIC per (npcId, position) so
// a given spawn keeps one stable look across re-spawns / frames while
// different spawns differ — same idea as the per-ambient PRNG seed.
namespace nccu {

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
        std::sort(out.begin(), out.end());   // stable order across runs
        return out;
    }();
    return roster;
}

// fallbackPath is returned verbatim when the PIPOYA pack is absent, so
// callers stay correct on a clean checkout.
inline std::string PickNpcSprite(const std::string& npcId,
                                 nccu::gfx::Vec2 pos,
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
