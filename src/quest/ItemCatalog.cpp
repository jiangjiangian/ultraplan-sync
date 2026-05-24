#include "quest/ItemCatalog.h"
#include "entities/Player.h"
#include "entities/EnergyDrink.h"
#include "entities/HotPack.h"
#include "entities/WaterproofSpray.h"
#include "quest/Chapter1Quest.h"
#include "quest/Chapter2Quest.h"
#include "quest/Chapter3Quest.h"
#include "controller/EventBus.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace nccu {

namespace {

// One catalog row per itemId. zh-TW only (forbidden-string rule). The
// consumable rows describe the SAME effect their ConsumableItem::Consume
// body applies; the umbrella / 金幣 / 任務紙張 rows describe the carried
// objects the inventory DTO synthesises from flags. Static so the
// string_views into it stay valid for the program lifetime.
const std::unordered_map<std::string, ItemInfo>& Table() {
    static const std::unordered_map<std::string, ItemInfo> kTable = {
        // ---- currency --------------------------------------------------
        // G4: state the economy rules the owner asked to KEEP (money is
        // cross-chapter; consumables are chapter-scoped, wiped on market
        // entry — see SceneRouter ClearConsumables).
        {kItemMoney,
         {"金幣", "錢包餘額，跨章節保留；可在市集與便利商店消費（消耗品則每章用完）。"}},

        // ---- consumables (usable from the bag) -------------------------
        // G4: each row states its EXACT effect (the use-from-bag path
        // applies it; descriptions kept in sync with ApplyConsumableEffect).
        {"EnergyDrink",
         {"能量飲料", "使用：業力 +3、雨量 −15；也能用來喚醒攤倒的學霸。"}},
        {"HotPack",
         {"暖暖包", "使用：雨量 −25（烘乾大半雨水）、業力 +5。"}},
        {"WaterproofSpray",
         {"防水噴霧", "使用：雨量 −35（彈開大半雨水）；專門擋雨，不影響業力。"}},

        // ---- market food / flavour buys (held in the bag) --------------
        // G4: the edible 小吃 are now USABLE (each sheds −15 雨量 on use,
        // no karma). 愛心捐款 stays a view-only held gift (no rain effect).
        // They land in the bag like any purchase (Vendor::TryBuy →
        // AddConsumable); the catalog gives them a 中文 name so neither the
        // purchase toast nor the bag row prints a raw English id.
        {"EggCake",   {"雞蛋糕", "使用：雨量 −15。古早味雞蛋糕，暖手暖胃。"}},
        {"FlowerTea", {"花茶", "使用：雨量 −15。茶藝社的熱花茶，雨天裡的暖意。"}},
        {"Takoyaki",  {"章魚燒", "使用：雨量 −15。三色章魚燒，校慶味道的小點心。"}},
        {"Donation",  {"愛心捐款", "捐給學生會的善款，幫弱勢同學繳活動費（持有用）。"}},

        // ---- vendor-sold umbrellas (the toast / bag use these itemIds;
        // the flag-derived bag rows use the kItem* sentinels above) ------
        {"UglyUmbrella",
         {"螢光綠醜傘", "醜得全校最好認，但保證沒人拿錯。務實的選擇。"}},
        {"CursedUmbrella",
         {"詛咒傘", "傘骨上刻著別人的名字。拿了它，雨會一直跟著你。"}},
        {"TransparentUmbrella",
         {"透明傘", "再普通不過的透明傘——但它可能是別人的。"}},

        // ---- umbrellas (view-only; derived from flags) -----------------
        // G4: clarify the umbrella's rain relief is AUTOMATIC while held
        // (撐著 → 雨量 緩升而非急升, ApplyRainSheltered) — no "use" needed,
        // unlike the消耗品 above.
        {kItemTrueUmbrella,
         {"真傘", "撐著它，雨量上升會大幅減緩（自動生效）。完美結局的關鍵。"}},
        {kItemCursedUmbrella,
         {"詛咒傘", "傘骨上刻著別人的名字。拿了它，雨會一直跟著你。"}},
        {kItemUglyUmbrella,
         {"螢光綠醜傘", "醜得全校最好認，但保證沒人拿錯。務實的選擇。"}},
        {kItemVictimUmbrella,
         {"苦主的傘", "撿到的透明傘，傘主在綜院等它回家。記得還給他。"}},
        // B2.1: the remaining held-over-head umbrellas the bag can show.
        // (Descriptions use only glyphs already in docs/content so the
        // headless font-glyph-scan gate stays green — see test_font_ui_glyph_scan.)
        {kItemFragileUmbrella,
         {"破傘", "骨架斷了的傘，雨還是會慢慢滲進來；有總比沒有好。"}},
        {kItemProfTrapUmbrella,
         {"陷阱傘", "傘骨上有奇怪的刻字，撐著它總覺得有人在後面跟著。"}},
        // B2.3: the Ch2 圖書館管理員 loaner. Functional shelter (auto-rains
        // while held) but explicitly NOT the true umbrella (no Ending A).
        {kItemLoanerUmbrella,
         {"管理員的傘", "圖書館管理員借你的傘，撐著它雨量會自動減緩；記得歸還。"}},

        // ---- quest papers (view-only; derived from flags) --------------
        {kItemForm,
         {"申請書",
          "撿到的加退選申請書，還不知道該交給哪位助教，先收著吧。"}},
        {kItemNotes,
         {"學霸的筆記", "替學霸撿回的筆記，散落校園的三頁心血。"}},

        // ---- Ch3 物物交換鏈 carried items (view-only; derived from flags) --
        // B2.4: shown in the bag so the trade chain is visible; each is
        // consumed (its flag cleared) the moment it is traded for the next.
        {kItemSausage,
         {"香腸", "A 系攤主給的烤香腸，熱的、燙手；拿去找 B 系同學換東西。"}},
        {kItemLoudspeaker,
         {"大聲公", "B 系同學換來的大聲公，拿去找 C 系學姊換情報。"}},
    };
    return kTable;
}

}  // namespace

