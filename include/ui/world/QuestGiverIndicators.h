#ifndef UI_WORLD_QUEST_GIVER_INDICATORS_H_
#define UI_WORLD_QUEST_GIVER_INDICATORS_H_

namespace nccu {

class World;
namespace engine::render { class IRenderer; }

/**
 * @file QuestGiverIndicators.h
 * @brief 任務給予者「!」浮標（自 View::RenderWorld 抽出）。
 */

/**
 * @brief 走訪存活 NPC，對每個 `QuestIndicatorVisible(npcId, IsQuestGiver,
 *        semester, player)` 判定為真者，在其上方抬起任務「!」浮標。
 *
 * 在繪製順序掃描「之後」、但仍在呼叫端持有的 CameraScope「之內」繪製，使浮標於
 * 世界座標跟隨各 NPC，並疊在可能遮住躲於建築輪廓後之任務給予者的建築／sprite 之
 * 上。QuestIndicatorVisible 是「唯一」判定點（把名冊的 IsQuestGiver() 位元與各章
 * 規則折在一起），使 View 維持純渲染：無遊戲邏輯、無 dynamic_cast。第四章終局
 * NPC 的 isQuestGiver=false，故判定不能僅靠 IsQuestGiver() 短路——由
 * QuestIndicatorVisible 負責。無玩家時為空操作；每幀呼叫皆安全。
 */
void DrawQuestGiverIndicators(nccu::engine::render::IRenderer& r, const World& world);

}  // namespace nccu

#endif  // UI_WORLD_QUEST_GIVER_INDICATORS_H_
