#include "doctest/doctest.h"
#include "game/quest/Flags.h"
#include "ui/InventoryView.h"
#include "game/quest/ItemCatalog.h"
#include "game/quest/Chapter1Quest.h"
#include "game/quest/Chapter2Quest.h"
#include "game/entities/Player.h"
#include "engine/render/IRenderer.h"
#include "game/gfx/UmbrellaGlyph.h"
#include "game/dialog/DialogLayout.h"   // CellWidth — 換行斷言用

#include <string>
#include <vector>

/**
 * @file test_inventory_view.cpp
 * @brief 驗證物品欄：DrawInventory 的面板／逐列文字／選取游標／說明面板繪製，
 *        BuildInventoryRows 彙整金幣、消耗品、手持傘與任務紙張，背包傘列依
 *        手持傘種類而非結局旗標、各分類左側色塊、分頁視窗計算與長說明換行。
 */

namespace {

// DrawInventory 是 InventoryRow 資料物件加游標索引的純函式 —— 以注入的攔截器
// 觀察，在無 GL 環境下斷言面板、逐列文字與說明面板。
struct Spy final : nccu::engine::render::IRenderer {
    int rects = 0;
    std::vector<nccu::engine::math::Color> rectColors;
    std::vector<std::string> texts;
    void DrawRect(nccu::engine::math::Rect, nccu::engine::math::Color c) override {
        ++rects; rectColors.push_back(c);
    }
    void DrawSprite(const nccu::engine::render::Texture&, nccu::engine::math::Rect,
                    nccu::engine::math::Rect, nccu::engine::math::Color) override {}
    void DrawText(std::string_view t, nccu::engine::math::Vec2, int,
                  nccu::engine::math::Color) override { texts.emplace_back(t); }
};

bool Has(const std::vector<std::string>& v, const std::string& s) {
    for (const auto& x : v) if (x == s) return true;
    return false;
}

bool HasRectRGB(const Spy& s, nccu::engine::math::Color want) {
    for (const nccu::engine::math::Color& c : s.rectColors)
        if (c.r == want.r && c.g == want.g && c.b == want.b) return true;
    return false;
}

}  // namespace

// 繪製背景、面板、標題、每列一行文字與選取游標。
TEST_CASE("DrawInventory：背景、面板、標題、每列一行與選取游標") {
    Spy r;
    std::vector<nccu::InventoryRow> rows{
        {"暖暖包", 2, "立刻烘乾", true, "HotPack"},
        {"能量飲料", 1, "喝下提神", true, "EnergyDrink"},
    };
    nccu::DrawInventory(r, rows, /*cursor=*/0, 800.0f, 450.0f);

    CHECK(r.rects >= 2);                       // 暗背景 + 面板
    CHECK(Has(r.texts, "物品欄"));             // 標題
    // 第 0 列被選取 -> 「> 」游標；第 1 列 -> 「  」留白。並顯示數量。
    CHECK(Has(r.texts, "> 暖暖包 x2"));
    CHECK(Has(r.texts, "  能量飲料 x1"));
}

// 游標會把選取符號移到被選取的列。
TEST_CASE("DrawInventory：游標把選取符號移到被選取的列") {
    Spy r;
    std::vector<nccu::InventoryRow> rows{
        {"暖暖包", 2, "立刻烘乾", true, "HotPack"},
        {"能量飲料", 1, "喝下提神", true, "EnergyDrink"},
    };
    nccu::DrawInventory(r, rows, /*cursor=*/1, 800.0f, 450.0f);
    CHECK(Has(r.texts, "  暖暖包 x2"));
    CHECK(Has(r.texts, "> 能量飲料 x1"));
}

