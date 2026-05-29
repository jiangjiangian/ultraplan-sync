// 詛咒之傘的「污染值」機制：撿取詛咒傘不再一次扣 karma，而是遞增
// Player::cursedTaint_；之後每進入一個章節（SceneRouter Ch2/3/4 入口）
// 由 ApplyCursedTaintDecay 依 -5 * taint 緩慢扣 karma。因此第一次撿傘
// 在剩餘章節各扣 -5（乾淨的 Ch1 撿取在 Ch2/3/4 入口合計 ≤ -15）；第二次
// 撿取把扣率提高到每次轉場 -10、第三次 -15。Flag_TookCursedUmbrella 結局
// 標記仍在撿取當下設定，保留 Ending B 的前置條件。
//
// 本檔測試：撿取使污染值遞增而 karma 當下不變；ApplyCursedTaintDecay 依
// 污染值扣 karma 且 taint=0 時為空操作；污染值在持傘旗標清除／章節重置時
// 仍保留；扣減受 -100 karma 地板裁切。

#include "doctest/doctest.h"
#include "game/entities/CursedUmbrella.h"
#include "game/entities/Player.h"
#include "engine/events/EventBus.h"
#include "engine/math/Vec2.h"
#include "game/quest/Flags.h"

using nccu::engine::math::Vec2;

// 撿取詛咒傘會遞增污染值，但撿取當下 karma 不變；重複撿取具冪等性。
TEST_CASE("P2: cursed pickup increments taint, leaves karma untouched at pickup") {
    EventBus::Instance().Clear();

    Player p{Vec2{0.0f, 0.0f}};
    const int k0 = p.GetKarma();
    CHECK(k0 == 50);                                  // 企劃設定的起始值
    CHECK(p.GetCursedTaint() == 0);
    CHECK_FALSE(p.HasFlag(nccu::kFlagTookCursedUmbrella));

    CursedUmbrella cursed{Vec2{0.0f, 0.0f}};
    cursed.BeClaimed(&p);
    CHECK(p.HasFlag(nccu::kFlagTookCursedUmbrella));  // Ending B 的關鍵旗標仍設定
    CHECK(p.HasUmbrella());                            // 背包出現詛咒傘
    CHECK(p.GetKarma() == k0);                        // 撿取本身對 karma 中性
    CHECK(p.GetCursedTaint() == 1);                   // 計數遞增

    cursed.BeClaimed(&p);                             // 冪等保護（isActive_）
    CHECK(p.GetCursedTaint() == 1);                   // 不會重複加成
    CHECK(p.GetKarma() == k0);

    EventBus::Instance().Clear();
}

// ApplyCursedTaintDecay 依 -5 * 污染值扣 karma；taint=0 時為空操作。
TEST_CASE("P2: ApplyCursedTaintDecay bleeds -5 * taint, no-op at taint=0") {
    EventBus::Instance().Clear();

    Player p{Vec2{0.0f, 0.0f}};
    const int k0 = p.GetKarma();
    p.ApplyCursedTaintDecay();
    CHECK(p.GetKarma() == k0);                        // taint=0 → 不變

    p.IncCursedTaint();                               // 模擬一次撿取
    CHECK(p.GetCursedTaint() == 1);
    p.ApplyCursedTaintDecay();
    CHECK(p.GetKarma() == k0 - 5);                    // taint=1 → -5

    p.IncCursedTaint();                               // 第二次撿取
    CHECK(p.GetCursedTaint() == 2);
    p.ApplyCursedTaintDecay();
    CHECK(p.GetKarma() == k0 - 5 - 10);               // taint=2 → -10

    p.IncCursedTaint();                               // 第三次撿取
    p.ApplyCursedTaintDecay();
    CHECK(p.GetKarma() == k0 - 5 - 10 - 15);          // taint=3 → -15

    EventBus::Instance().Clear();
}

// 污染值在 SetHasUmbrella(false) 與章節重置後仍保留。
TEST_CASE("P2: taint persists through SetHasUmbrella(false) / chapter resets") {
    EventBus::Instance().Clear();

    Player p{Vec2{0.0f, 0.0f}};
    CursedUmbrella cursed{Vec2{0.0f, 0.0f}};
    cursed.BeClaimed(&p);
    CHECK(p.GetCursedTaint() == 1);

    // SceneRouter Ch2/3/4 入口會呼叫 SetHasUmbrella(false) 清空背包的傘格。
    // 污染值必須保留——它是詛咒「道德污點永久存在」的那一半，與永不清除的
    // Flag_TookCursedUmbrella 成對。
    p.SetHasUmbrella(false);
    CHECK(p.GetCursedTaint() == 1);
    CHECK(p.HasFlag(nccu::kFlagTookCursedUmbrella));

    EventBus::Instance().Clear();
}

// 扣減受 karma 地板值 -100 裁切。
TEST_CASE("P2: decay clamps at the karma floor of -100") {
    EventBus::Instance().Clear();

    Player p{Vec2{0.0f, 0.0f}};
    p.AddKarma(-150);                                 // 50 - 150 → 裁切為 -100
    CHECK(p.GetKarma() == -100);
    p.IncCursedTaint();
    p.ApplyCursedTaintDecay();                        // 未裁切會是 -105
    CHECK(p.GetKarma() == -100);                      // 裁切於地板值

    EventBus::Instance().Clear();
}
