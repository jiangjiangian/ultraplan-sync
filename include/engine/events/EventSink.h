#ifndef EVENT_SINK_H_
#define EVENT_SINK_H_
#include "engine/events/EventBus.h"

namespace nccu::events {

/**
 * @file EventSink.h
 * @brief 實體層發布事件的接縫（seam）：讓實體不直接點名 EventBus::Instance()，
 *        改向可替換的當前匯流排發布。
 *
 * 與刻意保留的 nccu::engine::input::Input::SetSource /
 * nccu::engine::platform::Time::SetFixedStep 測試接縫同形。實體層（雨傘家族／
 * 消耗品／NPC／拾取物／Vendor／BuildingTracker）原本直接呼叫
 * EventBus::Instance().Publish(...)；若改以參數把 EventBus& 串過每個
 * Interact/Consume/OnPickup 函式體，將連帶污染 IInteractable 的角色簽章與所有
 * 測試夾具。因此實體改透過此接縫發布：
 *
 *     nccu::events::Sink().Publish(Event{...});
 *
 * main.cpp／GameController 在啟動時呼叫一次 SetSink(&bus)，使程序當前的接收端
 * 即為控制器在章節切換與任務鉤子中所用的同一個 EventBus。測試可把接收端設成
 * 區域匯流排以求隔離、再設回 Instance()，或保持 null（預設）讓接縫退回
 * EventBus::Instance()，使既有呼叫點行為完全不變，直到明確改向為止。
 */

/**
 * @brief 設定程序當前的事件接收端。
 * @param[in] bus 要作為接收端的匯流排；傳 null 則退回 EventBus::Instance()。
 */
void SetSink(EventBus* bus) noexcept;

/** @brief 取得程序當前的事件接收端。@return 已設定的匯流排，否則 EventBus::Instance()。 */
[[nodiscard]] EventBus& Sink() noexcept;

} // namespace nccu::events

#endif // EVENT_SINK_H_
