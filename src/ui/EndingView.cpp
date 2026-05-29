#include "ui/EndingView.h"
#include "game/dialog/DialogLayout.h"
#include "engine/render/IRenderer.h"
#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"
#include "engine/math/Color.h"
#include "game/gfx/UmbrellaGlyph.h"
#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace nccu {
using namespace nccu::game::gfx;  // game/gfx 輔助函式

using namespace nccu::engine::render;
using namespace nccu::engine::math;

bool IsEndingState(SemesterState s) noexcept {
    return s == SemesterState::Ending_A ||
           s == SemesterState::Ending_B ||
           s == SemesterState::Ending_D ||
           s == SemesterState::Ending_C;
}

namespace {

// 各結局的開場字卡，依據遊戲企劃與敘事架構：
//   A 結局A 完美通關 (True)  — 真相大白：雨過天晴，傘回到你手上。
//   B 結局B 屠龍者終成惡龍   — 詛咒傳承：企劃明定的成就字卡，
//       「你成為了你曾經最討厭的那種人」。
//   C 結局C 破財消災 (Normal) — 沿用既有的開場字卡。
// 刻意保持簡短（不超過對話框的份量），且不含任何外部工具／模型品牌字串。
std::string_view caption(SemesterState s) {
    switch (s) {
        case SemesterState::Ending_A:
            return "「雨過天晴，傘還在你手上。」";
        case SemesterState::Ending_B:
            return "「你成為了你曾經最討厭的那種人」";
        case SemesterState::Ending_D:
            return "「傘破了，但你沒丟下任何人。」";
        case SemesterState::Ending_C:
            return "「這樣以後再也不會有人拿錯你的傘了。」";
        default:
            return "";
    }
}

// 劇情內「你為何走到這裡」的故事台詞，精簡轉錄自各結局內容文件頂端的字卡區塊（內容
// 文件維持為敘事聖經；「這張」表格才是實際渲染的內容，因為 View.cpp 在那些文件被繪製
// 之前就提前返回）。每行都短到能容於螢幕寬度；此處每個字形都已烘焙進
// nccu::engine::render::Font.h 的 UiLiteralChars()，故字卡絕不會出現缺字方塊。每個結局
// 3 行，使字卡容於 450px 視窗內。
const std::vector<std::string>& reasonLines(SemesterState s) {
    static const std::vector<std::string> kA = {
        "你把不屬於你的傘，一把一把都還了回去。",
        "面對抱錯傘的人，你沒有把氣討回來，",
        "只說了一句「辛苦了」。雨，就這樣停了。",
    };
    static const std::vector<std::string> kB = {
        "該放手的時候你沒放，",
        "該體諒的時候你選了把氣討回來。",
        "到頭來，你也成了會在傘架前伸手的人。",
    };
    static const std::vector<std::string> kC = {
        "你沒有去追那把丟掉的傘，",
        "也沒有計較是誰拿走的——",
        "你掏光口袋的錢，買過了這整場雨。",
    };
    // 結局 D — 風雨同行：選了體諒卻未達成 A。心地善良，但傘已被整學期的風雨磨穿。
    static const std::vector<std::string> kD = {
        "你選擇了體諒，沒把這口氣討回來。",
        "只是這學期的風吹雨打，把傘磨破了——",
        "傘骨還在，雨會滲進來，但你撐著它走完了。",
    };
    static const std::vector<std::string> kNone;
    switch (s) {
        case SemesterState::Ending_A: return kA;
        case SemesterState::Ending_B: return kB;
        case SemesterState::Ending_D: return kD;
        case SemesterState::Ending_C: return kC;
        default:                      return kNone;
    }
}

// 各結局的路線標籤（結算卡片頁尾）——採用企劃自身的命名。
std::string_view pathLabel(SemesterState s) {
    switch (s) {
        case SemesterState::Ending_A: return "完美結局（True）";
        case SemesterState::Ending_B: return "墮落結局";
        case SemesterState::Ending_D: return "風雨同行結局";
        case SemesterState::Ending_C: return "務實結局（Normal）";
        default:                      return "";
    }
}

// 組出本次遊玩「實際」觸發之條件的精簡清單——對齊 EndingGate.cpp 各結局的邏輯，使
// 卡片解釋的是本次判定、而非通用評分準則。每個條目前綴一個 ✓「已達成」標記（U+2713，
// 已烘焙進字形圖集）。只列出已觸發的條件（B/C 有多個可能觸發源；只顯示成立者）。A 永遠
// 顯示其完整的三項 AND。
std::vector<std::string> conditionsFired(const EndingSummary& g) {
    std::vector<std::string> out;
    const std::string mark = "\xE2\x9C\x93 ";   // U+2713 ✓ 加一個空格
    switch (g.state) {
        case SemesterState::Ending_A:
            // 結局 A：karma>80 && Flag_HasTrueUmbrella && Flag_ConsoledTA
            // ——三者皆為 AND，永遠顯示。
            out.push_back(mark + "業力 > 80");
            out.push_back(mark + "還回真傘");
            out.push_back(mark + "體諒助教");
            break;
        case SemesterState::Ending_B: {
            // 結局 B：Flag_TookCursedUmbrella || karma<0 ||
            // coldFinale(finaleChoiceMade && !consoledTA)。只顯示實際成立的
            // 析取項。
            if (g.tookCursed) out.push_back(mark + "拿了詛咒傘");
            if (g.karma < 0)  out.push_back(mark + "業力低於零");
            if (g.finaleChoiceMade && !g.consoledTA)
                out.push_back(mark + "最後質問助教");
            break;
        }
        case SemesterState::Ending_D:
            // 結局 D：在此走到 Flag_ConsoledTA ⇒ 選了體諒但未達 A（karma ≤ 80）
            // 也非 B。顯示使玩家落在風雨同行路線的兩個條件。
            out.push_back(mark + "體諒助教");
            out.push_back(mark + "業力 ≤ 80（差一點點圓滿）");
            break;
        case SemesterState::Ending_C:
            // 結局 C：Flag_BoughtUglyUmbrella || Flag_TaFinaleChoiceMade
            //（平穩收尾的預設）。顯示是哪一個成立。
            if (g.boughtUgly) out.push_back(mark + "買了醜傘");
            if (g.finaleChoiceMade && !g.boughtUgly)
                out.push_back(mark + "平穩收尾（未體諒、未買綠傘）");
            break;
        default:
            break;
    }
    return out;
}

// 在沒有渲染器端 MeasureText（IRenderer 未提供——見 MessageView.cpp）的情況下做水平
// 置中。nccu::dialog::CellWidth 是本專案與字型無關的文字量測輔助函式（依東亞寬度：
// CJK = 2 格、ASCII = 1 格），與 View.cpp 為目標面板所用的字格估計相同。在字體大小
//`sz` 下，全形字形推進約 `sz` px、窄字形約 `sz/2`，即每格約 `sz/2` px——故 `s` 的像素
// 寬約等於 CellWidth(s) * sz/2。回傳可使其置中的 x。
float CenteredX(const std::string& s, int sz, float screenW) {
    const float w = static_cast<float>(nccu::dialog::CellWidth(s)) *
                    (static_cast<float>(sz) * 0.5f);
    const float x = screenW * 0.5f - w * 0.5f;
    return x < 0.0f ? 0.0f : x;
}

// `s` 在字體大小 `sz` 下的像素寬估計（沿用上方字格模型）——用來為結算面板定寬，使其
// 緊貼最寬的一列。
float TextWidthPx(const std::string& s, int sz) {
    return static_cast<float>(nccu::dialog::CellWidth(s)) *
           (static_cast<float>(sz) * 0.5f);
}

// 在字體大小 `sz`、沿用共用的每格約 sz/2 px 模型下，`widthPx` 內可容納多少 EAW 字格。
// 用來換行較長的卡片文字，使其絕不溢出黑框（卡片原本是針對 800×450 手工排版、未換行）。
// 至少取 1，使極窄的框仍能輸出列。
int CellsForWidth(float widthPx, int sz) {
    const float perCell = static_cast<float>(sz) * 0.5f;
    return std::max(1, static_cast<int>(widthPx / perCell));
}

// 以本專案 EAW 感知的 nccu::dialog::WrapToCells 將 `s` 換行到螢幕文字寬度（左右各內縮
// 一段邊距），再把每列置中畫在 `y`、依 `lineH` 遞進。回傳最後一列「之後」的 y。在沒有
// 渲染器端 MeasureText 的情況下（IRenderer 無此功能——與本檔其餘部分受同一限制），仍使
// 每張卡片的列都容於面板內。
float DrawCenteredWrapped(IRenderer& r, const std::string& s, int sz,
                          float screenW, float marginPx, float y,
                          float lineH, Color col) {
    const float textW = std::max(40.0f, screenW - marginPx * 2.0f);
    for (const std::string& row :
         nccu::dialog::WrapToCells(s, CellsForWidth(textW, sz))) {
        r.DrawText(row, Vec2{CenteredX(row, sz, screenW), y}, sz, col);
        y += lineH;
    }
    return y;
}

// 依企劃，結局 B 是灰暗色調的時間線（「畫面色調永久轉為灰暗」）：把原本白色的標題／字卡
// 去飽和偏灰，使壞結局看起來明顯比 A/C 更冷。純呈現層。
Color endingTextColor(SemesterState s, unsigned char a) {
    if (s == SemesterState::Ending_B) return Color{150, 150, 155, a};
    return Color{255, 255, 255, a};
}

// 企劃需求「第四章結局顯示最終雨傘樣貌」：卡片顯示本次遊玩「結束時」持有的傘，並以與
// 世界中雨傘／拾取物相同的共用字形繪製。以「結局」（而非原始旗標）為鍵，保證它絕不會與
// 判定不符——這是「體諒卻顯示醜傘」的修正：
//   A 完美結局 → 真傘（藍）   B 墮落結局 → 詛咒傘（暗紫）
//   C 務實結局 → 醜傘（綠）
[[nodiscard]] nccu::game::gfx::UmbrellaLook endingUmbrellaLook(SemesterState s) {
    switch (s) {
        case SemesterState::Ending_A: return nccu::game::gfx::UmbrellaLook::TrueBlue;
        case SemesterState::Ending_B: return nccu::game::gfx::UmbrellaLook::CursedPurple;
        // 結局 D 的破傘——心地善良，但傘面已不在，只剩彎折的傘骨／傘柄（即「風吹雨打把傘
        // 磨破了」）。
        case SemesterState::Ending_D: return nccu::game::gfx::UmbrellaLook::FragileBroken;
        case SemesterState::Ending_C: return nccu::game::gfx::UmbrellaLook::UglyGreen;
        default:                      return nccu::game::gfx::UmbrellaLook::TrueBlue;
    }
}

// 底部選單的標籤表——三個選項字串的單一事實來源。集中於此（而非內聯字面值），使
// EndingCardStrings 能餵給字形掃描測試，且渲染器讀到的是同一份文字。
std::string_view endingMenuLabel(EndingMenuChoice c) {
    switch (c) {
        case EndingMenuChoice::BackToTitle: return "回首頁";
        case EndingMenuChoice::RestartGame: return "重新開始";
        case EndingMenuChoice::Quit:        return "結束";
    }
    return "";
}

}  // namespace