// 會畫出被選取列的說明與使用提示。
TEST_CASE("DrawInventory：會畫出被選取列的說明與使用提示") {
    Spy r;
    std::vector<nccu::InventoryRow> rows{
        {"暖暖包", 2, "立刻烘乾全身雨水", true, "HotPack"},
        {"金幣", 100, "你的錢包餘額", false, nccu::kItemMoney},
    };
    // 游標在可用消耗品上 -> 顯示「E 使用」提示與其說明。
    nccu::DrawInventory(r, rows, /*cursor=*/0, 800.0f, 450.0f);
    CHECK(Has(r.texts, "立刻烘乾全身雨水"));
    CHECK(Has(r.texts, "↑↓ 選擇   E 使用"));

    // 游標在僅供檢視的金幣列 -> 只顯示一般提示（無 E 使用）。
    Spy r2;
    nccu::DrawInventory(r2, rows, /*cursor=*/1, 800.0f, 450.0f);
    CHECK(Has(r2.texts, "你的錢包餘額"));
    CHECK(Has(r2.texts, "↑↓ 選擇"));
    CHECK_FALSE(Has(r2.texts, "↑↓ 選擇   E 使用"));
}

// 單一持有的列（count 為 0）不顯示 xN 後綴。
TEST_CASE("DrawInventory：單一持有的列（count 為 0）不顯示 xN 後綴") {
    Spy r;
    std::vector<nccu::InventoryRow> rows{
        {"真傘", 0, "完美結局的關鍵", false, nccu::kItemTrueUmbrella},
    };
    nccu::DrawInventory(r, rows, /*cursor=*/0, 800.0f, 450.0f);
    CHECK(Has(r.texts, "> 真傘"));
    CHECK_FALSE(Has(r.texts, "> 真傘 x0"));
}

// 空背包會顯示「（空）」佔位字。
TEST_CASE("DrawInventory：空背包顯示「（空）」佔位字") {
    Spy r;
    std::vector<nccu::InventoryRow> none;
    nccu::DrawInventory(r, none, /*cursor=*/0, 800.0f, 450.0f);

    CHECK(r.rects >= 2);
    CHECK(Has(r.texts, "物品欄"));
    CHECK(Has(r.texts, "（空）"));
}

// 超出範圍的游標會被夾住，而非當機。
TEST_CASE("DrawInventory：超出範圍的游標會被夾住而非當機") {
    Spy r;
    std::vector<nccu::InventoryRow> rows{
        {"暖暖包", 1, "立刻烘乾", true, "HotPack"},
    };
    // 游標 99 必須夾到最後一列（也是唯一一列）並仍能繪製。
    nccu::DrawInventory(r, rows, /*cursor=*/99, 800.0f, 450.0f);
    CHECK(Has(r.texts, "> 暖暖包 x1"));
}

// BuildInventoryRows 會彙整玩家實際持有的每一種類別 —— 金幣 + 持有的消耗品 +
// 攜帶的傘 + 當前章節的任務紙張 —— 每項都帶有道具表名稱。
namespace {
const nccu::InventoryRow* Find(const std::vector<nccu::InventoryRow>& rows,
                               std::string_view itemId) {
    for (const auto& r : rows) if (r.itemId == itemId) return &r;
    return nullptr;
}
}  // namespace

