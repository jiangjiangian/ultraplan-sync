#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "game/quest/Chapter2Quest.h"
#include "engine/events/EventBus.h"
#include "game/entities/NPC.h"
#include "game/entities/Player.h"
#include "engine/core/GameObject.h"
#include "game/world/World.h"
#include "game/state/SemesterState.h"
#include "game/state/SemesterStateMachine.h"
#include "engine/math/Vec2.h"
#include <string_view>

using nccu::SemesterState;
using nccu::World;

/**
 * @file test_interlude_loaner_return.cpp
 * @brief 驗證「歸還管理員借傘」這個可選的責任感選項：僅限第二章往第三章的
 *        幕間、且持有借傘時，歸還給一次 +10 業力並清除借傘；範圍判定、
 *        冪等性、歸還點地標的生成，以及跳過歸還的安全性。
 */

// 僅在第二章→第三章的幕間、且玩家仍持有管理員借傘時，於中正圖書館前歸還
// 可獲得一次 +10 業力，並清除手上的傘與 Flag_LibrarianUmbrella，且顯示感謝詞。
// 它不會給予任何影響結局的傘旗標；即使跳過，借傘仍會在進第三章時自動清除
// （無業力）— 是個純正面、可選的責任感選擇。

namespace {

Player MakePlayer() { return Player{nccu::engine::math::Vec2{0.0f, 0.0f}}; }

// 讓玩家進入「持有第二章借傘」的狀態。
void GiveLoaner(Player& p) {
    p.SetHeldUmbrella(HeldUmbrella::Loaner);
    p.SetFlag(nccu::kFlagLibrarianUmbrella);
}

bool HasReturnMarker(const World& w) {
    for (const auto& o : w.Objects())
        if (o->NpcId() == std::string_view(nccu::kNpcLibrarianReturn))
            return true;
    return false;
}

} // namespace

// 歸還管理員的傘給一次 +10 業力並清除借傘；重複互動不再加分（冪等）。
TEST_CASE("歸還管理員的傘給一次 +10 業力並清除借傘（冪等）") {
    EventBus::Instance().Clear();
    Player p = MakePlayer();
    GiveLoaner(p);
    const int k0 = p.GetKarma();

    // +10 只在「會回到第三章」的幕間生效。
    nccu::TryReturnLibrarianUmbrella(EventBus::Instance(),
        p, nccu::kNpcLibrarianReturn, SemesterState::Interlude_Market,
        SemesterState::Chapter3_SportsDay);

    CHECK(p.GetKarma() == k0 + 10);
    CHECK(p.HeldUmbrellaKind() == HeldUmbrella::None);   // 借傘已交還
    CHECK_FALSE(p.HasUmbrella());
    CHECK_FALSE(p.HasFlag(nccu::kFlagLibrarianUmbrella));
    CHECK(p.HasFlag(nccu::kFlagLibrarianUmbrellaReturned));
    // 關鍵：這不是結局旗標 — 借傘永遠不會解鎖結局 A。
    CHECK_FALSE(p.HasFlag(nccu::kFlagHasTrueUmbrella));

    // 冪等：再次對話會重播一句收尾台詞，但不會再給第二次 +10。
    nccu::TryReturnLibrarianUmbrella(EventBus::Instance(),
        p, nccu::kNpcLibrarianReturn, SemesterState::Interlude_Market,
        SemesterState::Chapter3_SportsDay);
    CHECK(p.GetKarma() == k0 + 10);
    EventBus::Instance().Clear();
}

