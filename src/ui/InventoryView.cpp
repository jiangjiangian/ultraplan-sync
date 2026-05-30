#include "ui/InventoryView.h"
#include "engine/render/IRenderer.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"
#include "engine/math/Color.h"
#include "game/gfx/UmbrellaGlyph.h"
#include "game/quest/ItemCatalog.h"   // kItem* 分類哨兵值
#include "game/dialog/DialogLayout.h" // WrapToCells——CJK 感知的說明文字換行

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace nccu {
using namespace nccu::game::gfx;  // game/gfx 輔助函式

using namespace nccu::engine::render;
using namespace nccu::engine::math;

// ---- 分頁視窗運算（純函式、於標頭宣告、有單元測試） ------
int InventoryPageCount(int rowCount) noexcept {
    if (rowCount <= 0) return 1;                 // 空背包為「1 / 1」
    return (rowCount + kInventoryRowsPerPage - 1) / kInventoryRowsPerPage;
}

int InventoryPageOf(int cursor, int rowCount) noexcept {
    if (rowCount <= 0) return 0;
    if (cursor < 0) cursor = 0;
    if (cursor >= rowCount) cursor = rowCount - 1;
    return cursor / kInventoryRowsPerPage;
}

namespace {

// 一個背包列的「分類」，純由其 itemId 衍生（僅呈現用——不含玩法邏輯，DTO 已帶有 id）。
// 它驅動左緣的色塊 + 該列顏色，使金幣 / 雨傘 / 任務紙張 / 食物道具讀來與可用消耗品屬不同
//「種類」。為第三章物物交換鏈的攜帶物（香腸 / 大聲公）新增 Food 類，使它們不再借用青色
// 藥水瓶色塊（那暗示「可用」）——它們只供檢視（usable=false），故「絕不可」讀成可用消耗品。
enum class RowKind { Consumable, Money, Umbrella, Paper, Food };

RowKind KindOf(const InventoryRow& row) {
    if (row.itemId == kItemMoney) return RowKind::Money;
    if (row.itemId == kItemForm || row.itemId == kItemNotes)
        return RowKind::Paper;
    // 第三章攜帶的交易物是只供檢視的道具，而非消耗品。
    if (row.itemId == kItemSausage || row.itemId == kItemLoudspeaker)
        return RowKind::Food;
    // 任何雨傘哨兵值（或攤販雨傘 id）→ Umbrella。共用 ItemCatalog::IsUmbrellaItemId
    //（與 BuildInventoryRows 用來把雨傘排除於計數迴圈外的「同一」述詞），使兩者不會走樣。
    if (IsUmbrellaItemId(row.itemId))
        return RowKind::Umbrella;
    return RowKind::Consumable;        // 持有的、可用的消耗品
}

// 雨傘列所用的雨傘外觀，使背包色塊與世界中／結局的字形一致（同一個共用的
// nccu::game::gfx::DrawUmbrellaGlyph）。先前只對應 cursed/ugly，其餘「全部」落到
// TrueBlue——故玩家現在能持有的破傘／陷阱傘會畫出錯誤的（完好藍色）傘面。將每個持有型
// 雨傘哨兵值對應到其真正外觀：
//   fragile  → FragileBroken（破傘，只剩傘柄／傘骨）
//   proftrap → ProfessorTrap（陷阱傘，危險紅）
//   cursed   → CursedPurple，ugly → UglyGreen
//   true / loaner（管理員的傘，普通借傘）／victim（苦主的透明傘）→ TrueBlue
UmbrellaLook UmbrellaLookOf(const InventoryRow& row) {
    if (row.itemId == kItemCursedUmbrella || row.itemId == "CursedUmbrella")
        return UmbrellaLook::CursedPurple;
    if (row.itemId == kItemUglyUmbrella || row.itemId == "UglyUmbrella")
        return UmbrellaLook::UglyGreen;
    if (row.itemId == kItemFragileUmbrella)
        return UmbrellaLook::FragileBroken;     // 破傘
    if (row.itemId == kItemProfTrapUmbrella)
        return UmbrellaLook::ProfessorTrap;     // 陷阱傘
    // 真傘 + 管理員借傘 + 攜帶的苦主透明傘，全部讀作乾淨的藍色傘面。
    return UmbrellaLook::TrueBlue;
}

// 各種類的列文字顏色：可用消耗品維持金色（你可以「做」些什麼），只供檢視的分類各有其
// 柔和色調，使玩家一眼就能分辨金幣 / 傘 / 紙 / 道具 列與可操作的列。
Color RowColor(RowKind k) {
    switch (k) {
        case RowKind::Money:      return Color{255, 205, 90, 255};   // 硬幣金
        case RowKind::Umbrella:   return Color{150, 200, 255, 255};  // 柔藍
        case RowKind::Paper:      return Color{225, 225, 230, 255};  // 紙白
        case RowKind::Food:       return Color{255, 190, 120, 255};  // 暖褐
        case RowKind::Consumable: return Colors::Gold;
    }
    return Colors::White;
}

// 在 `box` 內畫出左緣的小分類色塊。
void DrawSwatch(IRenderer& r, const InventoryRow& row, RowKind k, Rect box) {
    namespace C = Colors;
    switch (k) {
        case RowKind::Umbrella:
            DrawUmbrellaGlyph(r, UmbrellaLookOf(row), box);
            break;
        case RowKind::Money: {
            // 一枚略圓的金色硬幣標記（僅用矩形，內縮以讀作圓形）。
            r.DrawRect(Rect{box.x + box.width * 0.18f, box.y + box.height * 0.08f,
                            box.width * 0.64f, box.height * 0.84f}, C::Gold);
            r.DrawRect(Rect{box.x + box.width * 0.36f, box.y + box.height * 0.30f,
                            box.width * 0.28f, box.height * 0.40f},
                       Color{200, 150, 0, 255});           // 中央刻印
            break;
        }
        case RowKind::Paper: {
            // 一張帶折角的白紙——與世界中的拾取物一致。
            r.DrawRect(Rect{box.x + box.width * 0.20f, box.y + box.height * 0.10f,
                            box.width * 0.60f, box.height * 0.80f}, C::White);
            r.DrawRect(Rect{box.x + box.width * 0.58f, box.y + box.height * 0.10f,
                            box.width * 0.22f, box.height * 0.22f}, C::DarkGray);
            break;
        }
        case RowKind::Food: {
            // 一個鮮明的食物／道具標記——暖橘色包裹加一條較深的綁帶——使第三章的香腸 /
            // 大聲公 讀作攜帶的道具，「而非」可用消耗品的青色藥水瓶。僅用矩形（架構規則），
            // 內縮以讀作一個包裝好的物品。
            r.DrawRect(Rect{box.x + box.width * 0.18f, box.y + box.height * 0.22f,
                            box.width * 0.64f, box.height * 0.58f},
                       Color{225, 140, 55, 255});          // 包裹本體
            r.DrawRect(Rect{box.x + box.width * 0.44f, box.y + box.height * 0.10f,
                            box.width * 0.12f, box.height * 0.80f},
                       Color{150, 85, 30, 255});           // 綁帶
            break;
        }
        case RowKind::Consumable: {
            // 一個小藥水瓶：青色瓶身，使可用物品讀來有別。
            r.DrawRect(Rect{box.x + box.width * 0.32f, box.y + box.height * 0.05f,
                            box.width * 0.36f, box.height * 0.18f},
                       Color{180, 180, 185, 255});         // 瓶蓋
            r.DrawRect(Rect{box.x + box.width * 0.24f, box.y + box.height * 0.23f,
                            box.width * 0.52f, box.height * 0.70f},
                       Color{60, 200, 180, 255});          // 瓶身
            break;
        }
    }
}

}  // namespace

void DrawInventory(IRenderer& r,
                   const std::vector<InventoryRow>& rows,
                   int cursor,
                   float screenW, float screenH) {
    // 先把（凍結的）世界變暗，再畫一個置中面板——與對話框／結局卡片相同的疊層慣用法。
    r.DrawRect(Rect{0.0f, 0.0f, screenW, screenH}, Color{0, 0, 0, 140});

    // 一個「更大」的方框。原為 400x312、含 96px 的說明帶，較長的效果行會溢出；現在寬 468
    //（給換行後的列 + 說明更多空間）、高 372，尺寸使固定的 kInventoryRowsPerPage 列以舒適的
    // 26px 間距排列，「且」下方說明帶能把多達 kDescLines 個「換行後」的行完整容於邊框內。
    const float pw = 468.0f;
    const float ph = 372.0f;
    const float px = screenW * 0.5f - pw * 0.5f;
    const float py = screenH * 0.5f - ph * 0.5f;
    r.DrawRect(Rect{px, py, pw, ph}, Color{30, 30, 40, 235});
    // 一條金色標題底線，使面板讀作一個有框的視窗。
    r.DrawRect(Rect{px + 16.0f, py + 44.0f, pw - 32.0f, 2.0f},
               Color{255, 200, 70, 220});

    r.DrawText("物品欄", Vec2{px + 16.0f, py + 12.0f}, 24, Colors::White);

    const float listTop = py + 58.0f;
    if (rows.empty()) {
        r.DrawText("（空）", Vec2{px + 16.0f, listTop}, 18,
                   Color{200, 200, 200, 255});
        return;
    }

    const int rowCount = static_cast<int>(rows.size());
    // 把高亮索引夾到範圍內，使超出邊界的游標（例如某列被使用後清單縮短）仍能為說明面板
    // 選到一個真實的列。
    const int sel = std::min(std::max(cursor, 0), rowCount - 1);

    // ---- 分頁。只顯示游標所在頁，使被選列「永遠」可見，且大背包絕不會把列擠扁／溢出
    // 方框。頁索引由游標「衍生」（不保留 UI 狀態、不序列化任何東西——存檔逐位元一致）。
    const int pageCount = InventoryPageCount(rowCount);
    const int page      = InventoryPageOf(sel, rowCount);
    const int first     = page * kInventoryRowsPerPage;
    const int last      = std::min(rowCount, first + kInventoryRowsPerPage);

    // 幾何分帶：列位於上方帶、以固定舒適間距排列，接著一條頁碼指示條，再來是位於底部的
    // 說明帶。
    constexpr float kRowH     = 26.0f;
    constexpr float kSwatchSz = 20.0f;
    constexpr int   kDescLines = 3;          // 顯示的換行說明行數
    constexpr float kDescLineH = 18.0f;

    // 每個「可見」背包列畫一列：左緣分類色塊、被選列上的「> 」插字符、中文名稱，以及
    // count>0 時的「xN」後綴。被選列後方加一條高亮條，使游標明確無誤。索引 i 是絕對列號。
    for (int i = first; i < last; ++i) {
        const InventoryRow& row = rows[static_cast<std::size_t>(i)];
        const bool isSel = (i == sel);
        const RowKind kind = KindOf(row);
        const float rowY = listTop + static_cast<float>(i - first) * kRowH;

        if (isSel)
            r.DrawRect(Rect{px + 8.0f, rowY - 2.0f, pw - 16.0f, kRowH - 2.0f},
                       Color{70, 60, 25, 235});            // 選取條

        // 分類色塊（雨傘列會畫出其實際外觀）。
        DrawSwatch(r, row, kind,
                   Rect{px + 14.0f, rowY + (kRowH - kSwatchSz) * 0.5f - 1.0f,
                        kSwatchSz, kSwatchSz});

        std::string line = (isSel ? "> " : "  ") + row.name;
        if (row.count > 0) line += " x" + std::to_string(row.count);
        // 被選列讀作亮橘色；否則用各分類色調，使金幣 / 雨傘 / 任務紙張 / 道具 在視覺上與
        // 可用物品分明。
        const Color c = isSel ? Color{255, 170, 40, 255} : RowColor(kind);
        r.DrawText(line, Vec2{px + 14.0f + kSwatchSz + 8.0f, rowY + 2.0f},
                   18, c);
    }

    // 「第 N／M 頁」頁碼指示——位於列帶正下方的獨立條上，並在超過 1 頁時加上 ←/→翻頁
    // 提示，使導覽可被發現。每個背包都繪製（單頁讀作「第 1／1 頁」），使提示性一致。
    const float pageY = listTop + kInventoryRowsPerPage * kRowH + 4.0f;
    {
        std::string ind = "第 " + std::to_string(page + 1) + "／" +
                          std::to_string(pageCount) + " 頁";
        if (pageCount > 1) ind += "   ←／→ 翻頁";
        r.DrawText(ind, Vec2{px + 16.0f, pageY}, 14, Color{200, 200, 210, 255});
    }

    // ---- 被選列的說明帶——一條淡分隔線、列名，接著「換行後」的圖鑑文字（使其絕不衝出
    // 方框邊框），再加一個使用提示。位於面板底部、列帶 + 頁碼條下方，故絕不與它們重疊。
    const InventoryRow& cur = rows[static_cast<std::size_t>(sel)];
    const float descTop = pageY + 24.0f;
    r.DrawRect(Rect{px + 12.0f, descTop - 8.0f, pw - 24.0f, 1.0f},
               Color{120, 120, 140, 200});
    r.DrawText(cur.name, Vec2{px + 16.0f, descTop}, 18, Colors::Gold);

    // 把說明換行到方框的內部「字格」寬度（東亞寬度感知，本專案的單一事實來源
    // nccu::dialog::WrapToCells——全形 CJK = 2 格）。文字由 px+16 延伸到約 px+pw-16
    //（內部約 436px）；在字體大小 14 下，全形字形推進約 14 + 14/10 ≈ 15.4px ≈ 7.7px/格，
    // 故約 436/7.7 ≈ 56 格可容於一列。取 54 以對方框右緣留安全裕度。顯示多達 kDescLines
    // 個換行行；最密的圖鑑行（防水噴霧的「使用：雨量 −35（彈開大半雨水）；專門擋雨，不影響
    // 業力。」約 48 格）可容於一列，故 kDescLines=3 仍留有充裕空間。
    if (!cur.description.empty()) {
        constexpr int kDescCells = 54;
        const std::vector<std::string> wrapped =
            nccu::dialog::WrapToCells(cur.description, kDescCells);
        const int shown =
            std::min(static_cast<int>(wrapped.size()), kDescLines);
        for (int i = 0; i < shown; ++i)
            r.DrawText(wrapped[static_cast<std::size_t>(i)],
                       Vec2{px + 16.0f,
                            descTop + 24.0f + static_cast<float>(i) * kDescLineH},
                       14, Color{210, 210, 215, 255});
    }
    r.DrawText(cur.usable ? "↑↓ 選擇   E 使用" : "↑↓ 選擇",
               Vec2{px + 16.0f,
                    descTop + 24.0f + static_cast<float>(kDescLines) * kDescLineH},
               14, Color{185, 185, 190, 255});
}

} // namespace nccu
