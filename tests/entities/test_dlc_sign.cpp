#include "doctest/doctest.h"
#include "game/entities/DlcSign.h"
#include "engine/core/GameObject.h"
#include "engine/events/EventBus.h"
#include "game/entities/Player.h"
#include "game/world/World.h"
#include "game/state/SemesterState.h"
#include "engine/math/Vec2.h"

#include <string>

namespace {
int CountDlcSigns(const nccu::World& w) {
    int n = 0;
    for (const auto& o : w.Objects())
        if (dynamic_cast<const DlcSign*>(o.get()) != nullptr) ++n;
    return n;
}
}  // namespace

// B2 regression — the 風雩走廊 DLC easter-egg "?" sign. Unlike the
// CashPickup / QuestFlagPickup it is RE-READABLE: Interact publishes the
// teaser ShowMessage but must NOT deactivate, so a player can read it again.
// It carries no gameplay effect (no flag / karma / money) and dispatches
// through the generic IInteractable role (its NpcId() is empty, IsVendor()
// false), so the GameController E-interact sweep routes it down the
// `AsInteractable()->Interact()` branch — never the NPC-dialog / Vendor path.

namespace {

struct MessageCapture {
    int         hits = 0;
    std::string lastText;
    void Attach() {
        EventBus::Instance().Clear();
        EventBus::Instance().Subscribe(EventType::ShowMessage,
            [this](const Event& e) { hits++; lastText = e.text; });
    }
};

} // namespace

TEST_CASE("DlcSign Interact: publishes the teaser and is NOT consumed") {
    MessageCapture cap;
    cap.Attach();
    Player p{nccu::engine::math::Vec2{0, 0}};

    DlcSign sign{nccu::engine::math::Vec2{1305.0f, 88.0f}};
    CHECK(sign.IsActive());

    sign.Interact(&p);
    CHECK(cap.hits == 1);
    // UI-B-3: the exact teaser, with the literal '\n'. MessageView wraps at
    // the '\n' and CENTRES each row, so it shows as two tidy centred lines
    //   DLC開發中
    //   敬請期待
    // (the 【…】 brackets / … ellipsis were dropped so the two lines balance
    // when centred). 敬 is baked into the atlas (test_font_ui_literal_scan).
    CHECK(cap.lastText == "DLC開發中\n敬請期待");
    CHECK(cap.lastText.find('\n') != std::string::npos);   // the line break

    // RE-READABLE: still active after the interact (a pickup would have
    // flipped isActive_ here). No gameplay state touched on the player.
    CHECK(sign.IsActive());
    CHECK(p.GetKarma()  == 50);          // ctor default — unchanged
    CHECK(p.GetMoney()  == 100);         // ctor default — unchanged
    CHECK_FALSE(p.HasUmbrella());

    // Re-interacting fires again (re-readable, not one-shot).
    sign.Interact(&p);
    CHECK(cap.hits == 2);
    CHECK(sign.IsActive());

    EventBus::Instance().Clear();
}

TEST_CASE("DlcSign roles + identity: IInteractable+IDrawable, no NPC/Vendor") {
    DlcSign sign{nccu::engine::math::Vec2{1305.0f, 88.0f}};
    GameObject& asObj = sign;   // exercise the heterogeneous-container view

    // It plays exactly the interact + draw roles (E-interact sweep finds it
    // via AsInteractable; the View paints it via AsDrawable).
    CHECK(asObj.AsInteractable() != nullptr);
    CHECK(asObj.AsDrawable()     != nullptr);
    CHECK(asObj.AsUpdatable()    == nullptr);   // it never ticks

    // NOT a talk target / not a shop / not a quest-giver / not a wall, so the
    // controller routes it to the generic AsInteractable branch (the same
    // path CashPickup / QuestFlagPickup use), never the NPC or Vendor branch.
    CHECK(asObj.NpcId().empty());
    CHECK_FALSE(asObj.IsVendor());
    CHECK_FALSE(asObj.IsQuestGiver());
    CHECK_FALSE(asObj.BlocksMovement());
}

TEST_CASE("DlcSign dispatches its Interact through a GameObject& (sweep path)") {
    MessageCapture cap;
    cap.Attach();
    Player p{nccu::engine::math::Vec2{0, 0}};

    DlcSign sign{nccu::engine::math::Vec2{1305.0f, 88.0f}};
    GameObject& asObj = sign;
    // Mirror the controller's E-interact dispatch: route a non-NPC object
    // through AsInteractable()->Interact(), exactly as GameController does.
    if (auto* it = asObj.AsInteractable()) it->Interact(&p);

    CHECK(cap.hits == 1);
    CHECK(cap.lastText == "DLC開發中\n敬請期待");   // UI-B-3 two centred lines
    CHECK(asObj.IsActive());             // still there after the sweep touch
    EventBus::Instance().Clear();
}

TEST_CASE("DlcSign spawns ONLY in Chapter4_Finals and is swept on exit") {
    EventBus::Instance().Clear();
    nccu::World w("", /*loadSprites=*/false);

    // Not present in Ch1 (the open-explore sign belongs to the finale).
    CHECK(CountDlcSigns(w) == 0);

    // Enters with Chapter4_Finals (roster-tracked, so exactly one).
    w.Semester().Transition(nccu::SemesterState::Chapter4_Finals);
    w.RespawnChapterRoster(nccu::SemesterState::Chapter4_Finals);
    CHECK(CountDlcSigns(w) == 1);

    // Swept the moment Ch4 ends — it is roster-tracked like every other
    // chapter object, so leaving Ch4 removes it (no leak into an ending).
    w.Semester().Transition(nccu::SemesterState::Ending_C);
    w.RespawnChapterRoster(nccu::SemesterState::Ending_C);
    CHECK(CountDlcSigns(w) == 0);

    EventBus::Instance().Clear();
}
