#ifndef CONTROLLER_DIALOG_CHOICE_APPLY_H_
#define CONTROLLER_DIALOG_CHOICE_APPLY_H_

class Player;  // global-namespace model object

namespace nccu {

struct DialogChoice;

// Applies a confirmed dialog choice's side effects to the player. Free
// function so it is unit-testable without the controller's input loop.
//
// Item 5a — once-only self-flagging choices. A choice that grants a
// ONE-OFF reward expresses it as "karma +N AND set this flag true"; the
// Ch1 福利社阿姨 (d) 請咖啡 (karma +5, Flag_BoughtCoffeeForAuntie_Ch1) is
// exactly this shape. shop_auntie is a re-enterable choice-opener (its
// (b)/(c) 詢問傘/醜傘 are inert flavour with karma +0 / no flag, so re-
// talking them is harmless and stays allowed), but WITHOUT this guard a
// player could re-pick 請咖啡 every visit and farm +5 each time. So: if
// the choice would SET a flag the player ALREADY holds, the reward has
// already been collected — skip BOTH the karma and the (idempotent) flag
// write. This is the karma-application spot the suit_senior / ta
// self-locks (Flag_SuitSeniorChoiceMade / Flag_TaFinaleChoiceMade)
// protect structurally by never re-presenting their menus;
// shop_auntie's menu IS re-presented, so the guard lives here instead.
// A clearing choice (flagValue=false) or a flag the player does not yet
// hold applies normally (first pick still rewards).
void ApplyDialogChoice(Player& player, const DialogChoice& choice);

} // namespace nccu

#endif // CONTROLLER_DIALOG_CHOICE_APPLY_H_
