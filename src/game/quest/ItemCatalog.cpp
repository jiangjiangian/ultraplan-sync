#include "game/quest/ItemCatalog.h"
#include "game/entities/Player.h"
#include "game/entities/EnergyDrink.h"
#include "game/entities/HotPack.h"
#include "game/entities/WaterproofSpray.h"
#include "game/quest/Chapter1Quest.h"
#include "game/quest/Chapter2Quest.h"
#include "game/quest/Chapter3Quest.h"
#include "engine/events/EventBus.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace nccu {

namespace {

// 每個 itemId 一筆圖鑑列。僅繁體中文。消耗品列描述的效果與其
// ConsumableItem::Consume 主體所套用者「相同」；雨傘／金幣／任務紙張列則描述物品欄
// DTO 由旗標合成出的攜帶物。設為 static，使指向其中的 string_view 在整個程式生命期
// 都保持有效。
const std::unordered_map<std::string, ItemInfo>& Table() {
    static const std::unordered_map<std::string, ItemInfo> kTable = {
        // ---- 貨幣 --------------------------------------------------
        // 載明企劃要求保留的經濟規則（金錢跨章節保留；消耗品為章節限定，進入市集時
        // 清除——見 SceneRouter 的 ClearConsumables）。
        {kItemMoney,
         {"金幣", "錢包餘額，跨章節保留；可在市集與便利商店消費（消耗品則每章用完）。"}},

        // ---- 消耗品（可從背包使用） -------------------------
        // 每一列載明其「精確」效果（從背包使用的路徑會套用它；描述與
        // ApplyConsumableEffect 保持同步）。
        {"EnergyDrink",
         {"能量飲料", "使用：業力 +3、雨量 −15；也能用來喚醒攤倒的學霸。"}},
        {"HotPack",
         {"暖暖包", "使用：雨量 −25（烘乾大半雨水）、業力 +5。"}},
        {"WaterproofSpray",
         {"防水噴霧", "使用：雨量 −35（彈開大半雨水）；專門擋雨，不影響業力。"}},

        // ---- 市集小吃／風味購買（持有於背包） --------------
        // 可食用的小吃現在「可使用」（每次使用減 15 雨量、無業力）。愛心捐款維持為只供
        // 檢視的持有贈品（無降雨效果）。它們與任何購買一樣進入背包（Vendor::TryBuy →
        // AddConsumable）；圖鑑給它們一個中文名稱，使購買提示與背包列都不會印出原始的
        // 英文 id。
        {"EggCake",   {"雞蛋糕", "使用：雨量 −15。古早味雞蛋糕，暖手暖胃。"}},
        {"FlowerTea", {"花茶", "使用：雨量 −15。茶藝社的熱花茶，雨天裡的暖意。"}},
        {"Takoyaki",  {"章魚燒", "使用：雨量 −15。三色章魚燒，校慶味道的小點心。"}},
        {"Donation",  {"愛心捐款", "捐給學生會的善款，幫弱勢同學繳活動費（持有用）。"}},

        // ---- 攤販販售的雨傘（提示／背包使用這些 itemId；由旗標衍生的背包列則用上方的
        // kItem* 哨兵值） ------
        {"UglyUmbrella",
         {"螢光綠醜傘", "醜得全校最好認，但保證沒人拿錯。務實的選擇。"}},
        {"CursedUmbrella",
         {"詛咒傘", "傘骨上刻著別人的名字。拿了它，雨會一直跟著你。"}},
        {"TransparentUmbrella",
         {"透明傘", "再普通不過的透明傘——但它可能是別人的。"}},

        // ---- 雨傘（只供檢視；由旗標衍生） -----------------
        // 釐清雨傘的雨勢緩解在持有時「自動」生效（撐著 → 雨量緩升而非急升，
        // ApplyRainSheltered）——不像上方的消耗品需要「使用」。
        {kItemTrueUmbrella,
         {"真傘", "撐著它，雨量上升會大幅減緩（自動生效）。完美結局的關鍵。"}},
        {kItemCursedUmbrella,
         {"詛咒傘", "傘骨上刻著別人的名字。拿了它，雨會一直跟著你。"}},
        {kItemUglyUmbrella,
         {"螢光綠醜傘", "醜得全校最好認，但保證沒人拿錯。務實的選擇。"}},
        {kItemVictimUmbrella,
         {"苦主的傘", "撿到的透明傘，傘主在綜院等它回家。記得還給他。"}},
        // 背包能顯示的其餘「拿在頭頂」型雨傘。（描述只用 docs/content 中已存在的字形，
        // 使無頭的字形掃描關卡維持綠燈——見 test_font_ui_glyph_scan。）
        {kItemFragileUmbrella,
         {"破傘", "骨架斷了的傘，雨還是會慢慢滲進來；有總比沒有好。"}},
        {kItemProfTrapUmbrella,
         {"陷阱傘", "傘骨上有奇怪的刻字，撐著它總覺得有人在後面跟著。"}},
        // 第二章圖書館管理員的借傘。具實際遮蔽功能（持有時自動緩解降雨），但明確「非」
        // 真傘（不通往結局 A）。
        {kItemLoanerUmbrella,
         {"管理員的傘", "圖書館管理員借你的傘，撐著它雨量會自動減緩；記得歸還。"}},

        // ---- 任務紙張（只供檢視；由旗標衍生） --------------
        {kItemForm,
         {"申請書",
          "撿到的加退選申請書，還不知道該交給哪位助教，先收著吧。"}},
        {kItemNotes,
         {"學霸的筆記", "替學霸撿回的筆記，散落校園的三頁心血。"}},

        // ---- Ch3 物物交換鏈攜帶物（只供檢視；由旗標衍生） --
        // 顯示在背包中，使交換鏈可見；每一個在被換成下一個的當下即被消費（清除其旗標）。
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

// 通用小吃的雨勢緩解（-15）。市集食物購買沒有實體類別（它們經 Vendor::TryBuy 以裸
// itemId 購得），故其效果只存在於此。愛心捐款（Donation）刻意「不」在此集合中——它是
// 慈善贈品而非食物，故無降雨效果，維持為只供檢視的持有物。
namespace {
constexpr float kFoodRainRelief = 15.0f;
bool IsGenericFood(std::string_view itemId) {
    return itemId == "EggCake" || itemId == "FlowerTea" ||
           itemId == "Takoyaki";
}
}  // namespace

bool IsUsableConsumable(std::string_view itemId) {
    // 三種裝備／飲料消耗品，「加上」可食用的小吃，皆可從背包使用（每次使用減雨）。
    // 愛心捐款／金錢／雨傘／任務紙張維持只供檢視。
    return itemId == "EnergyDrink" || itemId == "HotPack" ||
           itemId == "WaterproofSpray" || IsGenericFood(itemId);
}

void ApplyConsumableEffect(EventBus& bus, Player& player, std::string_view itemId) {
    // 與每個 ConsumableItem::Consume 主體「完全」一致地鏡像（相同的業力增減、相同的
    // ShowMessage 文字），只是現在改由玩家從背包觸發、而非拾取時觸發。常數仍以實體主體為
    // 來源（EnergyDrink::kKarmaBonus / HotPack::kKarmaBonus），使兩條路徑對數值絕不會
    // 不一致。字串與實體 .cpp 逐字保持同步（有 doctest 同時固定兩者）。
    if (itemId == "EnergyDrink") {
        // +3 業力「且」-15 雨量（鏡像 EnergyDrink::Consume）。
        player.AddKarma(EnergyDrink::kKarmaBonus)
              .DrainRainBy(EnergyDrink::kRainRelief);
        bus.Publish(Event{
            EventType::ShowMessage,
            "喝完飲料，精神好多了，淋到的雨也擦乾了一些。"});
    } else if (itemId == "HotPack") {
        // +5 業力「且」-25 雨量（鏡像 HotPack::Consume；原本是整個 resetRainMeter()）。
        player.AddKarma(HotPack::kKarmaBonus).DrainRainBy(HotPack::kRainRelief);
        bus.Publish(Event{
            EventType::ShowMessage,
            "用了暖暖包，烘乾了大半的雨水，心情也好了一些。"});
    } else if (itemId == "WaterproofSpray") {
        // -35 雨量、無業力（鏡像 WaterproofSpray::Consume）。
        player.DrainRainBy(WaterproofSpray::kRainRelief);
        bus.Publish(Event{
            EventType::ShowMessage,
            "噴了防水噴霧，雨水大半都被彈開了。"});
    } else if (IsGenericFood(itemId)) {
        // 通用小吃減 15 雨量（無業力、無實體類別——這是這些市集購買唯一帶有效果之處）。
        player.DrainRainBy(kFoodRainRelief);
        bus.Publish(Event{
            EventType::ShowMessage,
            "吃了點熱的，身子暖了，雨水也擦掉了一些。"});
    }
    // 未知／不可用的 id：空操作（呼叫端已以 IsUsableConsumable 閘控，但保持防禦性使此處
    // 仍可安全呼叫）。
}

namespace {

// 附加一個由圖鑑描述的列。count>0 印出「xN」；count==0 是單一實例（雨傘／任務紙張），
// 無後綴。
void PushRow(std::vector<InventoryRow>& rows, std::string_view itemId,
             int count, bool usable) {
    const ItemInfo info = ItemInfoFor(itemId);
    rows.push_back(InventoryRow{
        std::string(info.displayName), count,
        std::string(info.description), usable, std::string(itemId)});
}

// 某個持有型雨傘種類對應的圖鑑哨兵值。None ⇒ 無此列。
const char* HeldUmbrellaItem(HeldUmbrella kind) {
    switch (kind) {
        case HeldUmbrella::True:          return kItemTrueUmbrella;
        case HeldUmbrella::Cursed:        return kItemCursedUmbrella;
        case HeldUmbrella::Ugly:          return kItemUglyUmbrella;
        case HeldUmbrella::Fragile:       return kItemFragileUmbrella;
        case HeldUmbrella::ProfessorTrap: return kItemProfTrapUmbrella;
        // 苦主攜帶的傘經其任務旗標顯示，而非持有種類（它不提供遮蔽），故無持有種類列。
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
    // 攤販庫存的雨傘 id（ItemCatalog 的 Table）→ 持有種類。今日實際販售的只有集英樓的
    // 醜傘；其餘為穩健起見一併對應，使任何未來的雨傘庫存品項都能落為持有型雨傘。
    if (itemId == "UglyUmbrella")        return HeldUmbrella::Ugly;
    if (itemId == "CursedUmbrella")      return HeldUmbrella::Cursed;
    if (itemId == "TransparentUmbrella") return HeldUmbrella::True;
    return HeldUmbrella::None;
}

// 任何雨傘類 itemId 都是「拿在頭頂」的傘，由「持有種類」那一列呈現——並非計數型
// 消耗品。故 BuildInventoryRows 會把它「排除」於計數消耗品迴圈之外（第四章集英樓的
// 攤販會把 "UglyUmbrella" 加進計數表；若無此過濾，背包會畫出「兩」列雨傘——一列來自
// 計數、一列來自持有種類），InventoryView 則用它把某列歸類為雨傘種類。苦主身上帶的
// 透明傘是一個「旗標」、而非計數項目，故不受影響。以子字串比對使其對英文攤販 id
//（"UglyUmbrella"/"CursedUmbrella"/"TransparentUmbrella"）與任何未來的雨傘庫存品項
// 皆穩健適用。將其自匿名命名空間提出，使 InventoryView 共用這唯一定義，而非各自重寫。
bool IsUmbrellaItemId(std::string_view id) {
    return id.find("Umbrella") != std::string_view::npos ||
           id.find("umbrella") != std::string_view::npos;
}

std::vector<InventoryRow> BuildInventoryRows(const Player& player) {
    std::vector<InventoryRow> rows;

    // 金幣——永遠存在；跨章節保留（圖鑑文案有註明）。count 帶有餘額，使 View 能顯示
    //「金幣 x123」。
    PushRow(rows, kItemMoney, player.GetMoney(), /*usable=*/false);

    // 持有的消耗品——依 itemId 排序以取得具決定性的面板（計數表的迭代順序未指定）。每個
    // 都可從背包使用（E/Enter 套用其效果）。此處略過雨傘 itemId——持有型雨傘由下方的持有
    // 種類列顯示，絕不作為計數項目（第四章集英樓攤販會把 "UglyUmbrella" 加進此表）。
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

    // 玩家「撐在頭頂」的那把傘——以 HeldUmbrellaKind()（即時的持有雨傘狀態）為鍵，「非」
    // 持久的結局旗標。如此一來，失去的傘（第四章進入時 SetHasUmbrella(false)／某章「傘又
    // 掉了」的重置會清除持有種類）便「不」顯示雨傘列，而詛咒／醜傘的結局旗標——那些「絕不」
    // 清除——也不再於傘消失後留下過時的列。只供檢視的單一實例。（None / Victim ⇒ 無持有
    // 種類列；苦主攜帶的傘由其下方的任務旗標顯示，因它不提供遮蔽，是「要歸還的東西」、
    // 而非「你撐著的東西」。）
    if (const char* held = HeldUmbrellaCatalogId(player.HeldUmbrellaKind()))
        PushRow(rows, held, 0, /*usable=*/false);

    // 苦主的透明傘，在「為歸還而攜帶」期間（第一章，授予之前）。它是玩家運送回去的任務
    // 物品，並非撐在自己頭頂（它不提供遮蔽——hasUmbrella_ 維持 false），故它是與任何持有
    // 遮蔽傘並列的「獨立」列，與任務紙張一樣由旗標驅動。歸還授予會清除此旗標並設下持有型
    // 真傘，使背包乾淨地換列。
    if (player.HasFlag(kFlagHasVictimUmbrella))
        PushRow(rows, kItemVictimUmbrella, 0, /*usable=*/false);

    // Ch2 圖書館管理員的借傘——由旗標驅動的「獨立」列，與上方手持型雨傘並列而非覆蓋它。
    // 借傘提供遮蔽（lend 時 SetHasUmbrella(true)）卻不佔用 heldUmbrella_ 槽，故換回的真傘
    //（HeldUmbrella::True 顯示於上方手持列）與借傘能在插曲段同時列出。歸還
    //（TryReturnLibrarianUmbrella）或進入下一章（SceneRouter 清旗標）後本列消失。
    if (player.HasFlag(kFlagLibrarianUmbrella))
        PushRow(rows, kItemLoanerUmbrella, 0, /*usable=*/false);

    // 玩家在本輪已找到的任務紙張（只供檢視）。
    if (player.HasFlag(kFlagFoundForm))
        PushRow(rows, kItemForm, 0, /*usable=*/false);
    {
        // 第二章的三頁筆記合併成「一」列「學霸的筆記 xN」，計入玩家目前持有 3 頁中的幾頁
        // ——比三列相同的列更清楚。一頁都沒持有時則完全略去。
        int notes = 0;
        if (player.HasFlag(kFlagFoundNote1)) ++notes;
        if (player.HasFlag(kFlagFoundNote2)) ++notes;
        if (player.HasFlag(kFlagFoundNote3)) ++notes;
        if (notes > 0) PushRow(rows, kItemNotes, notes, /*usable=*/false);
    }

    // 第三章物物交換鏈的攜帶物，與紙張一樣由旗標驅動。每一個在交易消費它的當下即消失
    //（Chapter3Quest 交出大聲公時清除 Flag_HasSausage，交出情報時清除 Flag_HasLoudspeaker），
    // 故背包對手上持有之物始終誠實。情報（Flag_KnowsUmbrellaLoc）是知識、而非攜帶物。
    if (player.HasFlag(kFlagHasSausage))
        PushRow(rows, kItemSausage, 0, /*usable=*/false);
    if (player.HasFlag(kFlagHasLoudspeaker))
        PushRow(rows, kItemLoudspeaker, 0, /*usable=*/false);

    return rows;
}

}  // namespace nccu
