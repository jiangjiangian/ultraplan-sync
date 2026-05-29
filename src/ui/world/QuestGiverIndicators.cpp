#include "ui/world/QuestGiverIndicators.h"

#include "game/controller/GameObjectQueries.h"
#include "engine/core/GameObject.h"
#include "game/entities/Player.h"
#include "game/quest/QuestIndicator.h"
#include "game/state/SemesterState.h"
#include "ui/QuestGiverIndicator.h"
#include "game/world/World.h"
#include "game/world/WorldConfig.h"
#include "engine/render/IRenderer.h"
#include "engine/math/Rect.h"

namespace nccu {

void DrawQuestGiverIndicators(nccu::engine::render::IRenderer& r, const World& world) {
    using namespace nccu::engine::render;
    using namespace nccu::engine::math;
    using nccu::queries::ForEachActive;

    // 任務給予者的「!」提示。在畫家順序這一趟「之後」、但仍在 CameraScope 內繪製，
    // 使圖示跟隨 NPC 位於世界座標空間——並疊在可能遮住躲在足跡後方之任務給予者的
    // 建築／sprite「之上」。QuestIndicatorVisible（任務層）是「唯一」的決策點：它把
    // 名冊的虛擬 IsQuestGiver() 位元與各章節規則（第三章依序 A→B→C 連鎖；第四章
    // 終局只有助教顯示 `!`）合併，使 View 維持純渲染——不含玩法邏輯、不用 dynamic_cast。
    // 注意第四章終局 NPC 在名冊中 isQuestGiver=false，故此決策「不能」再僅憑
    // IsQuestGiver() 短路；改由該述詞掌管。QuestGiverIndicator 將每個繪圖原語都經
    // IRenderer 送出，使該輔助函式可做 headless 測試。
    const Player* qgPlayer = world.GetPlayer();
    if (!qgPlayer) return;
    const nccu::SemesterState qgState = world.Semester().Current();
    ForEachActive(world.Objects(), [&](const GameObject& o) {
        if (!nccu::QuestIndicatorVisible(o.NpcId(), o.IsQuestGiver(),
                                         qgState, *qgPlayer)) return;
        // 碰撞盒位於 NPC 的雙腳；QuestGiverIndicator 會把「!」抬到以底邊對齊的
        // sprite 頂端之上。
        DrawQuestGiverIndicator(
            r,
            Rect{o.GetPosition().x, o.GetPosition().y,
                 ::world::kPlayerWidth, ::world::kPlayerHeight});
    });
}

}  // namespace nccu
