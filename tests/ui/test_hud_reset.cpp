/**
 * @file test_hud_reset.cpp
 * @brief 驗證 HudExpired()：當 HUD 提示存活時間超過 kHudTtl 時轉為已過期，
 *        讓 View 以外的消費者（輸出 state.jsonl 的自動遊玩工具）停止回報過時的
 *        提示；同時固定「重新發佈會重置存活時間」與「空訊息不算過期」。
 */
//
// 在此修正前，SetHudMessage() 會覆寫緩衝，而 TickHud 雖讓 hudAge_ 超過 kHudTtl
// 卻從不清空字串 —— 因此 View 已正確停止繪製（DrawHudMessage 在超過 TTL 時提前
// 返回），但 state.jsonl 工具仍在整段執行期間持續輸出同一段過時文字。
//
// HudExpired() 是個唯讀述詞（不改變狀態），故 View 的淡出動畫契約 —— 仍觀察原始
// 的 hudMessage_ / hudAge_ —— 維持逐位元不變。

#include "doctest/doctest.h"
#include "ui/MessageView.h"     // kHudTtl
#include "game/world/World.h"

#include <string>

// 當 hudAge_ 超過 kHudTtl 後，HudExpired 轉為 true。
TEST_CASE("hudAge_ 超過 kHudTtl 後 HudExpired 轉為 true") {
    nccu::World w{"", /*loadSprites=*/false};

    // 全新世界：尚未設定任何提示，故 HudExpired() 為 false（從未設定的 HUD
    // 不算「過期」—— 它沒有內容可過期）。
    CHECK_FALSE(w.HudExpired());
    CHECK(w.HudMessage().empty());

    w.SetHudMessage("章節清關");
    CHECK_FALSE(w.HudExpired());           // 剛發佈，存活時間 = 0
    CHECK(w.HudMessage() == "章節清關");

    // 40 次 0.1s 約為 4.0s；但浮點累加在某些 libm 上會讓 hudAge_ 略低於 kHudTtl
    // （40 * 0.1f 可能加成 3.99999...）。多加一次跳過 4 秒，確保不論捨入皆穩定
    // 落在 kHudTtl 之上 —— 契約是畫面上的 TTL，而非 0.1f 乘法的精確相等。邊界
    // 為包含式（DrawHudMessage 在 age >= kHudTtl 時提前返回，View 已轉空白），
    // HudExpired 對齊此邊界，使工具與 View 對「不再可見」的判定一致。
    for (int i = 0; i < 41; ++i) w.TickHud(0.1f);

    CHECK(w.HudExpired());
    // View 的淡出動畫需要訊息與存活時間在最後 kHudFade 秒內持續存在；此處刻意
    // 不清空 hudMessage_。HudExpired() 是工具唯一讀取的訊號。
    CHECK(w.HudMessage() == "章節清關");
    CHECK(w.HudAge() >= nccu::kHudTtl);
}

// 重新呼叫 SetHudMessage 會重置存活時間並清除過期狀態。
TEST_CASE("重新呼叫 SetHudMessage 會重置存活時間並清除過期狀態") {
    nccu::World w{"", /*loadSprites=*/false};

    w.SetHudMessage("first");
    // 讓存活時間超過 TTL，使 HudExpired = true。
    w.TickHud(nccu::kHudTtl + 0.5f);
    REQUIRE(w.HudExpired());

    // 新的一次發佈（透過 SetHudMessage）會重新錨定橫幅：存活時間歸零、HudExpired
    // 轉回 false。正式接線（WireHudMessageSubscriber）會在每個 EventType::ShowMessage
    // 呼叫 SetHudMessage，使章節／業力／商人提示乾淨地接替過時字串。
    w.SetHudMessage("second");
    CHECK_FALSE(w.HudExpired());
    CHECK(w.HudMessage() == "second");
    CHECK(w.HudAge() == doctest::Approx(0.0f));
}

// 空的訊息緩衝不算過期。
TEST_CASE("HudExpired 不理會空的訊息緩衝") {
    // 從未顯示過提示（或訊息已被明確清除）的 World 不算「過期」—— 沒有東西可
    // 過期。這保證工具在 HUD 確實無內容、以及過期提示被抑制這兩種情況下都輸出
    // 空字串；兩種情況收斂為相同的輸出格式。
    nccu::World w{"", /*loadSprites=*/false};
    w.TickHud(nccu::kHudTtl + 100.0f);   // 遠超 TTL，但無訊息
    CHECK_FALSE(w.HudExpired());
    CHECK(w.HudMessage().empty());
}
