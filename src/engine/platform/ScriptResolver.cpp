#include "engine/platform/ScriptInput.h"

#include "game/world/World.h"
#include "game/entities/Player.h"
#include "engine/core/GameObject.h"
#include "game/dialog/DialogState.h"
#include "engine/input/Key.h"
#include "raylib.h"

#include <cmath>
#include <string_view>

// ScriptInput::ResolvePlan 與其匿名命名空間內的支援輔助函式（AxisKeyToward +
// FindNpc + 6 個計畫預算常數）從 ScriptInput.cpp 抽到獨立 TU。這是 ScriptInput
// 的成員，此處「只」放實作。C++ 允許將類別實作依檔案分割，CMake 的 GLOB 也會自動
// 納入新的 TU。行為完全不變；所有計畫狀態變動（planPc_、planWatchdog_、plan_）
// 與 Synth* 呼叫皆與抽出前的內聯路徑逐位元相同。

namespace nccu {
using namespace nccu::engine::input;  // 輸入型別已自 nccu::gfx 移出
namespace {

// 移動常數——由玩法推導、經斷言而非猜測：
// 玩家速度 180 px/s，在 harness 固定的 1/60 s 步長下 => 沿軸恰為 3.0 px/frame
//（見 Player 建構式／Character::Move）。
constexpr float kPxPerFrame = 3.0f;
// 兩軸皆落在此帶內即視為抵達。小於一幀的位移量，故玩家不會在目標附近來回震盪（會停下，
// 不會越過 epsilon 又折返）。具決定性的浮點比較。
constexpr float kArriveEps  = 2.0f;
// 有界進度防護（具決定性的幀數預算，絕不用實際時鐘）：相對於最長的校園穿越仍寬裕，
// 但有限，使瞄錯的動詞會讓該次執行失敗而非卡死。
constexpr int kGotoBudget     = 4000;   // 約 66 秒的純移動
constexpr int kInteractBudget = 4200;   // 移動再加幾次 E
constexpr int kChooseBudget   = 64;     // 游標移動再加確認
constexpr int kAdvanceBudget  = 8;      // 翻一頁／一行所需的 E 次數

[[maybe_unused]] constexpr float kPxPerFrameRef = kPxPerFrame;  // 留存備記；目前未使用

int AxisKeyToward(float px, float py, float tx, float ty) {
    using nccu::engine::input::Key;
    const float dx = tx - px;
    const float dy = ty - py;
    if (std::fabs(dx) >= kArriveEps)
        return dx > 0.0f ? ToRaylibKey(Key::D) : ToRaylibKey(Key::A);
    if (std::fabs(dy) >= kArriveEps)
        return dy > 0.0f ? ToRaylibKey(Key::S) : ToRaylibKey(Key::W);
    return -1;
}

const GameObject* FindNpc(const World& w, std::string_view id) {
    for (const auto& up : w.Objects()) {
        if (!up || !up->IsActive()) continue;
        if (up->NpcId() == id) return up.get();
    }
    return nullptr;
}

} // namespace

void ScriptInput::ResolvePlan(const World* world) {
    using nccu::engine::input::Key;

    // 純傳統腳本「沒有」計畫動詞（plan_ 為空）。它完全靠 Advance() 在同一幀套用的
    // 定時 `down`/`up`/`press` 指令來操控 WASD（Advance 在 ResolvePlan 之前執行）。
    // 此情形下計畫解析器不擁有任何按鍵，故絕不可碰按鍵狀態——在 releaseMoveKeys()
    // 之前就返回，可保留傳統指令的按住／按下邊緣（曾發生的缺陷：此處若無條件呼叫
    // releaseMoveKeys()，會每幀對 W/A/S/D 送出放開，悄悄讓所有傳統 harness 移動失效）。
    if (plan_.empty()) return;

    static const int kMoveKeys[4] = {
        ToRaylibKey(Key::W), ToRaylibKey(Key::A),
        ToRaylibKey(Key::S), ToRaylibKey(Key::D)};

    // 放開計畫先前按住的任何移動鍵；每個動詞會在本幀重新按下它要的那一個。（只有
    // 帶計畫的腳本會走到這裡；純傳統腳本已在上方原封不動返回。若某腳本對同一個 WASD
    // 鍵同時混用計畫動詞與傳統指令，那是它自己的問題——動詞作用期間計畫擁有 WASD。）
    auto releaseMoveKeys = [&] {
        for (int k : kMoveKeys) SynthUp(k);
    };

    if (planPc_ >= plan_.size()) { releaseMoveKeys(); return; }
    if (!world) { releaseMoveKeys(); return; }      // 尚無快照：待機

    const Step& step = plan_[planPc_];

    auto finishStep = [&] {
        releaseMoveKeys();
        ++planPc_;
        planSub_      = 0;
        planWatchdog_ = 0;
    };

    switch (step.verb) {
        case Verb::Quit:
            quit_ = true;
            finishStep();
            return;

        case Verb::Wait:
            releaseMoveKeys();
            if (++planSub_ >= step.n) finishStep();
            return;

        case Verb::Goto: {
            const Player* p = world->GetPlayer();
            if (!p) { releaseMoveKeys(); return; }
            const auto pos = p->GetPosition();
            const int k = AxisKeyToward(pos.x, pos.y, step.x, step.y);
            releaseMoveKeys();
            if (k < 0) { finishStep(); return; }    // 已抵達
            SynthDown(k);
            if (++planWatchdog_ >= kGotoBudget) finishStep();  // 有界
            return;
        }

        case Verb::Interact: {
            const Player* p = world->GetPlayer();
            const GameObject* npc = FindNpc(*world, step.arg);
            if (!p) { releaseMoveKeys(); finishStep(); return; }
            // 找不到該 id 的 NPC。若此步驟帶有世界座標（`interact <label> <x> <y>`
            // 形式），這是一次非 NPC 的 E 觸發（拾取 TrueUmbrella／撿起申請書／開啟
            // 攤販選單）：移動到 (x,y)，並在「鏡像引擎到達判定」的觸及探測盒與目標處
            // 一個標準 24x24 物件足跡重疊時按下 E。用 24x24 盒（而非 1x1 點）對「會
            // 阻擋移動」的目標（攤販）至關重要：物理會把玩家緊貼停在距物件原點約 24 px
            // 處，故點狀閘永遠不會觸發（曾發生的軟鎖類別）。一個位於原點的 24x24 盒
            // 加上 8 px 觸及裕度，正是 GameController 真正的 E 探測盒對真實碰撞盒所用
            // 的幾何，對非阻擋道具而言也偏保守（玩家會走「上去」，遠在此盒內）。既無座標
            // 又無 NPC ⇒ 未知 id，略過此步驟（行為不變）。
            if (!npc) {
                if (step.n != 1) { releaseMoveKeys(); finishStep(); return; }
                // 攤販選單（或任何對話）現已開啟 ⇒ E 觸發已生效；直接交棒（如同 NPC
                // 分支在對話開啟時的提前返回），讓後續的 `choose`/`advance` 去操作選單。
                // 若缺此判斷，下方的 E 連點迴圈會持續往已開的攤販選單按 E、翻頁／確認它
                //（曾觀察到重複購買）。無聲的拾取（雨傘／硬幣／紙條）不會開啟對話，故仍
                // 落入連點路徑。
                if (world->Dialog().Active()) { finishStep(); return; }
                const auto pp = p->GetPosition();
                // 鏡像 GameController 的 E 探測觸及範圍，使 harness 的「我現在夠近、
                // 可以按 E 了」閘與引擎的「這次按下會生效」閘逐位元一致。讀取
                // World::LargeTargets()（由 UMBRELLA_LARGE_TARGETS=1 設定）可使腳本的
                // E 按鍵時機在無障礙設定下也維持正確。預設關閉 ⇒ 8.0f，與此設定推出前
                // 所有已固定的 harness 回歸一致。
                const float kInteractReach =
                    world->LargeTargets() ? 16.0f : 8.0f;
                const nccu::engine::math::Rect pHit{pp.x - kInteractReach,
                                     pp.y - kInteractReach,
                                     24.0f + 2.0f * kInteractReach,
                                     24.0f + 2.0f * kInteractReach};
                // 寬鬆的「位於原點的 24x24 盒」只決定「何時」開始按 E（對會阻擋移動、
                // 且把玩家緊貼停在距原點約 24 px 的攤販而言，這是必要的）。它刻意「不是」
                // 完成判定：真正的 GameController E 探測是玩家盒對物件「實際」碰撞盒
                //（16x16 任務旗標拾取物、20x20 雨傘、24x24 攤販），故一個 24x24 腳本盒
                // 可能已在「觸及」範圍內、但真正判定仍為偽。因此我們持續朝原點移動「並」
                // 每幀按 E，只有在玩家真正「抵達」原點時才交棒（k < 0 ⇒ 對非阻擋道具，
                // 每個真實碰撞盒此刻都已重疊、且已在行進途中拾取），或對攤販而言，在其選單
                // 開啟的那一刻交棒（上方的對話開啟防護，下一幀）。移動看門狗會為無法抵達的
                // 目標設下界限。
                const nccu::engine::math::Rect tgt{step.x, step.y, 24.0f, 24.0f};
                const int k = AxisKeyToward(pp.x, pp.y, step.x, step.y);
                releaseMoveKeys();
                if (k >= 0) SynthDown(k);             // 移動／持續頂住
                if (pHit.Intersects(tgt))             // 在 E 觸及範圍內
                    SynthPress(ToRaylibKey(Key::E));
                if (k < 0) { finishStep(); return; }  // 已抵達 ⇒ 已拾取
                if (++planWatchdog_ >= kInteractBudget) finishStep();
                return;
            }
            // 已在對話中且有內容開啟？則 interact 已滿足。
            if (world->Dialog().Active()) { finishStep(); return; }

            const auto pos  = p->GetPosition();
            const auto npos = npc->GetPosition();
            // 按 E 閘必須是與 GameController 互動修正「相同」的膨脹 AABB 重疊測試，而非
            //「AxisKeyToward 回傳 -1」。對 BlocksMovement() 的 NPC 而言，移動碰撞體是
            // 位於 NPC 原點、玩家大小的盒子，physics::ResolveMove 會把玩家「恰好緊貼」
            // 停在它前面（接觸但絕不重疊）。因此若把移動瞄向 NPC 近側的任何一點（舊的
            // npos.x-8 目標），會落在那道牆的另一側——無法抵達——抵達測試也永遠不會觸發
            //（即軟鎖）。改作法：直接走向 NPC「原點」；碰撞體會把玩家緊貼停在接觸 NPC
            // 處，再以一個「依互動觸及範圍膨脹」的探測盒（鏡像 GameController）與 NPC
            // 碰撞盒重疊，於是按 E——正是該互動修正所依賴、與真人遊玩相同的幾何。為兩個
            // 即時座標的純函式 => 具決定性。
            // 如同上方的非 NPC 分支，鏡像引擎在大型目標下的觸及範圍，使 harness 的
            //「現在按 E」閘在無障礙設定下也跟隨 GameController 的「這次按下會生效」閘。
            const float kInteractReach =
                world->LargeTargets() ? 16.0f : 8.0f;
            const nccu::engine::math::Rect pHit{pos.x - kInteractReach,
                                 pos.y - kInteractReach,
                                 24.0f + 2.0f * kInteractReach,
                                 24.0f + 2.0f * kInteractReach};
            // 每幀直直朝 NPC「原點」移動，直到對話開啟。AxisKeyToward 先收斂 X 再收斂
            // Y，故起點即使與 NPC 不同列，也會先匯聚到 NPC 那一列再於 X 上貼齊。移動
            // 碰撞體（位於 NPC 原點、玩家大小的盒子）會把玩家緊貼停在「接觸」NPC 處——
            // 永遠無法穿過（此不變式）——故在觸及範圍內持續按住該鍵只是把玩家頂貼住，
            // 不會越過。這與真人走近的方式一致：持續朝 NPC 推進並狂按 E。
            const int k = AxisKeyToward(pos.x, pos.y, npos.x, npos.y);
            releaseMoveKeys();
            if (k >= 0) SynthDown(k);                 // 移動／持續頂住
            // 按 E 閘 = 與 GameController 互動修正「相同」的膨脹 AABB 重疊測試（而非
            //「AxisKeyToward 回傳 -1」：瞄準原點會把玩家緊貼停在約 24 px 外，故抵達測試
            // 會重現軟鎖）。8 px 觸及裕度涵蓋緊貼接觸的間隙，使被牆擋住的玩家也能觸發——
            // 正是 GameController 所依賴、與真人遊玩相同的幾何。只要仍在觸及範圍內，按住
            // 的鍵會持續把玩家走進緊貼停止點（無法穿過），故最終停在相鄰位置。
            if (npc->CheckCollision(pHit))           // 在對話觸及範圍內
                SynthPress(ToRaylibKey(Key::E));
            if (++planWatchdog_ >= kInteractBudget) finishStep();
            return;
        }

        case Verb::Choose: {
            releaseMoveKeys();
            const DialogState& d = world->Dialog();
            if (!d.Active()) { finishStep(); return; } // 無可選項
            if (!d.AtChoice()) {                       // 逐行翻頁
                SynthPress(ToRaylibKey(Key::E));
                if (++planWatchdog_ >= kChooseBudget) finishStep();
                return;
            }
            const int cur = d.ChoiceCursor();
            if (cur != step.n) {                        // 移動游標
                SynthPress(cur < step.n ? ToRaylibKey(Key::Down)
                                        : ToRaylibKey(Key::Up));
                if (++planWatchdog_ >= kChooseBudget) finishStep();
                return;
            }
            SynthPress(ToRaylibKey(Key::E));            // 確認
            finishStep();
            return;
        }

        case Verb::Advance: {
            releaseMoveKeys();
            const DialogState& d = world->Dialog();
            if (!d.Active()) { finishStep(); return; } // 無可推進
            // 一次 E 點按使 DialogState 恰好前進一步——翻一頁、走一行，或關閉對話框
            //（已於 DialogState::Advance 驗證）。故單一具決定性的點按「就是」「推進行／頁
            // 或關閉」的單位。控制器於下一幀消費此邊緣；我們屆時交棒。以 static_assert
            // 引用此預算常數，在不觸發未使用符號警告下，明確保留「設計上有界」的約定。
            static_assert(kAdvanceBudget > 0, "advance is bounded by design");
            SynthPress(ToRaylibKey(Key::E));
            finishStep();
            return;
        }
    }
}

} // namespace nccu