// BuildInventoryRows 彙整金幣、消耗品、傘與任務紙張。
TEST_CASE("BuildInventoryRows 彙整金幣、消耗品、雨傘與任務道具") {
    Player p{nccu::engine::math::Vec2{0, 0}};
    // 預設 Player：只有金幣（100）、空背包、無旗標。
    {
        const auto rows = nccu::BuildInventoryRows(p);
        REQUIRE(rows.size() == 1);
        const nccu::InventoryRow* money = Find(rows, nccu::kItemMoney);
        REQUIRE(money != nullptr);
        CHECK(money->name == "金幣");
        CHECK(money->count == 100);          // 餘額存於 count
        CHECK_FALSE(money->usable);          // 金幣僅供檢視
        CHECK_FALSE(money->description.empty());
    }

    // 持有消耗品、真傘、申請書，以及三頁筆記中的兩頁。
    // 背包傘列由「手持傘種類」（SetHeldUmbrella）驅動，而非持久的結局旗標 ——
    // 此處兩者都設，如同實際發放處（旗標是結局 A 的標記；手持種類才是背包列的
    // 真正來源）。
    p.AddConsumable("HotPack").AddConsumable("HotPack");
    p.AddConsumable("EnergyDrink");
    p.SetHeldUmbrella(HeldUmbrella::True);
    p.SetFlag(nccu::kFlagHasTrueUmbrella);
    p.SetFlag(nccu::kFlagFoundForm);
    p.SetFlag(nccu::kFlagFoundNote1);
    p.SetFlag(nccu::kFlagFoundNote3);

    const auto rows = nccu::BuildInventoryRows(p);

    // 金幣排第一（順序確定），仍在。
    REQUIRE(!rows.empty());
    CHECK(rows.front().itemId == nccu::kItemMoney);

    // 消耗品連同其數量與可用旗標。
    const nccu::InventoryRow* hot = Find(rows, "HotPack");
    REQUIRE(hot != nullptr);
    CHECK(hot->name == "暖暖包");
    CHECK(hot->count == 2);
    CHECK(hot->usable);
    const nccu::InventoryRow* energy = Find(rows, "EnergyDrink");
    REQUIRE(energy != nullptr);
    CHECK(energy->name == "能量飲料");
    CHECK(energy->count == 1);
    CHECK(energy->usable);

    // 手持的傘，由 HeldUmbrellaKind() 推導。
    const nccu::InventoryRow* umb = Find(rows, nccu::kItemTrueUmbrella);
    REQUIRE(umb != nullptr);
    CHECK(umb->name == "真傘");
    CHECK(umb->count == 0);                  // 單一持有，無 xN
    CHECK_FALSE(umb->usable);

    // 任務紙張：申請書 + 三頁筆記合併成一個「xN」列。
    const nccu::InventoryRow* form = Find(rows, nccu::kItemForm);
    REQUIRE(form != nullptr);
    CHECK(form->name == "申請書");
    const nccu::InventoryRow* notes = Find(rows, nccu::kItemNotes);
    REQUIRE(notes != nullptr);
    CHECK(notes->count == 2);                // 三頁中持有兩頁
    CHECK_FALSE(notes->usable);
}

// 背包傘列反映玩家「當下手持」的傘（HeldUmbrellaKind），而非持久的結局旗標。
// 醜傘／詛咒傘的結局旗標永不清除，故若以它們為依據，傘失去後會殘留過時的列；
// 手持種類才是真正的依據。
TEST_CASE("背包傘列反映當下手持的傘而非結局旗標") {
    // 手持醜傘 → 醜傘列（即使 Flag_BoughtUglyUmbrella 是結局 C 標記；此處驅動
    // 該列的是手持種類）。
    Player p{nccu::engine::math::Vec2{0, 0}};
    p.SetHeldUmbrella(HeldUmbrella::Ugly);
    {
        const auto rows = nccu::BuildInventoryRows(p);
        CHECK(Find(rows, nccu::kItemUglyUmbrella) != nullptr);
        CHECK(Find(rows, nccu::kItemTrueUmbrella) == nullptr);
    }
    // 手持詛咒傘 → 詛咒傘列。
    Player q{nccu::engine::math::Vec2{0, 0}};
    q.SetHeldUmbrella(HeldUmbrella::Cursed);
    {
        const auto rows = nccu::BuildInventoryRows(q);
        const nccu::InventoryRow* c = Find(rows, nccu::kItemCursedUmbrella);
        REQUIRE(c != nullptr);
        CHECK(c->name == "詛咒傘");
    }
    // 攜帶的苦主之傘（任務途中、歸還發放前）是「攜帶」的任務道具（旗標驅動、
    // 不提供遮蔽）—— 仍會顯示，但它不是撐在頭上的傘，故 HasUmbrella 維持 false。
    Player v{nccu::engine::math::Vec2{0, 0}};
    v.SetFlag(nccu::kFlagHasVictimUmbrella);
    {
        const auto rows = nccu::BuildInventoryRows(v);
        CHECK(Find(rows, nccu::kItemVictimUmbrella) != nullptr);
        CHECK_FALSE(v.HasUmbrella());
    }
}

