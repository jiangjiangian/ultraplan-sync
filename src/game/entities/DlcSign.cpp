#include "game/entities/DlcSign.h"
#include "engine/events/EventBus.h"
#include "engine/events/EventSink.h"
#include "engine/math/Color.h"
#include "engine/render/IRenderer.h"
#include "engine/math/Rect.h"

#include <string>

DlcSign::DlcSign(nccu::engine::math::Vec2 position)
    // 直接基底為 WithRoles<DlcSign, GameObject>，其 `using Base::Base` 繼承 GameObject 的
    // (position, hitBox) 建構子。28×28 碰撞盒——比 16×16 的拾取物略大，使粗體「？」讀來像
    // 一面立牌，且即使從緊貼障礙的方向也能被 E 探測（在 GameController 內膨脹到 ±8 px）輕鬆觸及。
    : WithRoles(position,
           nccu::engine::math::Rect{position.x, position.y, 28.0f, 28.0f}),
      // 字面 '\n' 會被 MessageView::WrapCjk 採納（在換行處斷開提示），且每個換行後的列現會
      // 在提示框內置中，故預告讀來是兩行整齊置中的字：
      //   DLC開發中
      //   敬請期待
      // （刪去了【…】括號與省略號，使兩行置中時平衡乾淨）。敬 已烘焙進字型圖集
      // （Font.h 的 UiLiteralChars），並由字型 UI 字面掃描測試強制檢查。
      message_("DLC開發中\n敬請期待") {}

void DlcSign::Render(nccu::engine::render::IRenderer& renderer) const {
    using nccu::engine::math::Rect;
    namespace C = nccu::engine::math::Colors;

    // 一個僅用矩形繪製的粗體「？」（佈景不得呼叫 DrawText／DrawTexture——架構規則，不含
    // raylib），採與 UmbrellaGlyph 相同的盒內比例樣式，使其隨碰撞盒縮放。先鋪一塊深色底
    // 板，使明亮的「？」在任何地面上都讀得出來，再以金色（「來互動我」的提示色）描出字符筆畫。
    const float x = hitBox_.x;
    const float y = hitBox_.y;
    const float w = hitBox_.width;
    const float h = hitBox_.height;
    auto rc = [&](float fx, float fy, float fw, float fh, nccu::engine::math::Color col) {
        renderer.DrawRect(Rect{x + fx * w, y + fy * h, fw * w, fh * h}, col);
    };

    // 底板（填滿碰撞盒）——一塊深色看板。
    rc(0.00f, 0.00f, 1.00f, 1.00f, nccu::engine::math::Color{30, 32, 40, 220});

    // 金色的「？」筆畫。上方的鉤、向中央彎下、一段空隙，再加一點——粗壯而清晰的問號。
    const nccu::engine::math::Color q = C::Gold;
    rc(0.28f, 0.14f, 0.44f, 0.12f, q);   // 鉤的上橫
    rc(0.62f, 0.14f, 0.16f, 0.30f, q);   // 鉤的右側下降
    rc(0.40f, 0.40f, 0.30f, 0.12f, q);   // 朝主幹彎入
    rc(0.42f, 0.50f, 0.16f, 0.20f, q);   // 向下至空隙的垂直主幹
    rc(0.42f, 0.80f, 0.16f, 0.12f, q);   // 下方的一點
}

void DlcSign::Interact(Player* /*initiator*/) {
    // 可重複閱讀：發布預告，但「不」使其失效——告示牌仍存於世界，故玩家可再次閱讀。無玩法
    // 效果（無旗標／業力／金錢），故未使用 initiator。
    nccu::events::Sink().Publish(
        Event{EventType::ShowMessage, message_});
}