ItemInfo ItemInfoFor(std::string_view itemId) {
    const auto& t = Table();
    auto it = t.find(std::string(itemId));
    if (it != t.end()) return it->second;
    return ItemInfo{itemId, std::string_view{}};
}

std::vector<std::string> CatalogStrings() {
    std::vector<std::string> out;
    for (const auto& [id, info] : Table()) {
        (void)id;
        out.emplace_back(info.displayName);
        if (!info.description.empty()) out.emplace_back(info.description);
    }
    return out;
}

// G4: generic 小吃 rain relief (-15). The market food buys have no entity
// class (they are bought as bare itemIds via Vendor::TryBuy), so their
// effect lives only here. 愛心捐款 (Donation) is deliberately NOT in this
// set — it is a charity gift, not food, so it has no rain effect and stays
// a view-only held item.
namespace {
constexpr float kFoodRainRelief = 15.0f;
bool IsGenericFood(std::string_view itemId) {
    return itemId == "EggCake" || itemId == "FlowerTea" ||
           itemId == "Takoyaki";
}
}  // namespace

bool IsUsableConsumable(std::string_view itemId) {
    // G4: the three gear/drink consumables PLUS the edible 小吃 are usable
    // from the bag (each sheds rain on use). Donation / money / umbrellas /
    // quest papers remain view-only.
    return itemId == "EnergyDrink" || itemId == "HotPack" ||
           itemId == "WaterproofSpray" || IsGenericFood(itemId);
}