// 失去的傘必須從背包消失。結局旗標持續存在（用以決定 A/B/C），但
// SetHasUmbrella(false)（進第四章「傘再度失蹤」／各章「傘又掉了」的重置）會清除
// 手持種類，故傘列消失、不留過時列。
TEST_CASE("SetHasUmbrella(false) 移除傘列，即使結局旗標仍存在") {
    Player p{nccu::engine::math::Vec2{0, 0}};
    // 持有詛咒傘 + 持久的結局 B 標記（如 CursedUmbrella::BeClaimed 一併設定）。
    p.SetHeldUmbrella(HeldUmbrella::Cursed).SetFlag(nccu::kFlagTookCursedUmbrella);
    REQUIRE(Find(nccu::BuildInventoryRows(p), nccu::kItemCursedUmbrella) != nullptr);

    // 傘被失去（進第四章／各章重置）。
    p.SetHasUmbrella(false);
    const auto rows = nccu::BuildInventoryRows(p);
    // 任何種類的傘列都不存在，儘管結局旗標仍設著。
    CHECK(Find(rows, nccu::kItemCursedUmbrella) == nullptr);
    CHECK(Find(rows, nccu::kItemTrueUmbrella) == nullptr);
    CHECK(Find(rows, nccu::kItemUglyUmbrella) == nullptr);
    CHECK(p.HasFlag(nccu::kFlagTookCursedUmbrella));   // 結局旗標未受影響
    CHECK(p.HeldUmbrellaKind() == HeldUmbrella::None);
}

// 每種撐在頭上的傘都對應恰好一列道具表（含破傘／陷阱傘）；None／Victim 不產生手持
// 種類的列（Victim 屬攜帶，透過其任務旗標顯示）。Ch2 管理員的借傘不在此列——它由
// Flag_LibrarianUmbrella 旗標驅動其背包列（見 test_ch2_quest），而非 HeldUmbrella 種類。
TEST_CASE("每種 HeldUmbrella 都對應其道具表列") {
    struct Case { HeldUmbrella kind; const char* item; };
    const Case cases[] = {
        {HeldUmbrella::True,          nccu::kItemTrueUmbrella},
        {HeldUmbrella::Cursed,        nccu::kItemCursedUmbrella},
        {HeldUmbrella::Ugly,          nccu::kItemUglyUmbrella},
        {HeldUmbrella::Fragile,       nccu::kItemFragileUmbrella},
        {HeldUmbrella::ProfessorTrap, nccu::kItemProfTrapUmbrella},
    };
    for (const auto& c : cases) {
        Player p{nccu::engine::math::Vec2{0, 0}};
        p.SetHeldUmbrella(c.kind);
        const auto rows = nccu::BuildInventoryRows(p);
        const nccu::InventoryRow* r = Find(rows, c.item);
        REQUIRE(r != nullptr);
        CHECK_FALSE(r->name.empty());
        CHECK_FALSE(r->description.empty());
        CHECK_FALSE(r->usable);            // 傘僅供檢視
    }
    // None / Victim → 無手持種類的傘列。
    Player none{nccu::engine::math::Vec2{0, 0}};
    CHECK(nccu::HeldUmbrellaCatalogId(none.HeldUmbrellaKind()) == nullptr);
    CHECK(nccu::HeldUmbrellaCatalogId(HeldUmbrella::Victim) == nullptr);
}

