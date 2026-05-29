#include "doctest/doctest.h"
#include "game/entities/DlcSign.h"
#include "engine/core/GameObject.h"
#include "engine/events/EventBus.h"
#include "game/entities/Player.h"
#include "game/world/World.h"
#include "game/state/SemesterState.h"
#include "engine/math/Vec2.h"

#include <string>

/**
 * @file test_dlc_sign.cpp
 * @brief 驗證 DlcSign（風雩走廊的「?」彩蛋告示牌）：互動可重複觸發且不會被消耗、
 *        不帶任何玩法效果、扮演的角色（IInteractable+IDrawable），以及只在
 *        Chapter4_Finals 生成、離開該章即被清除。
 */

namespace {
int CountDlcSigns(const nccu::World& w) {
    int n = 0;
    for (const auto& o : w.Objects())
        if (dynamic_cast<const DlcSign*>(o.get()) != nullptr) ++n;
    return n;
}
}  // namespace

// 風雩走廊的 DLC 彩蛋「?」告示牌。與 CashPickup / QuestFlagPickup 不同，它
// 可重複閱讀：Interact 會發出預告 ShowMessage，但不得停用，玩家能再次閱讀。
// 它不帶任何玩法效果（無 flag／karma／money），且透過一般的 IInteractable
// 角色分派（NpcId() 為空、IsVendor() 為 false），因此 GameController 的 E
// 互動掃描會走 AsInteractable()->Interact() 分支，而非 NPC 對話／Vendor 路徑。

namespace {

// 訂閱 ShowMessage 並記錄命中次數與最後文字。
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

// Interact 會發出預告訊息，且不會被消耗（維持啟用）。
TEST_CASE("DlcSign Interact: publishes the teaser and is NOT consumed") {
    MessageCapture cap;
    cap.Attach();
    Player p{nccu::engine::math::Vec2{0, 0}};

    DlcSign sign{nccu::engine::math::Vec2{1305.0f, 88.0f}};
    CHECK(sign.IsActive());

    sign.Interact(&p);
    CHECK(cap.hits == 1);
    // 完整的預告文字，含字面換行。MessageView 會在換行處斷行並將每列置中，
    // 因此顯示為兩列整齊置中的字：
    //   DLC開發中
    //   敬請期待
    CHECK(cap.lastText == "DLC開發中\n敬請期待");
    CHECK(cap.lastText.find('\n') != std::string::npos);   // 換行字元

    // 可重複閱讀：互動後仍維持啟用（若是拾取物此時已停用）。玩家狀態不受影響。
    CHECK(sign.IsActive());
    CHECK(p.GetKarma()  == 50);          // 建構預設值，未變
    CHECK(p.GetMoney()  == 100);         // 建構預設值，未變
    CHECK_FALSE(p.HasUmbrella());

    // 再次互動會再觸發（可重複，非一次性）。
    sign.Interact(&p);
    CHECK(cap.hits == 2);
    CHECK(sign.IsActive());

    EventBus::Instance().Clear();
}

// 角色與身分：扮演 IInteractable+IDrawable，但不是 NPC／Vendor。
TEST_CASE("DlcSign roles + identity: IInteractable+IDrawable, no NPC/Vendor") {
    DlcSign sign{nccu::engine::math::Vec2{1305.0f, 88.0f}};
    GameObject& asObj = sign;   // 以異質容器的視角操作

    // 它正好扮演 interact + draw 兩種角色（E 互動掃描透過 AsInteractable 找到它，
    // View 透過 AsDrawable 繪製）。
    CHECK(asObj.AsInteractable() != nullptr);
    CHECK(asObj.AsDrawable()     != nullptr);
    CHECK(asObj.AsUpdatable()    == nullptr);   // 不會逐幀更新

    // 不是對話對象／商店／任務給予者／牆，因此 controller 將它導向一般的
    // AsInteractable 分支（與 CashPickup / QuestFlagPickup 同路徑），
    // 不會走 NPC 或 Vendor 分支。
    CHECK(asObj.NpcId().empty());
    CHECK_FALSE(asObj.IsVendor());
    CHECK_FALSE(asObj.IsQuestGiver());
    CHECK_FALSE(asObj.BlocksMovement());
}

// 透過 GameObject& 分派 Interact（重現掃描路徑）。
TEST_CASE("DlcSign dispatches its Interact through a GameObject& (sweep path)") {
    MessageCapture cap;
    cap.Attach();
    Player p{nccu::engine::math::Vec2{0, 0}};

    DlcSign sign{nccu::engine::math::Vec2{1305.0f, 88.0f}};
    GameObject& asObj = sign;
    // 重現 controller 的 E 互動分派：將非 NPC 物件導向
    // AsInteractable()->Interact()，與 GameController 的做法一致。
    if (auto* it = asObj.AsInteractable()) it->Interact(&p);

    CHECK(cap.hits == 1);
    CHECK(cap.lastText == "DLC開發中\n敬請期待");   // 兩列置中文字
    CHECK(asObj.IsActive());             // 掃描觸碰後仍存在
    EventBus::Instance().Clear();
}

// 只在 Chapter4_Finals 生成，離開該章時被清除。
TEST_CASE("DlcSign spawns ONLY in Chapter4_Finals and is swept on exit") {
    EventBus::Instance().Clear();
    nccu::World w("", /*loadSprites=*/false);

    // Ch1 不存在（這個開放探索的告示牌屬於最終章）。
    CHECK(CountDlcSigns(w) == 0);

    // 隨 Chapter4_Finals 進場（由名冊追蹤，因此恰好一個）。
    w.Semester().Transition(nccu::SemesterState::Chapter4_Finals);
    w.RespawnChapterRoster(nccu::SemesterState::Chapter4_Finals);
    CHECK(CountDlcSigns(w) == 1);

    // Ch4 結束即被清除——它和其他章節物件一樣由名冊追蹤，因此離開 Ch4 就移除
    //（不會殘留到結局）。
    w.Semester().Transition(nccu::SemesterState::Ending_C);
    w.RespawnChapterRoster(nccu::SemesterState::Ending_C);
    CHECK(CountDlcSigns(w) == 0);

    EventBus::Instance().Clear();
}
