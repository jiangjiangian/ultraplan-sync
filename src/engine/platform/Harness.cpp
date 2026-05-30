#include "engine/platform/Harness.h"

#include "engine/platform/ScriptInput.h"
#include "game/quest/Flags.h"
#include "game/world/World.h"
#include "game/entities/Player.h"
#include "game/dialog/DialogState.h"
#include "game/state/SemesterStateMachine.h"
#include "engine/core/GameObject.h"
#include "engine/events/EventBus.h"
#include "engine/input/Input.h"
#include "engine/platform/Time.h"
#include "raylib.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace nccu {
namespace {

// 正規的任務／選擇旗標。Player 只暴露 HasFlag(name)，故可觀察的集合在此列舉；輸出時
// 只發出當前為真的子集。
const std::vector<std::string>& KnownFlags() {
    static const std::vector<std::string> kFlags = {
        kFlagFoundForm, kFlagPromisedVictim, kFlagHelpedTACh1,
        // 善有善報：可被尋獲的苦主之傘拾取旗標（由第一章 QuestFlagPickup 設下，於
        // TryReturnVictimUmbrella 的授予時清除）。列入白名單，使第一章的善有善報主線
        // 像 Flag_FoundForm 一樣在存檔中可觀察。
        kFlagHasVictimUmbrella,
        kFlagTookCursedUmbrella, kFlagHasTrueUmbrella,
        // （先前列於此處、無作用的 KnowsUgly 種子已移除；第一章阿姨 (c) 的購買現在是純
        // 敘事種子、不追蹤任何旗標——結局 C 完全由第四章集英樓攤販所設的
        // Flag_BoughtUglyUmbrella 驅動，見 EndingGate.cpp。）
        kFlagBoughtUglyUmbrella,
        kFlagSuitSeniorChoiceMade, kFlagScoldedSenior,
        kFlagHelpedSenior, kFlagFoundNote1, kFlagFoundNote2,
        kFlagFoundNote3, kFlagBookwormRecovered, kFlagCh2Cleared,
        kFlagCh2RippledSuitSenior, kFlagCh2RippledTA,
        kFlagHasSausage, kFlagHasLoudspeaker, kFlagKnowsUmbrellaLoc,
        // 圖書館管理員借傘的閂鎖，使第二章的借傘授予像其他任務旗標一樣在存檔中可觀察。
        kFlagLibrarianUmbrella,
        kFlagCh3RippledProfTrap, kFlagCh3Cleared, kFlagConsoledTA,
        kFlagTaFinaleChoiceMade, kFlagCh4RippledSenior,
        kFlagCh4RippledBookworm, kFlagCh4RippledTAHelped,
        kFlagCh4RippledProfTrap, kFlagLeaveInterlude,
        // 第四章結局自白的一次性旗標（TryOpenEndingConfession）。列出，使當測試驅動某個
        // 第四章結局時，先延後再解算的結局序列在存檔中可觀察。
        kFlagCh4ConfessedCursed, kFlagCh4ConfessedUgly,
        kFlagCh4ConfessedTrue,
    };
    return kFlags;
}

std::string EscapeJson(std::string_view s) {
    std::string o;
    o.reserve(s.size() + 8);
    for (unsigned char c : s) {
        switch (c) {
            case '"':  o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n";  break;
            case '\r': o += "\\r";  break;
            case '\t': o += "\\t";  break;
            default:
                if (c < 0x20) {
                    char buf[7];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    o += buf;
                } else {
                    o += static_cast<char>(c);  // UTF-8 位元組直接通過
                }
        }
    }
    return o;
}

const char* EventName(EventType t) {
    switch (t) {
        case EventType::UmbrellaClaimed: return "UmbrellaClaimed";
        case EventType::KarmaChanged:    return "KarmaChanged";
        case EventType::ShowMessage:     return "ShowMessage";
        case EventType::EnteredBuilding: return "EnteredBuilding";
        case EventType::PickupAcquired:  return "PickupAcquired";
    }
    return "Unknown";
}

std::string GetEnv(const char* k) {
    const char* v = std::getenv(k);
    return v ? std::string(v) : std::string();
}

} // namespace

// ---- Harness 狀態 -----------------------------------------------------
struct HarnessState {
    bool active = false;
    std::unique_ptr<ScriptInput> script;
    std::string shotsDir, statePath, spritePath;
    int  shotEvery = 30;
    int  maxFrames = 3600;
    int  frame     = -1;
    bool quitReq   = false;
    std::ofstream stateOut;
    std::vector<Event> events;
    // World 的非擁有快照，於 EndFrame 唯讀擷取，並由「下一次」 BeginFrame 的計畫解析
    // 消費。harness 絕不改寫它（MVC 純度）；使用前一幀的狀態，使高階動詞成為已記錄模擬
    // 的純粹且具決定性的函式。在首次 EndFrame 之前為 null（在那之前計畫待機）。
    const World* lastWorld = nullptr;
};