// 每個背包列在左緣有一個「分類」色塊，使金幣／雨傘／任務紙張讀起來與可用消耗品
// 是不同的種類。傘列畫的是世界／結局所用的同一個共用傘形圖（及其代表色），依
// 攜帶傘的標記決定。
TEST_CASE("背包傘列畫出其 umbrella-look 色塊") {
    using nccu::game::gfx::UmbrellaLook;
    // 詛咒傘列 → 暗紫色塊。
    {
        Spy r;
        std::vector<nccu::InventoryRow> rows{
            {"詛咒傘", 0, "傘骨上刻著別人的名字", false,
             nccu::kItemCursedUmbrella}};
        nccu::DrawInventory(r, rows, 0, 800.0f, 450.0f);
        CHECK(HasRectRGB(r,
            nccu::game::gfx::UmbrellaLookColor(UmbrellaLook::CursedPurple)));
    }
    // 醜傘列 → 螢光綠色塊。
    {
        Spy r;
        std::vector<nccu::InventoryRow> rows{
            {"螢光綠醜傘", 0, "醜得全校最好認", false,
             nccu::kItemUglyUmbrella}};
        nccu::DrawInventory(r, rows, 0, 800.0f, 450.0f);
        CHECK(HasRectRGB(r,
            nccu::game::gfx::UmbrellaLookColor(UmbrellaLook::UglyGreen)));
    }
    // 真傘列 → 藍色塊。
    {
        Spy r;
        std::vector<nccu::InventoryRow> rows{
            {"真傘", 0, "完美結局的關鍵", false, nccu::kItemTrueUmbrella}};
        nccu::DrawInventory(r, rows, 0, 800.0f, 450.0f);
        CHECK(HasRectRGB(r,
            nccu::game::gfx::UmbrellaLookColor(UmbrellaLook::TrueBlue)));
    }
}

// 多分類的背包會畫出比裸清單更多的矩形。
TEST_CASE("多分類的背包畫出比裸清單更多的矩形") {
    // 含金幣 + 消耗品 + 傘 + 紙張的背包會為每列畫一個色塊（使分類在視覺上可區別），
    // 即矩形數明顯多於「背景 + 面板 + 底線」。
    Spy r;
    std::vector<nccu::InventoryRow> rows{
        {"金幣", 100, "餘額", false, nccu::kItemMoney},
        {"暖暖包", 2, "立刻烘乾", true, "HotPack"},
        {"真傘", 0, "關鍵", false, nccu::kItemTrueUmbrella},
        {"申請書", 0, "交給助教", false, nccu::kItemForm},
    };
    nccu::DrawInventory(r, rows, /*cursor=*/0, 800.0f, 450.0f);
    // 背景 + 面板 + 底線 = 3；再加一個選取列底條與每列至少 1 個色塊矩形。
    // 四個分類 ⇒ 明顯多於 3 + 4。
    CHECK(r.rects > 8);
    // 4 個列名 + 說明名稱 + 說明 + 提示仍會繪製。
    CHECK(Has(r.texts, "> 金幣 x100"));
    CHECK(Has(r.texts, "  真傘"));
}

// ---- 分頁視窗計算（純函式、無渲染器）------------------------------------
// InventoryPageCount / InventoryPageOf 的視窗計算。
TEST_CASE("InventoryPageCount／InventoryPageOf 的視窗計算") {
    using nccu::InventoryPageCount;
    using nccu::InventoryPageOf;
    const int P = nccu::kInventoryRowsPerPage;
    REQUIRE(P >= 1);

    // 空 / 單頁。
    CHECK(InventoryPageCount(0) == 1);          // 空背包視為「1 / 1」
    CHECK(InventoryPageCount(1) == 1);
    CHECK(InventoryPageCount(P) == 1);          // 恰好一整頁
    CHECK(InventoryPageCount(P + 1) == 2);      // 多一個 → 第二頁
    CHECK(InventoryPageCount(2 * P) == 2);
    CHECK(InventoryPageCount(2 * P + 1) == 3);

    // 游標所在頁：列 0..P-1 → 第 0 頁，P..2P-1 → 第 1 頁，依此類推。
    CHECK(InventoryPageOf(0, 3 * P) == 0);
    CHECK(InventoryPageOf(P - 1, 3 * P) == 0);
    CHECK(InventoryPageOf(P, 3 * P) == 1);
    CHECK(InventoryPageOf(2 * P, 3 * P) == 2);
    // 超出範圍的游標會在除法前被夾住（不會有負頁、也不會超過最後一頁）——
    // 與 View 自身的夾值一致。
    CHECK(InventoryPageOf(-5, 2 * P) == 0);
    CHECK(InventoryPageOf(9999, 2 * P) == 1);   // 最後一頁
    CHECK(InventoryPageOf(0, 0) == 0);          // 空背包
}