void ApplyConsumableEffect(Player& player, std::string_view itemId) {
    // Mirror each ConsumableItem::Consume body EXACTLY (same karma delta,
    // same ShowMessage text), now player-triggered from the bag instead of
    // fired on pickup. The entity bodies stay the source of the constants
    // (EnergyDrink::kKarmaBonus / HotPack::kKarmaBonus) so the two paths
    // can never disagree on the number. Strings are kept literally in sync
    // with the entity .cpp (a doctest pins both).
    if (itemId == "EnergyDrink") {
        // G4: +3 karma AND -15 rain (mirrors EnergyDrink::Consume).
        player.AddKarma(EnergyDrink::kKarmaBonus)
              .DrainRainBy(EnergyDrink::kRainRelief);
        EventBus::Instance().Publish(Event{
            EventType::ShowMessage,
            "喝完飲料，精神好多了，淋到的雨也擦乾了一些。"});
    } else if (itemId == "HotPack") {
        // G4: +5 karma AND -25 rain (mirrors HotPack::Consume; was a full
        // resetRainMeter()).
        player.AddKarma(HotPack::kKarmaBonus).DrainRainBy(HotPack::kRainRelief);
        EventBus::Instance().Publish(Event{
            EventType::ShowMessage,
            "用了暖暖包，烘乾了大半的雨水，心情也好了一些。"});
    } else if (itemId == "WaterproofSpray") {
        // G4: -35 rain, no karma (mirrors WaterproofSpray::Consume).
        player.DrainRainBy(WaterproofSpray::kRainRelief);
        EventBus::Instance().Publish(Event{
            EventType::ShowMessage,
            "噴了防水噴霧，雨水大半都被彈開了。"});
    } else if (IsGenericFood(itemId)) {
        // G4: a generic 小吃 dries off -15 rain (no karma, no entity class —
        // this is the only place these market buys carry an effect).
        player.DrainRainBy(kFoodRainRelief);
        EventBus::Instance().Publish(Event{
            EventType::ShowMessage,
            "吃了點熱的，身子暖了，雨水也擦掉了一些。"});
    }
    // Unknown / non-usable id: no-op (the caller already gated on
    // IsUsableConsumable, but staying defensive keeps this safe to call).
}

namespace {

// Append a catalog-described row. count>0 prints "xN"; count==0 is a
// single instance (umbrella / quest paper) with no suffix.
void PushRow(std::vector<InventoryRow>& rows, std::string_view itemId,
             int count, bool usable) {
    const ItemInfo info = ItemInfoFor(itemId);
    rows.push_back(InventoryRow{
        std::string(info.displayName), count,
        std::string(info.description), usable, std::string(itemId)});
}

// B2.1: any umbrella itemId is a HELD-over-head umbrella, surfaced by the
// HeldUmbrellaKind() row below — NOT a count-consumable. So it must be
// EXCLUDED from the consumable-count loop (the Ch4 集英樓 vendor adds
// "UglyUmbrella" to the count map; without this filter the bag drew TWO
// umbrella rows — one from the count, one from the held-kind). The 苦主's
// carried transparent umbrella is a FLAG, not a count entry, so it is
// unaffected here. Substring match keeps it robust to both the English
// vendor ids ("UglyUmbrella"/"CursedUmbrella"/"TransparentUmbrella") and any
// future umbrella stock line.
bool IsUmbrellaItemId(const std::string& id) {
    return id.find("Umbrella") != std::string::npos ||
           id.find("umbrella") != std::string::npos;
}

// B2.1: the catalog sentinel for a held umbrella kind. None ⇒ no row.
const char* HeldUmbrellaItem(HeldUmbrella kind) {
    switch (kind) {
        case HeldUmbrella::True:          return kItemTrueUmbrella;
        case HeldUmbrella::Cursed:        return kItemCursedUmbrella;
        case HeldUmbrella::Ugly:          return kItemUglyUmbrella;
        case HeldUmbrella::Fragile:       return kItemFragileUmbrella;
        case HeldUmbrella::ProfessorTrap: return kItemProfTrapUmbrella;
        case HeldUmbrella::Loaner:        return kItemLoanerUmbrella;
        // Victim's carried umbrella is shown via its quest flag, not the
        // held-kind (it grants no shelter), so it has no held-kind row.
        case HeldUmbrella::Victim:
        case HeldUmbrella::None:          return nullptr;
    }
    return nullptr;
}

}  // namespace

const char* HeldUmbrellaCatalogId(HeldUmbrella kind) {
    return HeldUmbrellaItem(kind);
}

HeldUmbrella HeldUmbrellaForItemId(std::string_view itemId) {
    // The vendor-stock umbrella ids (ItemCatalog Table) → the held kind.
    // Only the 集英樓 醜傘 is actually sold today; the others are mapped for
    // robustness so any future umbrella stock line lands as a held umbrella.
    if (itemId == "UglyUmbrella")        return HeldUmbrella::Ugly;
    if (itemId == "CursedUmbrella")      return HeldUmbrella::Cursed;
    if (itemId == "TransparentUmbrella") return HeldUmbrella::True;
    return HeldUmbrella::None;
}

