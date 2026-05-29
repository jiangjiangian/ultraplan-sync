#ifndef EVENT_BUS_H_
#define EVENT_BUS_H_
#include "engine/events/HudSlot.h"
#include <cstdint>
#include <functional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @file EventBus.h
 * @brief 程序級事件匯流排（觀察者模式）：道具／UI 透過它廣播事件，發布者與訂閱者解耦。
 */

/** @brief 遊戲事件種類，供訂閱者依型別篩選。 */
enum class EventType {
    UmbrellaClaimed,   ///< 取得雨傘
    KarmaChanged,      ///< karma 數值變動
    ShowMessage,       ///< 顯示一則 HUD 訊息
    EnteredBuilding,   ///< 進入某棟建築
    /// 玩家取得非雨傘的物品欄道具；目前由 Vendor::TryBuy 在購買成功時發布，
    /// 事件 payload 的 text 欄位帶有道具 id（例如 "HotPack"）。
    PickupAcquired,
};

/**
 * @brief 一則事件：型別 + 文字載荷 + HUD 頻道。
 *
 * slot 將 ShowMessage 導向兩條獨立 HUD 頻道之一，使章節／結局提示能與一般拾取／
 * karma／抵達提示在同一幀並存（先前單槽會被後發布者覆蓋）。預設 HudSlot::Bottom
 * 讓既有發布者行為完全不變；僅少數高優先提示改用 HudSlot::Top。對非 ShowMessage
 * 的事件型別此欄無作用。
 */
struct Event {
    EventType        type;   ///< 事件種類
    std::string      text;   ///< 文字載荷（訊息內容或道具 id）
    nccu::HudSlot    slot = nccu::HudSlot::Bottom;   ///< 目標 HUD 頻道
};

/**
 * @brief 全域事件匯流排單例：集中收發遊戲事件，讓資料層與 UI 層互不直接相依。
 *
 * 道具與遊戲狀態只負責 Publish 事件，UI／音效等訂閱者各自 Subscribe；兩端不需要
 * 互相 #include。
 */
class EventBus {
public:
    using Handler = std::function<void(const Event&)>;

    /** @brief 取得程序級唯一實例。@return 全域 EventBus 參考。 */
    static EventBus& Instance();

    /**
     * @brief RAII 取消訂閱權杖：把一筆訂閱的生命週期綁定到一個 scope／擁有者。
     *
     * 由 ScopedSubscribe() 回傳；解構時剛好從匯流排移除它所擁有的那一個 handler，
     * 因此 handler 絕不會比它所捕捉的狀態活得更久（避免懸空捕捉的 use-after-free）。
     * 可移動（轉移所有權，不會重複取消）、不可複製。預設建構或被移走的權杖不持有
     * 任何訂閱，其解構為 no-op。移除以穩定 id 而非 iterator／指標進行——Publish 在
     * 分派前會先快照 handler 清單，因此即使在 handler 內取消訂閱，也只影響後續的
     * Publish，維持延後分派／可重入的合約。
     */
    class Subscription {
    public:
        Subscription() noexcept = default;
        Subscription(const Subscription&)            = delete;
        Subscription& operator=(const Subscription&) = delete;
        Subscription(Subscription&& other) noexcept;
        Subscription& operator=(Subscription&& other) noexcept;
        ~Subscription();

        /** @brief 此權杖目前是否持有一筆有效訂閱。 */
        bool Active() const noexcept { return bus_ != nullptr; }

        /**
         * @brief 立即移除所擁有的 handler（具冪等性）；之後解構為 no-op。
         *
         * 可安全地在 handler 內呼叫。
         */
        void Reset() noexcept;

    private:
        friend class EventBus;
        Subscription(EventBus* bus, std::uint64_t id) noexcept
            : bus_(bus), id_(id) {}

        EventBus*     bus_ = nullptr;
        std::uint64_t id_  = 0;
    };

    /**
     * @brief 訂閱某型別事件（流暢介面，可鏈式串接）。
     * @param[in] type    要訂閱的事件型別。
     * @param[in] handler 事件發生時呼叫的處理函式。
     * @return *this，便於鏈式呼叫。
     *
     * 此 handler 存活至下一次 Clear()。
     */
    EventBus& Subscribe(EventType type, Handler handler);

    /**
     * @brief Subscribe 的 RAII 版本：投遞語意相同，但回傳具範圍生命週期的權杖。
     * @param[in] type    要訂閱的事件型別。
     * @param[in] handler 事件發生時呼叫的處理函式。
     * @return Subscription 權杖；令其消滅即自動取消訂閱。
     *
     * 權杖一旦消滅（離開 scope／擁有者解構）即自動取消訂閱，免去手動 Clear() 簿記
     * 與懸空捕捉的 use-after-free。為附加功能，既有的 Subscribe()/Clear() 行為不變。
     */
    [[nodiscard]] Subscription ScopedSubscribe(EventType type,
                                               Handler   handler);

    /**
     * @brief 將事件廣播給所有對應型別的訂閱者。
     * @param[in] event 要發布的事件。
     */
    void      Publish(const Event& event) const;
    /** @brief 移除全部訂閱。@return *this。 */
    EventBus& Clear();

private:
    EventBus() = default;

    // 移除單一以 id 為鍵的 handler。若已不存在（Clear() 跑過、或已被取消）則為
    // no-op。可於分派途中安全呼叫：Publish 取自此函式可能執行前先拍下的快照。
    void Unsubscribe(std::uint64_t id) noexcept;

    struct Slot {
        std::uint64_t id;
        Handler       handler;
    };
    std::unordered_map<EventType, std::vector<Slot>> handlers_;
    std::uint64_t                                    nextId_ = 1;
    /// 僅保護 handler 清單、不保護 handler 函式體的讀寫鎖。
    /// Subscribe/ScopedSubscribe/Clear/Unsubscribe 取 unique_lock；Publish 取
    /// shared_lock 複製快照後即在分派前釋放。若從多執行緒呼叫 Publish，handler 函式
    /// 體仍會競爭——切勿在主執行緒以外發布：訂閱者函式體可能透過 View 觸碰 GL
    /// context，而 GL 是單執行緒。
    mutable std::shared_mutex mutex_;
};

#endif // EVENT_BUS_H_
