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
// Pipoya 行走圖集的 cell 數學現集中於共用且有單元測試的 gfx/WalkCycle.h（與 Player 相同的
// 來源），故校慶繞圈跑者與環境漫步者的動畫方式與 Player 完全一致。
constexpr int   kCell     = nccu::game::gfx::kPipoyaCell;
constexpr float kFrameDur = nccu::game::gfx::kWalkFrameDuration;
}  // namespace

NPC::NPC(nccu::engine::math::Vec2 position,
         std::vector<std::string> dialogLines,
         bool isQuestGiver,
         std::string_view npcId)
    // 直接基底為 WithRoles<NPC, Character>，其 `using Base::Base` 繼承 Character 的建構子，
    // 故此 3 參數形式仍可解析。
    : WithRoles(position,
                nccu::engine::math::Rect{position.x, position.y, 24.0f, 24.0f},
                0.0f /* 原型 NPC 為定點；由 EnableWander() 主動開啟漫步 */),
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
    speed_         = speed;                       // 於 Character 中為 protected
    rng_           = seed ? seed : 0x9E3779B9u;   // 切勿以全零狀態作為種子
    retargetTimer_ = 0.0f;                        // 第 1 幀即挑選朝向
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
    if (circular_) {  // 校慶繞圈跑者：沿固定跑道滑行並播放動畫
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
    if (!wander_) return;  // 原型 NPC 駐守原地

    retargetTimer_ -= deltaTime;
    if (retargetTimer_ <= 0.0f) {
        // xorshift32——八個羅盤方位之一，或索引 8 = 暫停。
        rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5;
        static constexpr nccu::engine::math::Vec2 kDirs[9] = {
            {0, -1}, {0, 1}, {-1, 0}, {1, 0},
            {-1, -1}, {1, -1}, {-1, 1}, {1, 1}, {0, 0}};
        wanderDir_ = kDirs[rng_ % 9u];
        rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5;
        retargetTimer_ = 1.0f + static_cast<float>(rng_ % 2000u) / 1000.0f;
    }

    const nccu::engine::math::Vec2 prev = position_;
    Move(wanderDir_, deltaTime);  // Character::Move 積分位移並同步 hitBox_

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
            // 撞到牆——在一拍之內轉向，而非原地磨蹭。
            retargetTimer_ = std::min(retargetTimer_, 0.2f);
        }
        SetPosition(resolved);
    }

    // 讓漫步者像 Player 一樣播放動畫。刻意取自不同來源的兩個獨立訊號（修正詭異抖動）：
    //
    //   moving_  = 此幀 NPC 是否實際位移（已過夾制與遮罩解算）？它閘控行走循環，使暫停
    //              （wanderDir_ == {0,0}）或完全撞牆的漫步者維持待機姿勢而非滑步。
    //
    //   facing_  = 「穩定的」預定朝向（wanderDir_），而非逐幀有雜訊的淨位移。WalkRowForFacing
    //              依主導軸挑選 Pipoya 列；在近對角線行走時，逐幀的 step 會因次像素捨入使哪
    //              一軸暫時較大而抖動，導致朝向列「每幀」翻動——即回報的抖動。重選的朝向在
    //              整段 1–3 秒的行程中恆定，故以 facing_ 為鍵每行程只得一列（完美對角線經
    //              WalkRowForFacing 的平手規則解析為上／下，同樣穩定）。撞牆的 NPC 仍朝向其
    //              預定方向（它仍試圖往那走），讀來正確且絕不閃爍。
    //
    // Render 讀取 moving_／facing_／animStep_——皆不進入序列化輸出（純渲染選擇）。
    const nccu::engine::math::Vec2 step{position_.x - prev.x, position_.y - prev.y};
    moving_ = (step.x != 0.0f || step.y != 0.0f);
    if (moving_) {
        // 取自恆定重選朝向的穩定朝向。守衛 {0,0} 情形（moving_ 為 true 時不會發生，但若未來
        // 呼叫者傳入零 wanderDir_，可保留上一個有效朝向）。
        if (wanderDir_.x != 0.0f || wanderDir_.y != 0.0f)
            facing_ = wanderDir_;
        animTimer_ += deltaTime;
        if (animTimer_ >= kFrameDur) {
            animTimer_ -= kFrameDur;
            animStep_ = (animStep_ + 1) % 4;
        }
    } else {
        animTimer_ = 0.0f;
        animStep_  = 0;   // WalkColumn(0) 即待機欄
    }
}

void NPC::Render(nccu::engine::render::IRenderer& renderer) const {
    using nccu::engine::math::Rect;
    if (!sprite_ || !sprite_->IsValid()) {
        renderer.DrawRect(hitBox_, nccu::engine::math::Colors::Green);
        return;
    }
    // 靜態整張貼圖（例如自動販賣機）：繪製「整張」貼圖、縮放至約 48 px 高、底部置中於碰撞盒
    // ——機器美術是單張 45×94 圖，而非 Pipoya 32×32 行走圖集。
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
    // 校慶繞圈跑者（circular_）與環境漫步者（wander_）皆會播放行走動畫並面向其運動方向；
    // 定點的對話／任務原型 NPC 則顯示第 0 列（面向鏡頭）的待機欄——不變。暫時靜止（暫停或撞
    // 牆：moving_ == false）的漫步者已在 Update 將 animStep_ 歸 0，故 WalkColumn(0) 使其停在
    // 待機姿勢。CurrentRenderCell() 是挑選 cell 的「唯一」處（無頭測試在此固定）。將 32×32
    // sprite 底部置中於 24×24 碰撞盒，使腳部落在底邊——與 Player 相同的慣例。
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
    // 漫步者或校慶跑者播放以朝向為鍵的行走循環；定點原型 NPC 停在待機的欄 1／列 0。
    // moving_ == false 已在 Update 將 animStep_ 釘為 0，故 WalkColumn 無須特例即可處理靜止情形。
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
    // 在派發事件「之前」先快照當前台詞文字並推進索引。EventBus::Publish 為同步且支援遞迴
    // 派發，故任何回呼進 Interact() 的訂閱者都必須觀察到已推進後的狀態，而非陳舊索引。
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
