#include "doctest/doctest.h"
#include "InputHandler.h"
#include "gfx/Input.h"
#include "gfx/Key.h"
#include <cstdint>

// Cycle 10.P0a: pin the edge / hold timing the InputHandler extracted
// from GameController exposes. The dialog hold-E auto-advance feature
// (audit H2 / D5 / SC 2.2.2) used to be inline state on the Controller;
// pulling it into a focused class lets a doctest drive the exact
// boundary cases — edge-only, hold-but-not-yet-armed, armed + cooldown
// — that the controller previously trusted by inspection.
//
// A minimal stub InputSource lets the tests inject deterministic key
// state without spinning up the real raylib stack or the harness's
// ScriptInput. Same interface as nccu::gfx::LiveInput / ScriptInput so
// behaviour stays byte-equivalent to the gameplay path.

namespace {

class StubInput final : public nccu::gfx::InputSource {
public:
    bool IsDown(nccu::gfx::Key k) const noexcept override {
        return down_ & Bit(k);
    }
    bool IsPressed(nccu::gfx::Key k) const noexcept override {
        return pressed_ & Bit(k);
    }
    bool IsReleased(nccu::gfx::Key k) const noexcept override {
        return released_ & Bit(k);
    }

    void Hold(nccu::gfx::Key k)    { down_ |= Bit(k); }
    void Release(nccu::gfx::Key k) { down_ &= ~Bit(k); released_ |= Bit(k); }
    void Tap(nccu::gfx::Key k) {
        down_ |= Bit(k);
        pressed_ |= Bit(k);
    }
    // Reset edges between "frames" — `down_` stays so a held key
    // remains down across frames exactly like raylib + ScriptInput.
    void NextFrame() {
        pressed_ = 0;
        released_ = 0;
    }

private:
    static std::uint32_t Bit(nccu::gfx::Key k) {
        // Pack relevant keys into bits. We only test E here; the
        // helper trivially extends to other keys if future tests add
        // them. Anything outside the [0, 31] hash range hits bucket 0
        // (E) which is fine for a single-key test.
        const int raw = static_cast<int>(k);
        return std::uint32_t{1} << (raw & 31);
    }
    std::uint32_t down_     = 0;
    std::uint32_t pressed_  = 0;
    std::uint32_t released_ = 0;
};

// RAII source swap so the global Input::SetSource is always restored —
// every test in this file is hermetic regardless of order.
class ScopedInputSource {
public:
    explicit ScopedInputSource(StubInput* src) {
        nccu::gfx::Input::SetSource(src);
    }
    ~ScopedInputSource() {
        nccu::gfx::Input::SetSource(nullptr);  // back to LiveInput
    }
    ScopedInputSource(const ScopedInputSource&)            = delete;
    ScopedInputSource& operator=(const ScopedInputSource&) = delete;
};

constexpr float kFrameDt = 1.0f / 60.0f;       // ~16.67 ms — pinned

} // namespace

TEST_CASE("InputHandler: edge-E fires advance on a one-frame tap") {
    StubInput stub;
    ScopedInputSource scope(&stub);
    nccu::InputHandler ih;

    // Frame 1: E tapped (edge-press AND down for this frame). Tap.
    stub.Tap(nccu::gfx::Key::E);
    CHECK(ih.TickDialogAdvance(kFrameDt));      // edge fired
    CHECK(ih.HoldMs() > 0.0f);                  // ms accumulating
    CHECK(ih.Cooldown() == 0);                  // edge != auto

    // Frame 2: tap edge cleared, key released. No advance.
    stub.NextFrame();
    stub.Release(nccu::gfx::Key::E);
    CHECK_FALSE(ih.TickDialogAdvance(kFrameDt));
    CHECK(ih.HoldMs() == 0.0f);                 // reset on release
    CHECK(ih.Cooldown() == 0);
}

TEST_CASE("InputHandler: holding E under 300 ms does NOT auto-advance") {
    StubInput stub;
    ScopedInputSource scope(&stub);
    nccu::InputHandler ih;

    // Frame 1: tap E (edge fires advance). Now hold it.
    stub.Tap(nccu::gfx::Key::E);
    CHECK(ih.TickDialogAdvance(kFrameDt));      // edge advance

    // Hold continuously without re-tapping. Edge IsPressed clears next
    // frame; only IsDown stays true while the key is held.
    for (int f = 1; f < 17; ++f) {              // 16 frames * 16.67 ≈ 266 ms
        stub.NextFrame();                       // clear edges, key still down
        CHECK_FALSE(ih.TickDialogAdvance(kFrameDt));  // not yet armed
    }
    // 17 frames * 16.67 ms ≈ 283 ms — still under 300 ms.
    CHECK(ih.HoldMs() < nccu::InputHandler::kHoldAdvanceMs);
}

