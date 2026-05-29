#include "game/entities/NPC.h"
#include "game/dialog/DialogSource.h"
#include "engine/events/EventBus.h"
#include "engine/events/EventSink.h"
#include "game/world/Physics.h"
#include "game/world/WorldConfig.h"
#include "engine/render/IRenderer.h"
#include "engine/math/Color.h"
#include "engine/math/Rect.h"
#include "game/gfx/WalkCycle.h"
#include <algorithm>
#include <cmath>

namespace {
// Pipoya walk-sheet cell maths now live in the shared, unit-tested
// gfx/WalkCycle.h (the same source the Player follows), so a 校慶 crowd
// runner AND an ambient wanderer animate exactly the way the Player does.
constexpr int   kCell     = nccu::game::gfx::kPipoyaCell;
constexpr float kFrameDur = nccu::game::gfx::kWalkFrameDuration;
}  // namespace

NPC::NPC(nccu::engine::math::Vec2 position,
         std::vector<std::string> dialogLines,
         bool isQuestGiver,
         std::string_view npcId)
    // Direct base is WithRoles<NPC, Character>; its `using Base::Base`
    // inherits Character's ctor so this 3-arg form still resolves.
    : WithRoles(position,
                nccu::engine::math::Rect{position.x, position.y, 24.0f, 24.0f},
                0.0f /* archetype NPCs are stationary; EnableWander() opts in */),
      dialogLines_(std::move(dialogLines)),
      currentLineIndex_(0),
      isQuestGiver_(isQuestGiver),
      npcId_(npcId),
      wander_(false),
      retargetTimer_(0.0f),
      wanderDir_{0.0f, 0.0f},
      rng_(0),
      wanderMask_(nullptr) {}

NPC& NPC::EnableWander(float speed, unsigned seed) noexcept {
    wander_        = true;
    speed_         = speed;                       // protected in Character
    rng_           = seed ? seed : 0x9E3779B9u;   // never seed an all-zero state
    retargetTimer_ = 0.0f;                        // pick a heading on frame 1
    return *this;
}

NPC& NPC::EnableCircularRun(nccu::engine::math::Vec2 center, float radius,
                            float angularSpeed, float startAngle) noexcept {
    circular_     = true;
    circleCenter_ = center;
    circleRadius_ = radius;
    circleSpeed_  = angularSpeed;
    circleAngle_  = startAngle;
    return *this;
}

void NPC::Update(float deltaTime) {
    if (circular_) {  // 校慶 crowd runner: glide the fixed track + animate
        const nccu::engine::math::Vec2 prev = position_;
        circleAngle_ += circleSpeed_ * deltaTime;
        SetPosition(nccu::engine::math::Vec2{
            circleCenter_.x + circleRadius_ * std::cos(circleAngle_),
            circleCenter_.y + circleRadius_ * std::sin(circleAngle_)});
        const nccu::engine::math::Vec2 d{position_.x - prev.x, position_.y - prev.y};
        if (d.x != 0.0f || d.y != 0.0f) facing_ = d;
        animTimer_ += deltaTime;
        if (animTimer_ >= kFrameDur) {
            animTimer_ -= kFrameDur;
            animStep_ = (animStep_ + 1) % 4;
        }
        return;
    }
    if (!wander_) return;  // archetype NPCs stand at their post

    retargetTimer_ -= deltaTime;
    if (retargetTimer_ <= 0.0f) {
        // xorshift32 — one of 8 compass headings, or idx 8 = pause.
        rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5;
        static constexpr nccu::engine::math::Vec2 kDirs[9] = {
            {0, -1}, {0, 1}, {-1, 0}, {1, 0},
            {-1, -1}, {1, -1}, {-1, 1}, {1, 1}, {0, 0}};
        wanderDir_ = kDirs[rng_ % 9u];
        rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5;
        retargetTimer_ = 1.0f + static_cast<float>(rng_ % 2000u) / 1000.0f;
    }

    const nccu::engine::math::Vec2 prev = position_;
    Move(wanderDir_, deltaTime);  // Character::Move integrates + syncs hitBox_

    const float kMaxXY = ::world::kSize - ::world::kPlayerWidth;
    nccu::engine::math::Vec2 p = position_;
    p.x = std::clamp(p.x, 0.0f, kMaxXY);
    p.y = std::clamp(p.y, 0.0f, kMaxXY);
    SetPosition(p);

    if (wanderMask_) {
        static const std::vector<nccu::engine::math::Rect> kNoDynamic;
        const nccu::engine::math::Vec2 resolved = nccu::physics::ResolveMove(
            prev, position_,
            nccu::engine::math::Vec2{::world::kPlayerWidth, ::world::kPlayerHeight},
            kNoDynamic, wanderMask_);
        if (resolved.x != position_.x || resolved.y != position_.y) {
            // Bumped a wall — turn away within a beat instead of grinding.
            retargetTimer_ = std::min(retargetTimer_, 0.2f);
        }
        SetPosition(resolved);
    }

    // U1-T3: animate the wanderer like the Player. TWO independent
    // signals, deliberately kept on different sources (A-T1 詭異抖動 fix):
    //
    //   moving_  = did the NPC actually DISPLACE this frame (post-clamp,
    //              post-mask-resolve)? It gates the walk cycle, so a paused
    //              (wanderDir_ == {0,0}) or fully wall-blocked wanderer
    //              holds its idle pose instead of moon-walking.
    //
    //   facing_  = the STABLE intended heading (wanderDir_), NOT the noisy
    //              per-frame net displacement. WalkRowForFacing picks the
    //              Pipoya row by dominant axis; on a near-diagonal walk the
    //              per-frame `step` jitters left↔down/up as sub-pixel
    //              rounding flips which axis is momentarily larger, flipping
    //              the facing row EVERY frame — the reported jitter. The
    //              retarget heading is constant for a whole 1–3 s leg, so
    //              keying facing_ to it yields ONE row per leg (a perfect
    //              diagonal resolves to up/down via the WalkRowForFacing
    //              tie-break, also stable). A wall-blocked NPC keeps facing
    //              its intended heading (it's still trying to go that way),
    //              which reads correctly and never flickers.
    //
    // Render reads moving_/facing_/animStep_ — none of which is serialised
    // (state.jsonl stays byte-identical; verified vs baseline md5).
    const nccu::engine::math::Vec2 step{position_.x - prev.x, position_.y - prev.y};
    moving_ = (step.x != 0.0f || step.y != 0.0f);
    if (moving_) {
        // Stable facing from the constant retarget heading. Guard the
        // {0,0} case (can't happen while moving_ is true, but keeps the
        // last good facing if a future caller drives a zero wanderDir_).
        if (wanderDir_.x != 0.0f || wanderDir_.y != 0.0f)
            facing_ = wanderDir_;
        animTimer_ += deltaTime;
        if (animTimer_ >= kFrameDur) {
            animTimer_ -= kFrameDur;
            animStep_ = (animStep_ + 1) % 4;
        }
    } else {
        animTimer_ = 0.0f;
        animStep_  = 0;   // WalkColumn(0) == idle column
    }
}

