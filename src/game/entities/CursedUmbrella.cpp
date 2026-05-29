#include "game/entities/CursedUmbrella.h"
#include "game/entities/Player.h"
#include "engine/events/EventBus.h"
#include "engine/events/EventSink.h"
#include "game/quest/Flags.h"

void CursedUmbrella::BeClaimed(Player* player) {
    if (player == nullptr) return;
    if (!isActive_) return;        // 冪等：第二次呼叫為無動作
    // 詛咒污點機制。拾取不再一次性扣業力；改由 IncCursedTaint() 累加計數，再交給每章的
    // ApplyCursedTaintDecay（SceneRouter Ch2/3/4 進場）每次過場扣 -5 × 污點。故第一次
    // 拾取在每次剩餘的章節過場各扣 -5（橫跨 Ch2/3/4 進場至多 -15）；第二次拾取使速率升至
    // 每場 -10、第三次升至 -15——道德污點隨再犯而累乘。Flag_TookCursedUmbrella 仍無條件
    // 設立（Ending B 前提不變），但因 EndingGate 中 A→B 優先序成立，後續若 karma > 80
    // 加上溫和的結局，玩家仍能抵達 Ending A——污點從不硬鎖 B，只是讓救贖在數學上更難。
    // SetHeldUmbrella 為背包列記錄握傘種類。
    player->SetHeldUmbrella(HeldUmbrella::Cursed)
           .IncCursedTaint()
           .SetFlag(nccu::kFlagTookCursedUmbrella);
    isActive_ = false;
    // 本幀不發 KarmaChanged（業力由污點衰減在章節邊界經 AddKarma 處理，後者會自行發出帶
    // 正負號 delta 的 KarmaChanged）。拾取當下的敘事提示維持同一行 ShowMessage——玩家在
    // 下手的瞬間仍會聽到「成為了你最討厭的人」。
    nccu::events::Sink().Publish(Event{ EventType::UmbrellaClaimed, "CursedUmbrella" });
    nccu::events::Sink().Publish(Event{ EventType::ShowMessage, "你順手牽羊了！成為了你最討厭的人。" });
}