namespace {

std::string DumpStateJson(const HarnessState& st, const World& world) {
    std::ostringstream o;
    o << '{';
    o << "\"frame\":" << st.frame;
    o << ",\"dt\":" << nccu::engine::platform::Time::DeltaSeconds();

    const std::string_view sem = world.Semester().CurrentName();
    o << ",\"semester\":\"" << EscapeJson(sem) << '"';

    const Player* p = world.GetPlayer();
    if (p) {
        const auto pos = p->GetPosition();
        o << ",\"player\":{\"x\":" << pos.x << ",\"y\":" << pos.y
          << ",\"karma\":" << p->GetKarma()
          << ",\"money\":" << p->GetMoney()
          << ",\"rain\":" << p->GetRainMeter()
          << ",\"umbrella\":" << (p->HasUmbrella() ? "true" : "false");
        // 只在非零時才輸出 cursedTaint，使未受詛咒的測試基準維持逐位元一致。taint > 0 的
        // 執行刻意寫出一個「新」欄位——對該情境而言，相對於基準有可見差異是有意為之的，
        // 如同 flags 陣列增長一樣。
        if (p->GetCursedTaint() > 0)
            o << ",\"cursedTaint\":" << p->GetCursedTaint();
        o << ",\"flags\":[";
        bool first = true;
        for (const auto& f : KnownFlags()) {
            if (!p->HasFlag(f)) continue;
            if (!first) o << ',';
            o << '"' << EscapeJson(f) << '"';
            first = false;
        }
        o << "],\"consumables\":{";
        first = true;
        for (const auto& kv : p->Consumables()) {
            if (!first) o << ',';
            o << '"' << EscapeJson(kv.first) << "\":" << kv.second;
            first = false;
        }
        o << "}}";
    } else {
        o << ",\"player\":null";
    }

    const DialogState& d = world.Dialog();
    o << ",\"dialog\":{\"active\":" << (d.Active() ? "true" : "false")
      << ",\"npc\":\"" << EscapeJson(d.NpcId()) << '"'
      << ",\"atChoice\":" << (d.AtChoice() ? "true" : "false")
      << ",\"cursor\":" << d.ChoiceCursor()
      << ",\"line\":\"" << (d.Active() ? EscapeJson(d.CurrentLine()) : "")
      << "\",\"choices\":[";
    bool firstC = true;
    for (const auto& c : d.Choices()) {
        if (!firstC) o << ',';
        o << '"' << EscapeJson(c.label) << '"';
        firstC = false;
    }
    o << "]}";

    o << ",\"building\":\"" << EscapeJson(world.CurrentBuildingName()) << '"';
    o << ",\"invOpen\":" << (world.InventoryOpen() ? "true" : "false");
    // 不回放已過期的提示——View 早已停止繪製它（age >= kHudTtl 時 DrawHudMessage
    // 提前返回），故存檔輸出應與玩家所見一致，而非與 World 為淡出約定所保留的過時緩衝
    // 一致。HudMessage() 本身維持不動（該約定由 View 掌管）；harness 只在過期時輸出 ""。
    //
    // 拆成 top_hud + bottom_hud，使章節 -> 插曲段的轉場幀能同時記錄（Top）章節提示
    //「與」（Bottom）抵達提示——拆分前正是診斷所指出的那種互相覆蓋。舊的 "hud" 鍵已
    // 退役；下游診斷腳本改讀這兩個新鍵。
    o << ",\"top_hud\":\""
      << (world.HudExpired(HudSlot::Top)
              ? "" : EscapeJson(world.HudMessage(HudSlot::Top)))
      << '"';
    o << ",\"bottom_hud\":\""
      << (world.HudExpired(HudSlot::Bottom)
              ? "" : EscapeJson(world.HudMessage(HudSlot::Bottom)))
      << '"';

    int activeObjs = 0;
    std::ostringstream npcs;
    bool firstN = true;
    for (const auto& up : world.Objects()) {
        if (!up || !up->IsActive()) continue;
        ++activeObjs;
        const std::string_view id = up->NpcId();
        if (!id.empty()) {
            if (!firstN) npcs << ',';
            npcs << '"' << EscapeJson(id) << '"';
            firstN = false;
        }
    }
    o << ",\"objects\":{\"active\":" << activeObjs
      << ",\"npcs\":[" << npcs.str() << "]}";

    o << ",\"events\":[";
    bool firstE = true;
    for (const Event& e : st.events) {
        if (!firstE) o << ',';
        o << "{\"type\":\"" << EventName(e.type) << "\",\"text\":\""
          << EscapeJson(e.text) << "\"}";
        firstE = false;
    }
    o << "]}";
    return o.str();
}

} // namespace