EndingMenuChoice EndingMenuChoiceAt(int index) noexcept {
    // 夾到有效集合內，使游標亂跑時也絕不會選到「無」。
    switch (((index % 3) + 3) % 3) {
        case 0:  return EndingMenuChoice::BackToTitle;
        case 1:  return EndingMenuChoice::RestartGame;
        default: return EndingMenuChoice::Quit;
    }
}

std::string_view EndingMenuLabel(EndingMenuChoice c) noexcept {
    return endingMenuLabel(c);
}

std::vector<std::string> EndingCardStrings() {
    std::vector<std::string> out;
    // 每張卡片都會繪製、與狀態無關的靜態字串。
    out.emplace_back("── 為什麼你走到這裡 ──");
    out.emplace_back("結算");
    out.emplace_back("業力 karma：0");   // 業力標籤的字形（數字為 ASCII）
    out.emplace_back("結局類型：");
    // 底部選單的選項標籤 + 導覽提示，使字形掃描測試（test_font_ui_glyph_scan.cpp 會掃
    // EndingCardStrings）能驗證每個選單字形都已烘焙進字形圖集——不會無聲缺字。
    out.emplace_back(std::string(endingMenuLabel(EndingMenuChoice::BackToTitle)));
    out.emplace_back(std::string(endingMenuLabel(EndingMenuChoice::RestartGame)));
    out.emplace_back(std::string(endingMenuLabel(EndingMenuChoice::Quit)));
    out.emplace_back("← → 選擇   E 確認");
    const SemesterState states[] = {SemesterState::Ending_A,
                                    SemesterState::Ending_B,
                                    SemesterState::Ending_D,
                                    SemesterState::Ending_C};
    for (SemesterState s : states) {
        out.emplace_back(caption(s));
        out.emplace_back(pathLabel(s));
        for (const std::string& ln : reasonLines(s)) out.push_back(ln);
        // 強制讓「每個」條件分支都觸發，使每個標籤都被擷取（實際卡片只顯示已觸發的子集；
        // 測試需要全部）。
        EndingSummary g;
        g.state            = s;
        g.karma            = -1;     // 使 B 的「業力低於零」觸發
        g.hasTrueUmbrella  = true;
        g.consoledTA       = false;  // 使 B/C 的終局分支觸發
        g.tookCursed       = true;
        g.boughtUgly       = true;
        g.finaleChoiceMade = true;
        for (const std::string& c : conditionsFired(g)) out.push_back(c);
        // C 的「平穩收尾」只在 !boughtUgly 時觸發——也一併擷取。
        EndingSummary g2 = g;
        g2.boughtUgly = false;
        for (const std::string& c : conditionsFired(g2)) out.push_back(c);
    }
    return out;
}

