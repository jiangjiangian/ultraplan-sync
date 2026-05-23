#include "quest/ItemCatalog.h"
#include "entities/Player.h"
#include "entities/EnergyDrink.h"
#include "entities/HotPack.h"
#include "quest/Chapter1Quest.h"
#include "quest/Chapter2Quest.h"
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
        {kItemMoney,
         {"金幣", "你的錢包餘額，跨章節保留；可在市集與便利商店消費。"}},

        // ---- consumables (usable from the bag) -------------------------
        {"EnergyDrink",
         {"能量飲料", "喝下提神，業力小幅上升；也能用來喚醒攤倒的學霸。"}},
        {"HotPack",
         {"暖暖包", "立刻烘乾全身雨水（雨量歸零），心情也變好（業力上升）。"}},
        {"WaterproofSpray",
         {"防水噴霧", "噴一下，眼前這場雨就先不淋你了。"}},

        // ---- market food / flavour buys (held; no special effect) ------
        // These land in the bag like any purchase (Vendor::TryBuy →
        // AddConsumable) but carry no use-effect (not in IsUsable
        // Consumable); the catalog gives them a 中文 name so neither the
        // purchase toast nor the bag row prints a raw English id.
        {"EggCake",   {"雞蛋糕", "古早味雞蛋糕，暖手暖胃的小確幸。"}},
        {"FlowerTea", {"花茶", "茶藝社的熱花茶，雨天裡的一點暖意。"}},
        {"Takoyaki",  {"章魚燒", "三色章魚燒，校慶味道的小點心。"}},
        {"Donation",  {"愛心捐款", "捐給學生會的善款，幫弱勢同學繳活動費。"}},

        // ---- vendor-sold umbrellas (the toast / bag use these itemIds;
        // the flag-derived bag rows use the kItem* sentinels above) ------
        {"UglyUmbrella",
         {"螢光綠醜傘", "醜得全校最好認，但保證沒人拿錯。務實的選擇。"}},
        {"CursedUmbrella",
         {"詛咒傘", "傘骨上刻著別人的名字。拿了它，雨會一直跟著你。"}},
        {"TransparentUmbrella",
         {"透明傘", "再普通不過的透明傘——但它可能是別人的。"}},

        // ---- umbrellas (view-only; derived from flags) -----------------
        {kItemTrueUmbrella,
         {"真傘", "你還回去、又回到手上的那把傘。完美結局的關鍵。"}},
        {kItemCursedUmbrella,
         {"詛咒傘", "傘骨上刻著別人的名字。拿了它，雨會一直跟著你。"}},
        {kItemUglyUmbrella,
         {"螢光綠醜傘", "醜得全校最好認，但保證沒人拿錯。務實的選擇。"}},
        {kItemVictimUmbrella,
         {"苦主的傘", "撿到的透明傘，傘主在綜院等它回家。記得還給他。"}},

        // ---- quest papers (view-only; derived from flags) --------------
        {kItemForm,
         {"申請書", "被風吹走的加退選申請書，撿回來交給助教。"}},
        {kItemNotes,
         {"學霸的筆記", "替學霸撿回的筆記，散落校園的三頁心血。"}},
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

bool IsUsableConsumable(std::string_view itemId) {
    return itemId == "EnergyDrink" || itemId == "HotPack" ||
           itemId == "WaterproofSpray";
}

void ApplyConsumableEffect(Player& player, std::string_view itemId) {
    // Mirror each ConsumableItem::Consume body EXACTLY (same karma delta,
    // same ShowMessage text), now player-triggered from the bag instead of
    // fired on pickup. The entity bodies stay the source of the constants
    // (EnergyDrink::kKarmaBonus / HotPack::kKarmaBonus) so the two paths
    // can never disagree on the number. Strings are kept literally in sync
    // with the entity .cpp (a doctest pins both).
    if (itemId == "EnergyDrink") {
        player.AddKarma(EnergyDrink::kKarmaBonus);
        EventBus::Instance().Publish(Event{
            EventType::ShowMessage,
            "喝完飲料，精神好多了。下次小考應該能撐住。"});
    } else if (itemId == "HotPack") {
        player.AddKarma(HotPack::kKarmaBonus).resetRainMeter();
        EventBus::Instance().Publish(Event{
            EventType::ShowMessage,
            "用了暖暖包，雨水蒸發了，心情也好了一些。"});
    } else if (itemId == "WaterproofSpray") {
        // Mood-only, matching WaterproofSpray::Consume (no karma).
        EventBus::Instance().Publish(Event{
            EventType::ShowMessage,
            "噴了防水噴霧，接下來這場雨就無感了。"});
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

}  // namespace

std::vector<InventoryRow> BuildInventoryRows(const Player& player) {
    std::vector<InventoryRow> rows;

    // 金幣 — always present; persists across chapters (catalog copy notes
    // it). Count carries the balance so the View can show "金幣 x123".
    PushRow(rows, kItemMoney, player.GetMoney(), /*usable=*/false);

    // Held consumables — sorted by itemId for a deterministic panel (the
    // count map iteration order is unspecified). Each is usable from the
    // bag (E/Enter applies its effect).
    {
        std::vector<std::pair<std::string, int>> held;
        held.reserve(player.Consumables().size());
        for (const auto& kv : player.Consumables())
            if (kv.second > 0) held.emplace_back(kv.first, kv.second);
        std::sort(held.begin(), held.end(),
                  [](const auto& a, const auto& b) { return a.first < b.first; });
        for (const auto& [id, n] : held)
            PushRow(rows, id, n, /*usable=*/IsUsableConsumable(id));
    }

    // The carried umbrella — derived from the Flag_* the player holds, in
    // priority order. These flags are mutually meaningful end-states; if
    // more than one is somehow set, the "best" outcome (true umbrella)
    // wins the single row. Each is a view-only single instance.
    if (player.HasFlag("Flag_HasTrueUmbrella"))
        PushRow(rows, kItemTrueUmbrella, 0, /*usable=*/false);
    else if (player.HasFlag("Flag_TookCursedUmbrella"))
        PushRow(rows, kItemCursedUmbrella, 0, /*usable=*/false);
    else if (player.HasFlag("Flag_BoughtUglyUmbrella"))
        PushRow(rows, kItemUglyUmbrella, 0, /*usable=*/false);
    else if (player.HasFlag(kFlagHasVictimUmbrella))
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

    return rows;
}

}  // namespace nccu