// ---- Harness -----------------------------------------------------------
Harness::Harness() : s_(std::make_unique<HarnessState>()) {}
Harness::~Harness() {
    if (s_ && s_->active) {
        nccu::engine::input::Input::SetSource(nullptr);
        nccu::engine::platform::Time::SetFixedStep(0.0f);
    }
}
Harness::Harness(Harness&&) noexcept            = default;
Harness& Harness::operator=(Harness&&) noexcept = default;

bool Harness::Active() const noexcept { return s_ && s_->active; }

bool Harness::ShouldQuit() const noexcept {
    if (!Active()) return false;
    if (s_->quitReq) return true;
    if (!s_->script) return false;
    if (s_->script->WantsQuit()) return true;
    // 計畫驅動的執行在每個高階動詞都完成後即結束，故時間線無需手動放置 `quit`／maxframes。
    // 純傳統腳本（HasPlan()==false）在此絕不自動結束——maxframes／`quit` 看門狗仍管轄
    // 它們，行為不變。以 frame_ >= 1 為防護，使計畫只在「真正執行過」之後才結束（第 0 幀
    // 尚無 World 快照而待機，故 PlanDone() 不得短路整個迴圈）。
    return s_->frame >= 1 && s_->script->HasPlan() &&
           s_->script->PlanDone();
}

std::string Harness::SpritePath() const {
    return Active() ? s_->spritePath : std::string();
}

void Harness::WireEvents() {
    if (!Active()) return;
    HarnessState* st = s_.get();
    auto sink = [st](const Event& e) { st->events.push_back(e); };
    EventBus::Instance()
        .Subscribe(EventType::UmbrellaClaimed, sink)
        .Subscribe(EventType::KarmaChanged,    sink)
        .Subscribe(EventType::ShowMessage,     sink)
        .Subscribe(EventType::EnteredBuilding, sink)
        .Subscribe(EventType::PickupAcquired,  sink);
}

void Harness::BeginFrame() {
    if (!Active()) return;
    s_->script->Advance();
    ++s_->frame;
    // 在 GameController 讀取輸入「之前」，把當前作用中的高階動詞編譯成本幀的合成按鍵邊緣。
    // 以前一次 EndFrame 擷取的 World 快照解析（第 0 幀為 null）——為已記錄模擬的純函式，
    // 故重播一致。唯讀：harness 絕不改寫 World（MVC 純度）。
    s_->script->ResolvePlan(s_->lastWorld);
}

void Harness::EndFrame(const World& world) {
    if (!Active()) return;

    // 唯讀擷取 Update 後的 World，供「下一次」 BeginFrame 的計畫解析使用。`world` 存活於
    // 整個迴圈（main.cpp 在 harness 生命期內持有它），故裸指標保持有效。
    s_->lastWorld = &world;

    if (!s_->shotsDir.empty() && s_->shotEvery > 0 &&
        s_->frame % s_->shotEvery == 0) {
        std::error_code ec;
        std::filesystem::create_directories(s_->shotsDir, ec);
        char name[64];
        std::snprintf(name, sizeof(name), "frame_%06d.png", s_->frame);
        // raylib 5.5 的 TakeScreenshot() 會強制把 GetFileName(name) 寫進其基底路徑
        //（CWD）——路徑參數依設計被忽略。故先截圖到 CWD，再移動到我們要的位置。
        ::TakeScreenshot(name);
        std::filesystem::rename(name,
            std::filesystem::path(s_->shotsDir) / name, ec);
        if (ec) std::filesystem::remove(name, ec);
    }

    if (s_->stateOut.is_open())
        s_->stateOut << DumpStateJson(*s_, world) << '\n';

    s_->events.clear();

    if (s_->maxFrames > 0 && s_->frame + 1 >= s_->maxFrames)
        s_->quitReq = true;
}

Harness MaybeAttach() {
    Harness h;
    const std::string script = GetEnv("UMBRELLA_SCRIPT");
    if (script.empty()) return h;             // 未啟用：行為完全不變

    HarnessState& st = *h.s_;
    st.active     = true;
    st.script     = std::make_unique<ScriptInput>();
    st.script->LoadFile(script);
    st.shotsDir   = GetEnv("UMBRELLA_SHOTS");
    st.statePath  = GetEnv("UMBRELLA_STATE");
    st.spritePath = GetEnv("UMBRELLA_SPRITE");
    if (st.spritePath.empty())
        st.spritePath = "resources/assets/sprites/school_uniform_3/female_01.png";
    if (const std::string e = GetEnv("UMBRELLA_SHOT_EVERY"); !e.empty())
        st.shotEvery = std::atoi(e.c_str());
    if (const std::string m = GetEnv("UMBRELLA_MAXFRAMES"); !m.empty())
        st.maxFrames = std::atoi(m.c_str());
    if (!st.statePath.empty())
        st.stateOut.open(st.statePath, std::ios::out | std::ios::trunc);

    nccu::engine::input::Input::SetSource(st.script.get());
    nccu::engine::platform::Time::SetFixedStep(1.0f / 60.0f);
    return h;
}

} // namespace nccu
