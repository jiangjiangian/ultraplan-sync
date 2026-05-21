#include "Harness.h"

#include "ScriptInput.h"
#include "World.h"
#include "Player.h"
#include "DialogState.h"
#include "SemesterStateMachine.h"
#include "GameObject.h"
#include "EventBus.h"
#include "gfx/Input.h"
#include "gfx/Time.h"
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

// Canonical quest/choice flags (SCRIPT_HANDOFF.md). Player exposes only
// HasFlag(name), so the observable set is enumerated here; the dump emits
// the subset currently true.
const std::vector<std::string>& KnownFlags() {
    static const std::vector<std::string> kFlags = {
        "Flag_FoundForm", "Flag_PromisedVictim", "Flag_HelpedTA_Ch1",
        "Flag_TookCursedUmbrella", "Flag_HasTrueUmbrella",
        // (Cycle-8 audit F1, B3 precedent: the inert KnowsUgly seed
        // formerly listed here was removed; the Ch1 阿姨 (c) buy is
        // now a pure narrative seed with no tracked flag — Ending-C
        // is driven entirely by Flag_BoughtUglyUmbrella set by the
        // Ch4 集英樓 Vendor, EndingGate.cpp.)
        "Flag_BoughtUglyUmbrella",
        "Flag_SuitSeniorChoiceMade", "Flag_ScoldedSenior",
        "Flag_HelpedSenior", "Flag_FoundNote1", "Flag_FoundNote2",
        "Flag_FoundNote3", "Flag_BookwormRecovered", "Flag_Ch2Cleared",
        "Flag_Ch2Rippled_SuitSenior", "Flag_Ch2Rippled_TA",
        "Flag_HasSausage", "Flag_HasLoudspeaker", "Flag_KnowsUmbrellaLoc",
        "Flag_Ch3Rippled_ProfTrap", "Flag_Ch3Cleared", "Flag_ConsoledTA",
        "Flag_TaFinaleChoiceMade", "Flag_Ch4Rippled_Senior",
        "Flag_Ch4Rippled_Bookworm", "Flag_Ch4Rippled_TAHelped",
        "Flag_Ch4Rippled_ProfTrap", "Flag_LeaveInterlude",
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
                    o += static_cast<char>(c);  // UTF-8 bytes pass through
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

// ---- Harness state -----------------------------------------------------
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
    // Non-owning snapshot of the World, captured read-only at EndFrame and
    // consumed by the NEXT BeginFrame's plan resolution. The harness never
    // mutates it (MVC purity); using the previous frame's state keeps the
    // high-level verbs a pure, deterministic function of the recorded
    // simulation. Null until the first EndFrame (plan idles until then).
    const World* lastWorld = nullptr;
};

namespace {

std::string DumpStateJson(const HarnessState& st, const World& world) {
    std::ostringstream o;
    o << '{';
    o << "\"frame\":" << st.frame;
    o << ",\"dt\":" << gfx::Time::DeltaSeconds();

    const std::string_view sem = world.Semester().CurrentName();
    o << ",\"semester\":\"" << EscapeJson(sem) << '"';

    const Player* p = world.GetPlayer();
    if (p) {
        const auto pos = p->GetPosition();
        o << ",\"player\":{\"x\":" << pos.x << ",\"y\":" << pos.y
          << ",\"karma\":" << p->GetKarma()
          << ",\"money\":" << p->GetMoney()
          << ",\"rain\":" << p->GetRainMeter()
          << ",\"umbrella\":" << (p->HasUmbrella() ? "true" : "false")
          << ",\"flags\":[";
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
    // Cycle 9.B L9: don't echo an aged-out toast — the View has
    // already stopped drawing it (DrawHudMessage early-returns when
    // age >= kHudTtl), so the state.jsonl should agree with what the
    // player can see, not with a stale buffer the World holds for the
    // fade-out contract. HudMessage() itself stays untouched (the View
    // owns that contract); the harness simply emits "" when expired.
    o << ",\"hud\":\""
      << (world.HudExpired() ? "" : EscapeJson(world.HudMessage()))
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
        gfx::Input::SetSource(nullptr);
        gfx::Time::SetFixedStep(0.0f);
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
    // A plan-driven run ends once every high-level verb has completed, so
    // timelines need no hand-placed `quit`/maxframes. Classic-only scripts
    // (HasPlan()==false) are never auto-quit here — the maxframes/`quit`
    // watchdog still governs them, behaviour unchanged. Guard frame_ >= 1
    // so a plan only ends AFTER it has actually run (frame 0 idles with no
    // World snapshot yet, so PlanDone() must not short-circuit the loop).
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
    // Compile the active high-level verb into this frame's synthetic key
    // edges, BEFORE GameController reads input. Resolved against the World
    // snapshot captured at the previous EndFrame (null on frame 0) — a
    // pure function of the recorded simulation, so replays are identical.
    // Read-only: the harness never mutates the World (MVC purity).
    s_->script->ResolvePlan(s_->lastWorld);
}

void Harness::EndFrame(const World& world) {
    if (!Active()) return;

    // Capture the post-Update World read-only for the NEXT BeginFrame's
    // plan resolution. `world` lives for the whole loop (main.cpp owns it
    // for the harness's lifetime), so the raw pointer stays valid.
    s_->lastWorld = &world;

    if (!s_->shotsDir.empty() && s_->shotEvery > 0 &&
        s_->frame % s_->shotEvery == 0) {
        std::error_code ec;
        std::filesystem::create_directories(s_->shotsDir, ec);
        char name[64];
        std::snprintf(name, sizeof(name), "frame_%06d.png", s_->frame);
        // raylib 5.5 TakeScreenshot() forcibly writes GetFileName(name)
        // into its base path (CWD) — a path argument is ignored by
        // design. So shoot to CWD, then move it where we want it.
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
    if (script.empty()) return h;             // inactive: zero behavior change

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

    gfx::Input::SetSource(st.script.get());
    gfx::Time::SetFixedStep(1.0f / 60.0f);
    return h;
}

} // namespace nccu
