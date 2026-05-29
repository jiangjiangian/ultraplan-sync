#include "engine/events/EventSink.h"

namespace nccu::events {

namespace {
// 行程層級的指標，指向各呼叫端應發布事件的 EventBus。預設為 null，
// 使未做任何初始化的編譯單元（例如只建構 Player 的測試）退回到
// EventBus::Instance()，行為與導入注入式 sink 之前完全一致（byte 級相同）。
// main.cpp 會在建立 GameController 時一併設定；controller 解構與 main 的
// 外層收尾會將其重設為 null，讓重新開始時能乾淨地重新綁定。
EventBus* g_sink = nullptr;
} // namespace

void SetSink(EventBus* bus) noexcept { g_sink = bus; }

EventBus& Sink() noexcept {
    return g_sink ? *g_sink : EventBus::Instance();
}

} // namespace nccu::events