void NPC::Render(nccu::engine::render::IRenderer& renderer) const {
    using nccu::engine::math::Rect;
    if (!sprite_ || !sprite_->IsValid()) {
        renderer.DrawRect(hitBox_, nccu::engine::math::Colors::Green);
        return;
    }
    // Static full-image sprite (e.g. a 自動販賣機): draw the WHOLE texture
    // scaled to ~48 px tall, bottom-centred on the hitbox — machine art is
    // a single 45×94 image, not a Pipoya 32×32 walk sheet.
    if (staticSprite_) {
        const float tw = static_cast<float>(sprite_->Width());
        const float th = static_cast<float>(sprite_->Height());
        const float dh = 48.0f;
        const float dw = (th > 0.0f) ? tw * (dh / th) : 24.0f;
        const Rect dest{hitBox_.x + (hitBox_.width - dw) * 0.5f,
                        hitBox_.y + hitBox_.height - dh, dw, dh};
        renderer.DrawSprite(*sprite_, Rect{0.0f, 0.0f, tw, th}, dest);
        return;
    }
    // A 校慶 crowd runner (circular_) AND an ambient wanderer (wander_)
    // walk-animate and face their motion (U1-T3, owner's request); a
    // stationary dialogue / quest archetype NPC shows the idle column at
    // row 0 (facing the camera) — unchanged. A wanderer that's momentarily
    // still (paused or wall-blocked: moving_ == false) snapped animStep_
    // back to 0 in Update, so WalkColumn(0) keeps it on the idle pose.
    // CurrentRenderCell() is the SINGLE place the cell is chosen (the
    // headless test pins it there). Bottom-centre the 32x32 sprite on the
    // 24x24 hitbox so feet sit at the base — same convention as Player.
    const RenderCell cell = CurrentRenderCell();
    const Rect src{
        static_cast<float>(cell.col * kCell),
        static_cast<float>(cell.row * kCell),
        static_cast<float>(kCell),
        static_cast<float>(kCell)};
    const Rect dest{
        hitBox_.x + (hitBox_.width  - kCell) * 0.5f,
        hitBox_.y +  hitBox_.height - kCell,
        static_cast<float>(kCell),
        static_cast<float>(kCell)};
    renderer.DrawSprite(*sprite_, src, dest);
}

NPC::RenderCell NPC::CurrentRenderCell() const noexcept {
    // A wanderer or 校慶 runner plays the walk cycle keyed to its heading;
    // a stationary archetype NPC holds idle column 1 / row 0. moving_==false
    // already pinned animStep_ to 0 in Update, so WalkColumn handles the
    // at-rest case without a special branch.
    if (circular_ || wander_) {
        return RenderCell{nccu::game::gfx::WalkColumn(animStep_),
                          nccu::game::gfx::WalkRowForFacing(facing_)};
    }
    return RenderCell{1, 0};
}

void NPC::LoadSprite(const std::string& path) {
    sprite_ = nccu::engine::render::Texture::Load(path);
}

void NPC::Interact(Player* /*initiator*/) {
    if (dialogLines_.empty()) return;
    // Snapshot the current line text and advance the index BEFORE
    // dispatching the event. EventBus::Publish is synchronous and
    // supports recursive dispatch (see test_eventbus reentrancy case),
    // so any subscriber that calls back into Interact() must observe
    // the already-advanced state rather than a stale index.
    const std::string line = dialogLines_[currentLineIndex_];
    currentLineIndex_ = (currentLineIndex_ + 1) % dialogLines_.size();
    nccu::events::Sink().Publish(Event{ EventType::ShowMessage, line });
}

std::string_view NPC::NpcId() const noexcept { return npcId_; }

const std::string& NPC::CurrentLineText() const {
    static const std::string empty;
    if (dialogLines_.empty()) return empty;
    return dialogLines_[currentLineIndex_];
}

NPC& NPC::SetDialogLines(std::vector<std::string> lines) {
    dialogLines_ = std::move(lines);
    currentLineIndex_ = 0;
    return *this;
}

NPC& NPC::LoadDialog(std::string_view npcId, nccu::SemesterState state,
                     int subState) {
    for (const auto& e : nccu::dialog::Entries(npcId, state)) {
        if (e.subState == subState) {
            return SetDialogLines(e.lines);
        }
    }
    return SetDialogLines({});
}
