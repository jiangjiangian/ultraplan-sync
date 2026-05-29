#include "app/scenes/GameplayScene.h"

#include "engine/events/EventBus.h"
#include "engine/events/EventSink.h"
#include "engine/platform/Harness.h"
#include "engine/render/DrawScope.h"
#include "game/world/WorldOptions.h"

#include <cstdlib>
#include <cstring>

namespace nccu::app {

GameplayScene::GameplayScene(nccu::CharacterSelectResult selection,
                             nccu::audio::AudioDevice& audioDevice,
                             nccu::Harness& harness,
                             int windowWidth,
                             int windowHeight,
                             RestartFactory restartFactory)
    // Order MATTERS — same as main.cpp's pre-Phase-3 inline block.
    // World is the model; View is the presentation; GameController
    // wires both with the EventBus. AudioManager comes AFTER so
    // reverse-destruction tears down audio FIRST. The bus
    // initializer-list reference resolves to EventBus::Instance() so
    // the controller threads the SAME bus the entity-layer Sink
    // routes to (Plan P2 step 1+3).
    : world_(selection.spritePath, /*loadSprites=*/true,
             nccu::ReadWorldOptionsFromEnv()),
      view_(windowWidth, windowHeight),
      controller_(world_, EventBus::Instance()),
      audioManager_(EventBus::Instance(), audioDevice),
      harness_(harness),
      restartFactory_(std::move(restartFactory)) {
    // Harness-only debug warp: UMBRELLA_START_STATE jumps the FSM at
    // startup so a screenshot can reach a later chapter without
    // scripting the whole spine. Off by default — guarded by
    // harness.Active() AND the env var, so normal play and the
    // classic timed harness are byte-for-byte unchanged.
    if (harness_.Active()) {
        if (const char* s = std::getenv("UMBRELLA_START_STATE"); s) {
            using S = nccu::SemesterState;
            S target = world_.Semester().Current();
            bool warp = true;
            if      (std::strcmp(s, "Chapter2")  == 0) target = S::Chapter2_Midterms;
            else if (std::strcmp(s, "Chapter3")  == 0) target = S::Chapter3_SportsDay;
            else if (std::strcmp(s, "Chapter4")  == 0) target = S::Chapter4_Finals;
            else if (std::strcmp(s, "Interlude") == 0) target = S::Interlude_Market;
            else warp = false;
            if (warp) {
                world_.Semester().Transition(target);
                world_.RespawnChapterRoster(target);
            }
        }
    }
    // Plan P2 step 3: bind the entity-layer publish seam to the SAME
    // bus the controller threads through the chapter gates / quest
    // hooks. SetSink redirects nccu::events::Sink() from
    // EventBus::Instance() (the fallback) onto the bus the controller
    // owns this run. Reset to nullptr on Exit() so a Restart cycle
    // re-binds cleanly.
    nccu::events::SetSink(&EventBus::Instance());
    // Persona colour modulate — pure cosmetic, unchanged from main.cpp.
    if (Player* p = world_.GetPlayer())
        p->SetTint(selection.tint);
}

void GameplayScene::Enter() {
    // Harness's WireEvents subscribes to the bus AFTER the controller
    // owns it, so its subscribers tear down before the controller dtor
    // clears the bus on Exit. Matches the pre-Phase-3 ordering: ctor
    // owned the bus + sink wiring, then WireEvents joined.
    harness_.WireEvents();
}

SceneCommand GameplayScene::Update(float /*dt*/) {
    // The model-advance tick — identical to the inline body that used
    // to live in main.cpp's per-run while-loop, with one key change:
    // BeginFrame/EndFrame are NOT here. SceneManager::Run wraps every
    // Update + Draw pass with harness.BeginFrame() / harness.EndFrame(
    // world_) so the harness seam stays bit-identical and a future
    // Title/Select scene can share the same wrap without touching the
    // gameplay tick.
    controller_.Update();

    // In-game menu intent. The scripted harness input never opens the
    // pause menu, so the harness path is byte-for-byte unaffected; the
    // human path's Restart/Quit choices flow through here. Pop is not
    // used: a single-scene Phase 3.1 stack has nothing to pop to —
    // main.cpp owns the SceneManager teardown on RunOutcome::Restart
    // by simply rebuilding it.
    const auto act = world_.PendingAppAction();
    if (act == nccu::World::AppAction::Restart) {
        // Phase 3 step 4: rebuild via the composition-root-supplied
        // factory. main.cpp passes a closure that creates a fresh
        // LoadingScene (-> TitleScene -> CharacterSelectScene ->
        // GameplayScene), so the whole human path is re-enterable
        // without an outer loop in main. Harness path receives an
        // empty restartFactory_ and falls through to Quit, which
        // matches the never-restart contract: a script that asks
        // for Restart on a harness run ends the program rather than
        // looping back to title.
        if (restartFactory_) {
            auto fact = restartFactory_;
            return SceneCommand{SceneCommand::Kind::Replace,
                                std::move(fact)};
        }
        return SceneCommand{SceneCommand::Kind::Quit, {}};
    }
    if (act == nccu::World::AppAction::Quit)
        return SceneCommand{SceneCommand::Kind::Quit, {}};
    return SceneCommand{SceneCommand::Kind::None, {}};
}

void GameplayScene::Draw(nccu::gfx::IRenderer& /*renderer*/) {
    // The View renders straight through raylib via its existing
    // pipeline; Phase 3 step 1 keeps that intact — routing through
    // the abstract IRenderer is a step 2 concern (it lets a doctest
    // drive TitleScene without a real GL context). The DrawScope is
    // entered by SceneManager::Run for every Update+Draw pair.
    view_.Draw(world_);
}

void GameplayScene::Exit() {
    // Drop the entity-layer publish seam before the next iteration
    // rebuilds World/Controller (which would re-SetSink anyway).
    // Resetting to nullptr makes the seam fall through to
    // EventBus::Instance() for any code that runs between scenes
    // (asset cleanup, the title screen on the next Restart cycle).
    nccu::events::SetSink(nullptr);
}

} // namespace nccu::app