std::vector<InventoryRow> BuildInventoryRows(const Player& player) {
    std::vector<InventoryRow> rows;

    // 金幣 — always present; persists across chapters (catalog copy notes
    // it). Count carries the balance so the View can show "金幣 x123".
    PushRow(rows, kItemMoney, player.GetMoney(), /*usable=*/false);

    // Held consumables — sorted by itemId for a deterministic panel (the
    // count map iteration order is unspecified). Each is usable from the
    // bag (E/Enter applies its effect). B2.1: umbrella itemIds are skipped
    // here — a held umbrella is shown by the held-kind row below, never as a
    // count entry (the Ch4 集英樓 vendor adds "UglyUmbrella" to this map).
    {
        std::vector<std::pair<std::string, int>> held;
        held.reserve(player.Consumables().size());
        for (const auto& kv : player.Consumables())
            if (kv.second > 0 && !IsUmbrellaItemId(kv.first))
                held.emplace_back(kv.first, kv.second);
        std::sort(held.begin(), held.end(),
                  [](const auto& a, const auto& b) { return a.first < b.first; });
        for (const auto& [id, n] : held)
            PushRow(rows, id, n, /*usable=*/IsUsableConsumable(id));
    }

    // B2.1: the umbrella the player is HOLDING OVER THEIR HEAD — keyed off
    // HeldUmbrellaKind(), the live held-umbrella state, NOT the persistent
    // ending flags. So a lost umbrella (SetHasUmbrella(false) on Ch4 entry /
    // a per-chapter「傘又掉了」reset clears the held kind) shows NO umbrella
    // row, and a cursed / ugly ending flag — which is NEVER cleared — no
    // longer leaves a stale row after the umbrella is gone. View-only single
    // instance. (None / Victim ⇒ no held-kind row; the 苦主's carried
    // umbrella is shown by its quest flag just below, as it grants no
    // shelter and is a thing-to-return, not a thing-you-hold.)
    if (const char* held = HeldUmbrellaCatalogId(player.HeldUmbrellaKind()))
        PushRow(rows, held, 0, /*usable=*/false);

    // The 苦主's transparent umbrella, while CARRIED for return (Ch1, before
    // the grant). It is a quest item the player ferries back, not held over
    // their own head (it grants no shelter — hasUmbrella_ stays false), so it
    // is an INDEPENDENT row alongside any held shelter umbrella, flag-driven
    // like the quest papers. The return grant clears this flag and sets the
    // held TRUE umbrella, so the bag swaps rows cleanly.
    if (player.HasFlag(kFlagHasVictimUmbrella))
        PushRow(rows, kItemVictimUmbrella, 0, /*usable=*/false);

    // Current-cycle quest papers the player has found (view-only).
    if (player.HasFlag("Flag_FoundForm"))
        PushRow(rows, kItemForm, 0, /*usable=*/false);
    {
        // The Ch2 三頁筆記 collapse into ONE "學霸的筆記 xN" row counting
        // how many of the 3 the player currently holds — clearer than
        // three identical rows. Omitted entirely when none are held.
        int notes = 0;
        if (player.HasFlag(kFlagFoundNote1)) ++notes;
        if (player.HasFlag(kFlagFoundNote2)) ++notes;
        if (player.HasFlag(kFlagFoundNote3)) ++notes;
        if (notes > 0) PushRow(rows, kItemNotes, notes, /*usable=*/false);
    }

    // B2.4: the Ch3 物物交換鏈 carried items, flag-driven like the papers.
    // Each disappears the instant the trade consumes it (Chapter3Quest clears
    // Flag_HasSausage when it hands over the 大聲公, and Flag_HasLoudspeaker
    // when it hands over the 情報), so the bag is always honest about what is
    // in hand. 情報 (Flag_KnowsUmbrellaLoc) is knowledge, not a carried thing.
    if (player.HasFlag(kFlagHasSausage))
        PushRow(rows, kItemSausage, 0, /*usable=*/false);
    if (player.HasFlag(kFlagHasLoudspeaker))
        PushRow(rows, kItemLoudspeaker, 0, /*usable=*/false);

    return rows;
}

}  // namespace nccu
