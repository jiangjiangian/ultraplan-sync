#ifndef CONTROLLER_DIALOG_CHOICE_APPLY_H_
#define CONTROLLER_DIALOG_CHOICE_APPLY_H_

class Player;  // 全域命名空間的模型物件——前向宣告避免拉入整份 Player.h

namespace nccu {

struct DialogChoice;

/**
 * @brief 將一個「已確認」的對話選項之副作用套用到玩家身上（業力增減＋旗標寫入）。
 * @param[in,out] player 要套用副作用的玩家。
 * @param[in]     choice 玩家剛確認的對話選項。
 *
 * 設計為自由函式，使其不必啟動 Controller 的輸入迴圈即可單元測試。
 *
 * 一次性自旗標選項：給予「一次性」獎勵的選項表達為「業力 +N 並把某旗標設為
 * true」。福利社阿姨的「請咖啡」選項即為此形。其選單是可重複進入的選項開啟器，
 * 若無防護，玩家每次到訪都可重選而反覆刷取業力。因此：若選項欲設定的旗標玩家
 * 已持有，代表獎勵已領過，便同時略過業力與（本即冪等的）旗標寫入。suit_senior／
 * ta 的自鎖是靠「不再重現選單」在結構上防護；阿姨的選單會重現，故防護改放在此。
 * 清除型選項（flagValue=false）或玩家尚未持有的旗標則正常套用（首次仍給獎勵）。
 */
void ApplyDialogChoice(Player& player, const DialogChoice& choice);

} // namespace nccu

#endif // CONTROLLER_DIALOG_CHOICE_APPLY_H_
