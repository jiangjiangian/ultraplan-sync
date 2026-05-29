#include "game/entities/TrueUmbrella.h"
#include "game/entities/Player.h"
#include "engine/events/EventBus.h"
#include "engine/events/EventSink.h"
#include "game/quest/Flags.h"

void TrueUmbrella::BeClaimed(Player* player) {
    if (!player) return;
    if (!isActive_) return;        // 冪等：第二次呼叫為無動作
    // 記錄背包中現在握的是哪把傘（同時設立 HasUmbrella）。
    player->SetHeldUmbrella(HeldUmbrella::True);
    // TrueUmbrella 專屬標記：HasUmbrella() 由每把傘都會設立，無法分辨 Ending A 的
    // 「持 TrueUmbrella」狀態。此旗標僅 TrueUmbrella 設立；GameController 於 Ch4
    // 進場（chapter4.md 傘再度失蹤）清除它，故在 Ch4 它正好表示「再次認領了 Ch4 的
    // TrueUmbrella」——Ending A 的精確條件，不會被建構期誤生的 Fragile／ProfTrap 污染。
    player->SetFlag(nccu::kFlagHasTrueUmbrella);
    isActive_ = false; // 標記待幀末清除
    // 發布順序攸關正確性：EventWiring 接上的 UmbrellaClaimed 訂閱者會以副作用發出
    // 章節清關的 ShowMessage（「✓ 章節清關 — 進入幕間市集」）。兩則 ShowMessage 都落在
    // 同一個單槽 HUD 頻道，故「最後」發布者才是玩家實際讀到的內容。先 ShowMessage、後
    // UmbrellaClaimed 可讓章節提示成為可見橫幅；雨傘訊息僅鏡像到日誌與 HUD 約 0 幀後便
    // 被章節提示覆寫。對調此兩行會重現雨傘字串遮蔽章節提示的回歸缺陷，由章節過場測試固定。
    nccu::events::Sink().Publish(Event{ EventType::ShowMessage, "你撿到了 TrueUmbrella，雨停了。" });
    nccu::events::Sink().Publish(Event{ EventType::UmbrellaClaimed, "TrueUmbrella" });
}