TEST_CASE("InputHandler: holding E past 300 ms then cooldown-paced auto") {
    StubInput stub;
    ScopedInputSource scope(&stub);
    nccu::InputHandler ih;

    // Frame 1: tap E. Edge advance fires once.
    stub.Tap(nccu::gfx::Key::E);
    CHECK(ih.TickDialogAdvance(kFrameDt));

    // Tick frames while E stays held. Once HoldMs crosses 300 ms the
    // FIRST auto-fire lands; subsequent autos fire every (kAutoCooldown
    // Frames + 1) frames so cadence is readable. Count the autos
    // across a window long enough to see at least 3 in a row.
    int autoCount = 0;
    for (int f = 1; f < 40; ++f) {              // ~640 ms continuous hold
        stub.NextFrame();                       // edges clear, E still down
        if (ih.TickDialogAdvance(kFrameDt)) ++autoCount;
    }
    CHECK(autoCount >= 3);                      // multiple auto-fires
    // The cooldown reasserts after each fire (4 frames pinned).
    CHECK(nccu::InputHandler::kAutoCooldownFrames == 4);
}

TEST_CASE("InputHandler: release between holds resets the timer") {
    StubInput stub;
    ScopedInputSource scope(&stub);
    nccu::InputHandler ih;

    // First hold: get HoldMs to non-zero, then release.
    stub.Tap(nccu::gfx::Key::E);
    CHECK(ih.TickDialogAdvance(kFrameDt));
    stub.NextFrame();
    CHECK_FALSE(ih.TickDialogAdvance(kFrameDt));
    CHECK(ih.HoldMs() > 0.0f);

    // Release.
    stub.Release(nccu::gfx::Key::E);
    stub.NextFrame();
    CHECK_FALSE(ih.TickDialogAdvance(kFrameDt));
    CHECK(ih.HoldMs() == 0.0f);                 // reset

    // New tap a moment later — edge fires fresh, not "stale-hold auto".
    stub.Tap(nccu::gfx::Key::E);
    CHECK(ih.TickDialogAdvance(kFrameDt));
}

TEST_CASE("InputHandler: ResetDialogAdvance clears hold + cooldown") {
    StubInput stub;
    ScopedInputSource scope(&stub);
    nccu::InputHandler ih;

    // Accumulate hold time + arm cooldown.
    stub.Tap(nccu::gfx::Key::E);
    CHECK(ih.TickDialogAdvance(kFrameDt));
    for (int f = 1; f < 30; ++f) {
        stub.NextFrame();
        ih.TickDialogAdvance(kFrameDt);
    }
    CHECK(ih.HoldMs() > 0.0f);

    // Reset (Controller calls this when a dialog closes / never opens).
    ih.ResetDialogAdvance();
    CHECK(ih.HoldMs() == 0.0f);
    CHECK(ih.Cooldown() == 0);
}

TEST_CASE("InputHandler: edge + hold on the SAME frame double-fires nothing") {
    // Guards the edge-OR-auto dichotomy: a freshly-pressed E that
    // happens to ALSO satisfy the held-long-enough condition (rare,
    // but possible after a Reset) fires ONLY the edge path; the auto
    // path is skipped so a single press never advances the dialog
    // by 2 lines.
    StubInput stub;
    ScopedInputSource scope(&stub);
    nccu::InputHandler ih;

    // Cook the timer (simulating a previous hold that didn't fully
    // reset, even though normal release does — defensive check).
    // We poke HoldMs by ticking while down, then re-tap; the edge
    // path must dominate.
    stub.Hold(nccu::gfx::Key::E);
    for (int f = 0; f < 30; ++f) {
        stub.NextFrame();
        ih.TickDialogAdvance(kFrameDt);
    }
    // Force an edge-press on top of the hold.
    stub.NextFrame();
    stub.Tap(nccu::gfx::Key::E);                // tap == down + pressed
    const int cooldownBefore = ih.Cooldown();
    CHECK(ih.TickDialogAdvance(kFrameDt));      // returns true (edge)
    // The auto branch's cooldown stamp should NOT have re-armed: the
    // edge path returns true without ever touching the cooldown.
    CHECK(ih.Cooldown() == cooldownBefore);
}
