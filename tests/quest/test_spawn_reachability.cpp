#include "doctest/doctest.h"
#include "world/CollisionMask.h"
#include "quest/NpcSpawns.h"
#include "quest/ChapterSpawns.h"
#include "quest/ChapterQuestItems.h"
#include "quest/ChapterPickups.h"
#include "quest/ChapterVendors.h"
#include "state/SemesterState.h"
#include <array>
#include <queue>
#include <string>
#include <vector>

#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

// Defense-in-depth guard for the collision-mask integration. A baked
// terrain mask that seals the campus perimeter, or a spawn coordinate
// dropped onto a Collision-layer prop, makes the game silently
// unplayable while every other test stays green (the 2026-05-16 bug:
// the southern perimeter wall had its only gap ~100 px east of the 正門
// art, and 學霸 sat on a planter polygon). This test loads the REAL
// shipped mask and asserts, with a flood-fill from the player spawn,
// that every quest-critical entity is actually walkable AND reachable.
//
// It degrades gracefully: a fresh checkout without the untracked
// resources/ assets gets an empty mask (everything walkable) — the
// guard then skips rather than failing, since it can only assert what
// it can load.

using nccu::CollisionMask;

namespace {

constexpr float kBox = 24.0f;  // mirrors world::kPlayerWidth/Height

struct Spot { const char* name; float x; float y; };

// Player / umbrella / quest-pickup spawns MUST mirror the literals in
// src/World.cpp::World() — keep this list in sync if those move. The
// archetype NPCs and ambient pedestrians are pulled live from
// DefaultNpcSpawns() / AmbientStudentSpawns() so a future spawn move is
// covered with zero duplication. Ambient pedestrians are included
// because a wanderer that *starts* embedded in a wall reads in-game as
// "someone stuck in the wall" even though it never blocks progress.
std::vector<Spot> GameplaySpots() {
    std::vector<Spot> s = {
        {"player",            500.0f, 1860.0f},
        {"TrueUmbrella",      320.0f, 1280.0f},
        {"FragileUmbrella",   750.0f, 1280.0f},
        {"ProfTrapUmbrella", 1200.0f, 1256.0f},
        {"CursedUmbrella",   1560.0f, 1280.0f},
        {"QuestForm",         560.0f, 1725.0f},
    };
    for (const auto& n : nccu::DefaultNpcSpawns())
        s.push_back(Spot{n.npcId, n.pos.x, n.pos.y});
    for (const auto& n : nccu::AmbientStudentSpawns())
        s.push_back(Spot{n.spritePath, n.pos.x, n.pos.y});

    // S5b–S5e chapter spawns — these were NEVER reachability-validated
    // (the playtest found objects in walls/trees). Cover every coord a
    // chapter actually spawns: per-chapter NPC rosters (the new
    // librarian / 香腸 / 大聲公 / 學姊 + the re-positioned archetypes),
    // the Ch2 散落筆記, the parsed Interlude stalls + the hardcoded
    // Ch2 自販機 / Ch4 集英樓 Vendors, and the Ch3/Ch4 道具箱
    // TrueUmbrella (World.cpp inline spawn at {1500,1430}).
    using nccu::SemesterState;
    for (auto st : {SemesterState::Chapter2_Midterms,
                    SemesterState::Chapter3_SportsDay,
                    SemesterState::Chapter4_Finals}) {
        for (const auto& n : nccu::ChapterNpcSpawns(st))
            s.push_back(Spot{n.npcId, n.pos.x, n.pos.y});
    }
    for (const auto& q : nccu::ChapterQuestItems(SemesterState::Chapter2_Midterms))
        s.push_back(Spot{"Ch2note", q.pos.x, q.pos.y});
    for (const auto& pk : nccu::ChapterPickups(SemesterState::Chapter1_AddDrop))
        s.push_back(Spot{"Ch1cash", pk.pos.x, pk.pos.y});

    nccu::SetVendorContentDir(TEST_CONTENT_DIR);
    nccu::ReloadVendors();
    for (auto st : {SemesterState::Interlude_Market,
                    SemesterState::Chapter2_Midterms,
                    SemesterState::Chapter4_Finals}) {
        for (const auto& v : nccu::ChapterVendors(st))
            s.push_back(Spot{"vendor", v.pos.x, v.pos.y});
    }
    s.push_back(Spot{"Ch3/Ch4 道具箱 TrueUmbrella", 1640.0f, 375.0f});
    return s;
}

} // namespace

TEST_CASE("every gameplay spawn is walkable and reachable from the player") {
    const CollisionMask mask = nccu::LoadTerrainMask();
    if (mask.Empty()) {
        MESSAGE("terrain mask asset absent (untracked resources/) — "
                "reachability guard skipped");
        return;
    }

    const auto spots = GameplaySpots();

    // 1. No entity may spawn embedded in solid terrain.
    for (const auto& s : spots) {
        INFO("spawn '" << std::string(s.name) << "' at (" << s.x << ", "
             << s.y << ") is inside solid terrain");
        CHECK_FALSE(mask.BlockedBox(s.x, s.y, kBox, kBox));
    }

    // 2. Flood-fill the walkable space from the player spawn on an 8 px
    //    lattice; every other spot must be within one cell of it.
    constexpr int kStep = 8;
    const int gw = mask.Width()  / kStep;
    const int gh = mask.Height() / kStep;
    auto freeCell = [&](int cx, int cy) {
        const float x = static_cast<float>(cx * kStep);
        const float y = static_cast<float>(cy * kStep);
        if (cx < 0 || cy < 0 || cx >= gw || cy >= gh) return false;
        return !mask.BlockedBox(x, y, kBox, kBox);
    };

    const int sx = 500 / kStep, sy = 1860 / kStep;
    std::vector<char> seen(static_cast<std::size_t>(gw) * gh, 0);
    std::queue<std::pair<int,int>> q;
    if (freeCell(sx, sy)) {
        seen[static_cast<std::size_t>(sy) * gw + sx] = 1;
        q.push({sx, sy});
    }
    const std::array<std::pair<int,int>,4> dirs = {{{1,0},{-1,0},{0,1},{0,-1}}};
    while (!q.empty()) {
        auto [cx, cy] = q.front(); q.pop();
        for (auto [dx, dy] : dirs) {
            const int nx = cx + dx, ny = cy + dy;
            if (nx < 0 || ny < 0 || nx >= gw || ny >= gh) continue;
            auto& cell = seen[static_cast<std::size_t>(ny) * gw + nx];
            if (cell || !freeCell(nx, ny)) continue;
            cell = 1;
            q.push({nx, ny});
        }
    }

    auto reachable = [&](float fx, float fy) {
        const int bx = static_cast<int>(fx) / kStep;
        const int by = static_cast<int>(fy) / kStep;
        for (int dx = -3; dx <= 3; ++dx)
            for (int dy = -3; dy <= 3; ++dy) {
                const int cx = bx + dx, cy = by + dy;
                if (cx >= 0 && cy >= 0 && cx < gw && cy < gh &&
                    seen[static_cast<std::size_t>(cy) * gw + cx])
                    return true;
            }
        return false;
    };

    for (const auto& s : spots) {
        INFO("spawn '" << std::string(s.name) << "' at (" << s.x << ", "
             << s.y << ") is unreachable from the player spawn (campus sealed?)");
        CHECK(reachable(s.x, s.y));
    }
}