void DrawEndingCard(IRenderer& r, const EndingSummary& summary,
                    std::string_view title, float alpha,
                    float screenW, float screenH, int menuCursor) {
    alpha = std::min(1.0f, std::max(0.0f, alpha));
    const unsigned char a = static_cast<unsigned char>(alpha * 255.0f);
    const SemesterState state = summary.state;

    // 自成一體的淡出：背板採用相同 alpha，使偵測替身測試即使 View 也提前返回，仍能看到
    // 一張真正的卡片。
    r.DrawRect(Rect{0.0f, 0.0f, screenW, screenH}, Color{0, 0, 0, a});

    // ---- 最終雨傘（企劃需求「第四章結局顯示最終雨傘樣貌」） --------
    // 本次遊玩結束時持有之傘的主視覺色塊，以與世界中雨傘 + 拾取物相同的共用字形繪製，並以
    //「結局」為鍵，使其絕不會與判定不符（A 真傘藍／B 詛咒傘暗紫／C 醜傘綠）。置於頂端
    // 置中，並隨卡片淡出做 alpha 縮放。
    constexpr float kUmbW = 56.0f;
    constexpr float kUmbH = 50.0f;
    nccu::game::gfx::DrawUmbrellaGlyph(
        r, endingUmbrellaLook(state),
        Rect{screenW * 0.5f - kUmbW * 0.5f, 8.0f, kUmbW, kUmbH}, a);

    // ---- 標題 + 開場字卡（經字格模型置中） ------------
    // 依企劃，結局 B 偏灰；A/C 維持白色。尺寸／位置是為 800x450 視窗挑選的，使雨傘色塊、
    // 標題、字卡、理由區塊與結算卡片全部容於同一螢幕（單一螢幕——保留簡單的 endingAlpha_
    // 淡出；無需分頁）。
    constexpr int kTitleSize   = 28;
    constexpr int kCaptionSize = 18;
    constexpr int kReasonSize  = 16;
    constexpr int kStatSize    = 15;
    const Color tint = endingTextColor(state, a);
    const std::string ttl{title};
    const std::string cap{caption(state)};
    // 每張卡片的列都會在這個側邊距內換行，使較長的字卡／理由行也能在黑框內自動換行，而非
    // 衝出 800px 邊緣。標題維持單行（依設計即短，且置中）。
    constexpr float kSideMargin = 28.0f;
    r.DrawText(ttl, Vec2{CenteredX(ttl, kTitleSize, screenW), 64.0f},
               kTitleSize, tint);
    DrawCenteredWrapped(r, cap, kCaptionSize, screenW, kSideMargin, 100.0f,
                        22.0f, tint);

    // ---- 「你為何走到這裡」理由區塊（故事台詞） -------------
    // 先一個淡色的區段標籤，再 2-3 行劇情內台詞，置中並換行，使長行絕不溢出框。
    float y = 136.0f;
    {
        const std::string hdr = "── 為什麼你走到這裡 ──";
        r.DrawText(hdr, Vec2{CenteredX(hdr, kReasonSize, screenW), y},
                   kReasonSize, Color{200, 200, 205, a});
        y += 26.0f;
        for (const std::string& ln : reasonLines(state))
            y = DrawCenteredWrapped(r, ln, kReasonSize, screenW, kSideMargin,
                                    y, 22.0f, tint);
    }

    // ---- 結算統計卡片：緊貼業力 + 檢核清單的面板 -------
    // 位於理由區塊下方的置中面板。寬度取字格模型下最寬的一列（此處無法 MeasureText——與
    // HUD 受同一限制）；高度容納標頭 + 業力 + 每個已觸發條件 + 路線標籤。僅由 DTO 渲染
    // 建構。
    const std::string statHdr = "結算";
    char kbuf[32] = {0};
    std::snprintf(kbuf, sizeof(kbuf), "業力 karma：%d", summary.karma);
    const std::string karmaLine = kbuf;
    const std::vector<std::string> conds = conditionsFired(summary);
    const std::string path = std::string("結局類型：") +
                             std::string(pathLabel(state));

    // 最寬的一列決定面板寬度。
    float contentW = std::max(TextWidthPx(karmaLine, kStatSize),
                              TextWidthPx(path, kStatSize));
    contentW = std::max(contentW, TextWidthPx(statHdr, kStatSize + 3));
    for (const std::string& c : conds)
        contentW = std::max(contentW, TextWidthPx(c, kStatSize));

    constexpr float kPad = 14.0f;
    // 列：標頭、業力、每個條件、路線。
    const int rows = 3 + static_cast<int>(conds.size());
    const float rowH   = 22.0f;
    const float panelW = contentW + kPad * 2.0f;
    const float panelH = static_cast<float>(rows) * rowH + kPad * 2.0f;
    float panelX = screenW * 0.5f - panelW * 0.5f;
    if (panelX < 8.0f) panelX = 8.0f;
    // 把卡片緊貼放在理由區塊下方；夾住範圍，使其絕不衝出 450px 視窗的底邊。
    float panelY = y + 12.0f;
    const float maxPanelY = screenH - panelH - 8.0f;
    if (panelY > maxPanelY) panelY = maxPanelY < 0.0f ? 0.0f : maxPanelY;

    // 面板底色——比純黑背板略亮一點，使卡片讀來像個獨立元素；隨淡出做 alpha 縮放。
    const unsigned char panelA = static_cast<unsigned char>(alpha * 220.0f);
    r.DrawRect(Rect{panelX, panelY, panelW, panelH},
               Color{28, 30, 40, panelA});

    float sy = panelY + kPad;
    // 標頭（金色，使結算卡片在視覺上有定錨）。
    r.DrawText(statHdr, Vec2{panelX + kPad, sy}, kStatSize + 3,
               Color{255, 200, 70, a});
    sy += rowH;
    r.DrawText(karmaLine, Vec2{panelX + kPad, sy}, kStatSize, tint);
    sy += rowH;
    for (const std::string& c : conds) {
        r.DrawText(c, Vec2{panelX + kPad, sy}, kStatSize,
                   Color{120, 230, 140, a});   // 綠色 = 條件達成
        sy += rowH;
    }
    r.DrawText(path, Vec2{panelX + kPad, sy}, kStatSize,
               Color{200, 200, 205, a});

    // ---- 底部 3 選項選單（回首頁 / 重新開始 / 結束） -------
    // 結局畫面現在是穩定的可互動畫面；這一列是玩家在此唯一的能動之處。←/→ 移動游標
    //（於 GameController 處理），E/Enter 確認 → World::PendingAppAction。View「只」負責
    // 渲染：高亮選項加上插字符 + 金色；其餘變暗。水平置中排在接近底邊處，使其絕不與結算
    // 面板（其範圍被夾在 screenH - panelH - 8 之上）碰撞。以完整卡片 alpha 繪製，故隨畫面
    // 其餘部分一起淡入，顯示後保持穩定。
    constexpr int   kMenuSize = 18;
    constexpr float kHintSize = 13.0f;
    constexpr int   kMenuItems = 3;   // |EndingMenuChoice|
    const int cursor = ((menuCursor % kMenuItems) + kMenuItems) % kMenuItems;
    // 組出三個標籤，並在被選中者前加上插字符，使選取即使在灰階下也明確無歧義（無障礙
    // 考量）。插字符是暫停選單所用的 ASCII "> "（必在字形圖集中）。
    std::string opts[kMenuItems];
    float optW[kMenuItems];
    float totalW = 0.0f;
    constexpr float kGap = 26.0f;
    for (int i = 0; i < kMenuItems; ++i) {
        const std::string lbl{endingMenuLabel(EndingMenuChoiceAt(i))};
        opts[i] = (i == cursor ? std::string("> ") : std::string("  ")) + lbl;
        optW[i] = TextWidthPx(opts[i], kMenuSize);
        totalW += optW[i];
    }
    totalW += kGap * 2.0f;
    // 把這一列放在接近底部處；其後墊一塊薄面板，使它讀來像一條控制列，與上方故事文字
    // 區隔開。
    const float menuH = static_cast<float>(kMenuSize) + 14.0f;
    const float menuY = screenH - menuH - 22.0f;
    const float barX  = screenW * 0.5f - totalW * 0.5f - 12.0f;
    const float barW  = totalW + 24.0f;
    r.DrawRect(Rect{barX, menuY - 5.0f, barW, menuH},
               Color{18, 20, 28, static_cast<unsigned char>(alpha * 220.0f)});
    float ox = screenW * 0.5f - totalW * 0.5f;
    for (int i = 0; i < kMenuItems; ++i) {
        const Color c = (i == cursor) ? Color{255, 200, 70, a}    // 金色 = 已選
                                      : Color{170, 170, 180, a};  // 暗色 = 未選
        r.DrawText(opts[i], Vec2{ox, menuY}, kMenuSize, c);
        ox += optW[i] + kGap;
    }
    // 選項列正下方的導覽提示。
    const std::string hint = "← → 選擇   E 確認";
    r.DrawText(hint,
               Vec2{CenteredX(hint, static_cast<int>(kHintSize), screenW),
                    menuY + static_cast<float>(kMenuSize) + 2.0f},
               static_cast<int>(kHintSize), Color{150, 150, 160, a});
}

} // namespace nccu