// 歸還只在第二章→第三章的市集、且對正確 NPC、持有借傘時才生效。
TEST_CASE("歸還只在 Ch2→Ch3 市集、對正確 NPC、持有借傘時才生效") {
    EventBus::Instance().Clear();

    // 回程目的地不對的市集（例如第一章→第二章，returnTo 是第二章）：
    // 即使在歸還點且持有借傘也不給分。
    {
        Player p = MakePlayer();
        GiveLoaner(p);
        nccu::TryReturnLibrarianUmbrella(EventBus::Instance(),
            p, nccu::kNpcLibrarianReturn, SemesterState::Interlude_Market,
            SemesterState::Chapter2_Midterms);
        CHECK(p.GetKarma() == 50);
        CHECK(p.HeldUmbrellaKind() == HeldUmbrella::Loaner);   // 仍持有
    }
    // 狀態不對（非幕間）：不給分。
    {
        Player p = MakePlayer();
        GiveLoaner(p);
        nccu::TryReturnLibrarianUmbrella(EventBus::Instance(),
            p, nccu::kNpcLibrarianReturn, SemesterState::Chapter2_Midterms,
            SemesterState::Chapter3_SportsDay);
        CHECK(p.GetKarma() == 50);
        CHECK(p.HeldUmbrellaKind() == HeldUmbrella::Loaner);
    }
    // NPC id 不對（同市集的另一個 NPC）：不給分。
    {
        Player p = MakePlayer();
        GiveLoaner(p);
        nccu::TryReturnLibrarianUmbrella(EventBus::Instance(),
            p, "shop_auntie", SemesterState::Interlude_Market,
            SemesterState::Chapter3_SportsDay);
        CHECK(p.GetKarma() == 50);
        CHECK(p.HeldUmbrellaKind() == HeldUmbrella::Loaner);
    }
    // 手上沒有借傘（空手）：不給分，也不會誤設旗標。
    {
        Player p = MakePlayer();
        nccu::TryReturnLibrarianUmbrella(EventBus::Instance(),
            p, nccu::kNpcLibrarianReturn, SemesterState::Interlude_Market,
            SemesterState::Chapter3_SportsDay);
        CHECK(p.GetKarma() == 50);
        CHECK_FALSE(p.HasFlag(nccu::kFlagLibrarianUmbrellaReturned));
    }
    EventBus::Instance().Clear();
}

// 歸還點地標只在第二章→第三章的市集、且持有借傘時生成，且僅生成一次。
TEST_CASE("歸還點地標只在 Ch2→Ch3 市集且持有借傘時生成，且僅一次") {
    World w("", /*loadSprites=*/false);

    // 第一章（無幕間）：無地標。
    CHECK_FALSE(HasReturnMarker(w));

    // 回到第二章的幕間（第一章→第二章市集）：此時尚不可能持有借傘，因此即使
    // 強制設定手持類型，目的地判定也會擋下生成。
    w.Semester().SetInterludeReturnTo(SemesterState::Chapter2_Midterms);
    w.Semester().Transition(SemesterState::Interlude_Market);
    w.RespawnChapterRoster(SemesterState::Interlude_Market);
    GiveLoaner(*w.GetPlayer());
    CHECK_FALSE(w.MaybeSpawnInterludeLibrarianReturn());
    CHECK_FALSE(HasReturnMarker(w));

    // 回到第三章的幕間（第二章→第三章市集）且持有借傘：地標出現，恰好一次。
    w.Semester().SetInterludeReturnTo(SemesterState::Chapter3_SportsDay);
    // 透過重新生成名冊（視為一次新的幕間進入）重新武裝這個一次性生成。
    w.RespawnChapterRoster(SemesterState::Interlude_Market);
    GiveLoaner(*w.GetPlayer());
    CHECK(w.MaybeSpawnInterludeLibrarianReturn());        // 生成
    CHECK(HasReturnMarker(w));
    CHECK_FALSE(w.MaybeSpawnInterludeLibrarianReturn());  // 一次性

    // 延後生成後，Player 仍維持其不變量（位於物件清單首位）。
    CHECK(w.Objects().front().get() ==
          static_cast<GameObject*>(w.GetPlayer()));
}

// 跳過歸還是安全的 — 借傘會在進入第三章時自動清除。
TEST_CASE("跳過歸還是安全的：借傘會在進入 Ch3 時自動清除") {
    // 地標只在持有借傘時出現；若玩家直接走過，SceneRouter 進第三章的重置仍會
    // 清空手持的傘（無業力）。此處固定「歸還純屬可選、絕非關卡門檻」：從不歸還
    // 的玩家照樣在進第三章時失去傘並繼續遊戲。
    World w("", /*loadSprites=*/false);
    w.Semester().SetInterludeReturnTo(SemesterState::Chapter3_SportsDay);
    w.Semester().Transition(SemesterState::Interlude_Market);
    GiveLoaner(*w.GetPlayer());

    // 未執行歸還。借傘在幕間中仍被持有。
    CHECK(w.GetPlayer()->HeldUmbrellaKind() == HeldUmbrella::Loaner);
    // （實際的進第三章清除是 SceneRouter 的 SetHasUmbrella(false)，由章節轉移
    // 測試涵蓋；此處只斷言「不歸還」不會留下業力欠債或已歸還旗標 — 即它是個
    // 無副作用的略過，而非卡關。）
    CHECK(w.GetPlayer()->GetKarma() == 50);
    CHECK_FALSE(w.GetPlayer()->HasFlag(nccu::kFlagLibrarianUmbrellaReturned));
}