// 列數超過一頁的背包會顯示游標所在頁（使被選取列可見）與「第 N／M 頁」指示。
// 不在當前頁的列不繪製；當前頁的列與被選取列會繪製。
TEST_CASE("列數超過一頁的背包會分頁並顯示頁碼指示") {
    const int P = nccu::kInventoryRowsPerPage;
    // 建立 P+2 列：r00..r(P+1)。名稱各異，方便檢查畫的是哪一頁。
    std::vector<nccu::InventoryRow> rows;
    for (int i = 0; i < P + 2; ++i) {
        std::string nm = "道具" + std::to_string(i);
        rows.push_back({nm, 0, "說明", false, "id" + std::to_string(i)});
    }
    const int total = nccu::InventoryPageCount(P + 2);
    REQUIRE(total == 2);

    // 游標在第 0 列 → 第 1 頁：道具0 可見，最後一列（第 2 頁）不可見。
    {
        Spy r;
        nccu::DrawInventory(r, rows, /*cursor=*/0, 800.0f, 450.0f);
        CHECK(Has(r.texts, "> 道具0"));
        CHECK_FALSE(Has(r.texts, "  道具" + std::to_string(P + 1)));
        CHECK(Has(r.texts, "第 1／2 頁   ←／→ 翻頁"));
    }
    // 游標在最後一列 → 第 2 頁：該列可見且被選取；道具0 不可見。
    {
        Spy r;
        nccu::DrawInventory(r, rows, /*cursor=*/P + 1, 800.0f, 450.0f);
        CHECK(Has(r.texts, "> 道具" + std::to_string(P + 1)));
        CHECK_FALSE(Has(r.texts, "  道具0"));
        CHECK(Has(r.texts, "第 2／2 頁   ←／→ 翻頁"));
    }
}

// 單頁背包仍顯示「第 1／1 頁」指示（一致的操作提示），但不顯示 ←/→ 翻頁提示
// （沒有頁可翻）。
TEST_CASE("單頁背包顯示「第 1／1 頁」但不顯示翻頁提示") {
    Spy r;
    std::vector<nccu::InventoryRow> rows{
        {"金幣", 100, "餘額", false, nccu::kItemMoney},
        {"暖暖包", 2, "立刻烘乾", true, "HotPack"},
    };
    nccu::DrawInventory(r, rows, /*cursor=*/0, 800.0f, 450.0f);
    CHECK(Has(r.texts, "第 1／1 頁"));
    CHECK_FALSE(Has(r.texts, "第 1／1 頁   ←／→ 翻頁"));
}

// ---- 長說明在框內換行 ---------------------------------------------------
// 最密集的道具表台詞必須依 CellWidth 拆成多列，使任何單段繪製文字都不超過框的
// 內部字寬上限 —— 亦即不會溢出邊界。以換行所用的同一個 CellWidth 斷言。
TEST_CASE("長說明會換行以容納於框寬內") {
    Spy r;
    const std::string longDesc =
        "使用：雨量 −35（彈開大半雨水）；專門擋雨，不影響業力。";
    std::vector<nccu::InventoryRow> rows{
        {"防水噴霧", 1, longDesc, true, "WaterproofSpray"}};
    nccu::DrawInventory(r, rows, /*cursor=*/0, 800.0f, 450.0f);

    // 每段屬於說明切片的繪製文字都必須在框的上限內（54 字寬 —— DrawInventory
    // 換行所用的值）。未拆的完整字串（此處約 48 字寬）也會通過 54 的界限，故為了
    // 證明真的有換行，再額外檢查一個刻意過長的說明會被拆成多於 1 段。
    for (const std::string& t : r.texts)
        CHECK(nccu::dialog::CellWidth(t) <= 54);

    Spy r2;
    std::string huge;
    for (int i = 0; i < 8; ++i) huge += "防水噴霧雨量擋雨業力";  // 約 80 字寬
    std::vector<nccu::InventoryRow> rows2{
        {"測試", 0, huge, false, "id"}};
    nccu::DrawInventory(r2, rows2, 0, 800.0f, 450.0f);
    int sliceRows = 0;
    for (const std::string& t : r2.texts)
        if (t.find("防水噴霧") != std::string::npos) ++sliceRows;
    CHECK(sliceRows >= 2);                       // 已換到多於 1 列
    for (const std::string& t : r2.texts)
        CHECK(nccu::dialog::CellWidth(t) <= 54); // 絕不寬於框
}

