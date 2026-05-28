#ifndef ITEM_CATALOG_H_
#define ITEM_CATALOG_H_
#include "entities/Player.h"   // class Player + HeldUmbrella (bag row source)
#include <string>
#include <string_view>
#include <vector>

class EventBus;                 // Plan P2 step 2: ApplyConsumableEffect injects it

namespace nccu {

// Item 2(d) — the SINGLE code catalog of every item the player can hold,
// keyed by the same itemId the rest of the engine uses (the consumable
// count map key, the Vendor stock itemId, and the umbrella / quest-item
// sentinels the inventory DTO synthesises). Pure data — no raylib, no
// World/Player access in the lookup itself — so both the Tab inventory
// DTO (BuildInventoryRows) and the Vendor purchase toast (Item 5b) read
// their 中文 display name + description from ONE place and never drift.
//
// Forbidden-string rule (CLAUDE.md §5): every string here is in-fiction
// zh-TW; no external tool / model / brand identifier appears.
struct ItemInfo {
    std::string_view displayName;   // 中文 name shown in the bag / toast
    std::string_view description;   // one short line: what it does
};

// Sentinel itemIds for the non-consumable bag categories (the count map
// only ever holds consumable / vendor itemIds, so these can never collide
// with a real key). The inventory DTO uses them to look up the 中文 name
// + description of money, the carried umbrella variants, and the quest
// papers — keeping ALL copy in the one catalog.
inline constexpr const char* kItemMoney         = "__money__";
inline constexpr const char* kItemTrueUmbrella  = "__umbrella_true__";
inline constexpr const char* kItemCursedUmbrella = "__umbrella_cursed__";
inline constexpr const char* kItemUglyUmbrella  = "__umbrella_ugly__";
inline constexpr const char* kItemVictimUmbrella = "__umbrella_victim__";
// B2.1: held-umbrella sentinels for the remaining HeldUmbrella kinds so the
// bag can show EVERY umbrella the player can actually hold over their head
// (the 破傘 / 陷阱傘 ground pickups and the Ch2 管理員的傘 loaner), keyed off
// HeldUmbrellaKind() rather than the persistent ending flags.
inline constexpr const char* kItemFragileUmbrella  = "__umbrella_fragile__";
inline constexpr const char* kItemProfTrapUmbrella = "__umbrella_proftrap__";
inline constexpr const char* kItemLoanerUmbrella   = "__umbrella_loaner__";
inline constexpr const char* kItemForm          = "__quest_form__";
inline constexpr const char* kItemNotes         = "__quest_notes__";
// B2.4: the Ch3 物物交換鏈 carried items (香腸 / 大聲公). Like the 申請書 /
// 筆記 they are flag-modelled single-use quest items; surfacing them in the
// bag makes the trade chain visible (and they vanish the instant a trade
// consumes the prior flag — Chapter3Quest clears it). 情報 (KnowsUmbrellaLoc)
// is knowledge, not a carried object, so it has no bag row.
inline constexpr const char* kItemSausage       = "__quest_sausage__";
inline constexpr const char* kItemLoudspeaker   = "__quest_loudspeaker__";

// B2.1: the catalog sentinel itemId for a held umbrella kind (the id the bag
// row + InventoryView glyph key off). Returns nullptr for None / Victim (the
// 苦主's carried umbrella is shown via its quest flag, not the held-kind, as
// it grants no shelter). Pure mapping — no Player access.
[[nodiscard]] const char* HeldUmbrellaCatalogId(HeldUmbrella kind);

// B2.1/B2.4: map a VENDOR-stock umbrella itemId (e.g. "UglyUmbrella") to the
// HeldUmbrella kind the buyer then holds, so a bought umbrella becomes a held
// umbrella (a bag umbrella row + auto-shelter) instead of a phantom count
// entry. Returns HeldUmbrella::None for any non-umbrella id (the common
// case — food / drinks / gear stay count-consumables). Pure mapping.
[[nodiscard]] HeldUmbrella HeldUmbrellaForItemId(std::string_view itemId);

// Returns the catalog entry for `itemId`. An unknown id yields a sane
// fallback ({itemId-as-name, "" }) so a future stock item without a
// catalog row still renders its raw id rather than crashing or printing
// blank — never throws.
[[nodiscard]] ItemInfo ItemInfoFor(std::string_view itemId);

// 5c/T5 — every catalog displayName + description string, for the
// glyph-coverage scan (these names/descriptions render in the bag rows and
// the vendor purchase toast, so every glyph must be baked into gfx::Font.h).
// Pure data; flattens the internal catalog table. Order is unspecified.
[[nodiscard]] std::vector<std::string> CatalogStrings();

// True when `itemId` is a consumable the player can USE from the bag
// (Item 2b). Drives the DTO's `usable` flag and gates the use-from-bag
// effect: only these ids have an effect to apply; money / umbrellas /
// quest papers are view-only entries.
[[nodiscard]] bool IsUsableConsumable(std::string_view itemId);

// True when `itemId` names an umbrella (any of the catalog umbrella
// sentinels OR a vendor umbrella stock id — substring "Umbrella"/
// "umbrella"). A held-over-head umbrella is surfaced by the held-kind row,
// NOT the count loop, so BuildInventoryRows EXCLUDES these from the
// consumable count (B2.1) and InventoryView classifies a row as the
// Umbrella kind by it. Single source so the two callers (ItemCatalog's
// row builder + InventoryView's row classifier) can never drift.
[[nodiscard]] bool IsUmbrellaItemId(std::string_view itemId);

// Item 2b — apply a held consumable's effect when the player uses it from
// the open bag. This is the SAME effect each ConsumableItem::Consume body
// applies (identical karma delta + identical ShowMessage text, sharing the
// entity kKarmaBonus constants), now player-triggered instead of fired on
// pickup. No-op for a non-usable / unknown id. Does NOT touch the count
// map — the caller (controller) decrements via Player::ConsumeOne so the
// "spend one" bookkeeping stays in one place. Publishes the flavour
// ShowMessage through the EventBus exactly like the pickup path used to.
// Plan P2 step 2: `bus` is injected; publishes the flavour ShowMessage
// + per-effect ShowMessage exactly where the pickup path used to.
void ApplyConsumableEffect(EventBus& bus, Player& player,
                           std::string_view itemId);

// Item 2(c)/(e) — one render-only row of the Tab inventory. The DTO the
// View draws: a 中文 name, a count (0 ⇒ no "xN" suffix, used by the
// single-instance umbrella / quest-paper rows), the catalog description,
// and `usable` (true only for a consumable the player can use from the
// bag — money / umbrella / quest papers are view-only). `itemId` is
// carried so the controller can map the highlighted row back to a
// consumable id for ApplyConsumableEffect + ConsumeOne without re-deriving
// it from the display name. Mirrors EndingSummary: a flat struct of
// primitives, no World/Player handles, no gameplay logic — InventoryView
// renders it and nothing else.
struct InventoryRow {
    std::string name;          // 中文 display name
    int         count = 0;     // >0 shows "xN"; 0 = single instance, no suffix
    std::string description;   // one short function line
    bool        usable = false; // E/Enter applies an effect?
    std::string itemId;        // engine id (consumable key) for use-from-bag
};

// Item 2(c) — assemble the full bag from the Player: 金幣 (balance, noted
// as cross-chapter), every held consumable (count + what it does), the
// carried umbrella (derived from the Flag_* the player holds), and the
// current-cycle quest papers found (申請書 / Ch2 三頁筆記). Empty
// categories are simply omitted, so an empty bag yields an empty vector
// (the View paints its （空） placeholder). Built in the quest layer —
// it reads Player flags + the count map (read-only) and the catalog; the
// View never does. Deterministic order: 金幣, then consumables sorted by
// itemId (stable like the old DrawInventory), then umbrella, then quest
// papers — so the panel and the regression test are stable.
[[nodiscard]] std::vector<InventoryRow> BuildInventoryRows(const Player& player);

}  // namespace nccu

#endif  // ITEM_CATALOG_H_