// ---- 新增手持傘標記的色塊對應 -------------------------------------------
// 背包色塊必須為每種手持傘畫出「正確」的傘外觀 —— 破傘／陷阱傘先前會錯誤地落到
// 完整的藍傘。此處斷言色塊的代表色出現在繪製矩形中。
TEST_CASE("fragile→破傘 與 proftrap→陷阱傘 的色塊正確") {
    using nccu::game::gfx::UmbrellaLook;
    using nccu::game::gfx::UmbrellaLookColor;

    // 破傘（Fragile）→ FragileBroken（傘柄／傘骨、灰）的代表色。
    {
        Spy r;
        std::vector<nccu::InventoryRow> rows{
            {"破傘", 0, "骨架斷了的傘", false, nccu::kItemFragileUmbrella}};
        nccu::DrawInventory(r, rows, 0, 800.0f, 450.0f);
        CHECK(HasRectRGB(r, UmbrellaLookColor(UmbrellaLook::FragileBroken)));
        // 而非它先前錯誤預設的完整藍傘。
        CHECK_FALSE(HasRectRGB(r, UmbrellaLookColor(UmbrellaLook::TrueBlue)));
    }
    // 陷阱傘（ProfessorTrap）→ 危險紅的代表色。
    {
        Spy r;
        std::vector<nccu::InventoryRow> rows{
            {"陷阱傘", 0, "傘骨上有奇怪的刻字", false,
             nccu::kItemProfTrapUmbrella}};
        nccu::DrawInventory(r, rows, 0, 800.0f, 450.0f);
        CHECK(HasRectRGB(r, UmbrellaLookColor(UmbrellaLook::ProfessorTrap)));
        CHECK_FALSE(HasRectRGB(r, UmbrellaLookColor(UmbrellaLook::TrueBlue)));
    }
    // 一般的管理員借傘以乾淨的藍傘面呈現即可。
    {
        Spy r;
        std::vector<nccu::InventoryRow> rows{
            {"管理員的傘", 0, "圖書館管理員借你的傘", false,
             nccu::kItemLoanerUmbrella}};
        nccu::DrawInventory(r, rows, 0, 800.0f, 450.0f);
        CHECK(HasRectRGB(r, UmbrellaLookColor(UmbrellaLook::TrueBlue)));
    }
}

// 第三章物物交換鏈攜帶的道具（香腸／大聲公）是僅供檢視的道具，而非可用消耗品 ——
// 它們必須 (a) 不顯示「E 使用」提示，且 (b) 畫出獨特的食物／道具色塊，而非可用
// 消耗品的青色藥水瓶。
TEST_CASE("第三章交換道具是僅供檢視的食物色塊列") {
    // DrawSwatch 對 RowKind::Food 使用的獨特食物色塊顏色。
    const nccu::engine::math::Color kFoodParcel{225, 140, 55, 255};
    // 青色消耗品瓶身 —— 第三章道具不可畫出此色。
    const nccu::engine::math::Color kConsumableFlask{60, 200, 180, 255};

    for (const char* id : {nccu::kItemSausage, nccu::kItemLoudspeaker}) {
        Spy r;
        std::vector<nccu::InventoryRow> rows{
            {"道具", 0, "拿去交換", /*usable=*/false, id}};
        nccu::DrawInventory(r, rows, 0, 800.0f, 450.0f);
        // 僅供檢視：一般提示，絕不顯示使用提示。
        CHECK(Has(r.texts, "↑↓ 選擇"));
        CHECK_FALSE(Has(r.texts, "↑↓ 選擇   E 使用"));
        // 有食物色塊；無青色消耗品瓶。
        CHECK(HasRectRGB(r, kFoodParcel));
        CHECK_FALSE(HasRectRGB(r, kConsumableFlask));
    }
}
