#ifndef GFX_FONT_H_
#define GFX_FONT_H_
#include "raylib.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <set>
#include <string>
#include <vector>

/**
 * @file Font.h
 * @brief 程序級 CJK 字型管理器：載入一份涵蓋遊戲實際顯示字元的字型供 DrawTextEx 使用。
 *
 * raylib 內建預設字型僅含 ASCII，所有中文字串（HUD／對話／結局／物品欄）經文字
 * 層繪製時皆會變成 `?`／方框。本標頭載入一份字型、收齊遊戲真正會顯示的全部
 * 碼位並對外暴露。raylib.h 限制在此，符合渲染層的隔離不變式。
 */
namespace nccu::engine::render {

namespace detail {

/**
 * @brief 依序排列的 CJK 字型候選清單。
 * @return 候選字型路徑清單（依嘗試優先序排列）。
 *
 * raylib 的 LoadFontData 寫死 stbtt_InitFont(..., 0)：不支援 .ttc collection 索引
 * 選擇；遇到其內建 stb_truetype 無法點陣化的字型輪廓（CFF／PostScript——PingFang
 * 與 Hiragino 使用）會回傳 glyphCount==0 並悄悄退回僅 ASCII 的預設字型，使每個中文
 * 字渲染成 `?`。因此單一固定路徑很脆弱：EnsureFont 必須逐一嘗試候選，保留第一個
 * 確實產出 glyphCount>0 的字型。
 *
 * 順序：先試使用者自備的隨附字型（把真實 .ttf 放到 resources/assets/fonts/cjk.ttf
 * 可保證有一份 parser 友善的字型——resources/ 由使用者管理且未追蹤，故不提交任何
 * 二進位字型）；其次盡力嘗試 macOS 字型（TrueType 輪廓者排在 CFF .ttc 之前，讓本機
 * stb_truetype 能解析者勝出）；最後是 Linux 的 Noto。
 */
inline const std::vector<std::string>& FontCandidates() {
    static const std::vector<std::string> kCandidates = {
        "resources/assets/fonts/cjk.ttf",
        "resources/assets/fonts/cjk.otf",
        "resources/assets/fonts/cjk.ttc",
        "resources/assets/fonts/font.ttf",
        "resources/assets/fonts/font.otf",
        "resources/assets/fonts/font.ttc",
        // macOS——先試 .ttf／TrueType 輪廓字型，再試 stb_truetype 常無法點陣化的
        // CFF 輪廓 .ttc（PingFang／Hiragino）。皆不保證存在；迴圈會略過缺檔者，
        // 並對其餘者驗證 glyphCount>0。
        "/System/Library/Fonts/Supplemental/Arial Unicode.ttf",
        "/System/Library/Fonts/STHeiti Medium.ttc",
        "/System/Library/Fonts/STHeiti Light.ttc",
        "/Library/Fonts/Songti.ttc",
        "/System/Library/Fonts/Supplemental/Songti.ttc",
        "/System/Library/Fonts/PingFang.ttc",
        "/System/Library/Fonts/Hiragino Sans GB.ttc",
        // Linux (Noto CJK).
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/truetype/noto/NotoSansCJKtc-Regular.otf",
        "/usr/share/fonts/opentype/noto/NotoSerifCJK-Regular.ttc",
    };
    return kCandidates;
}

/**
 * @brief 由程式碼字面值構成的 UI 字元集合（即使 docs/content 無法讀取也能正確顯示）。
 * @return 涵蓋各 UI 字面值所有 CJK 字元的 UTF-8 字串（由 raylib 碼位載入器解碼）。
 *
 * 收齊各 UI 相關 .cpp 字串字面值（如 View.cpp／DialogView.cpp／EndingView.cpp／
 * InventoryView.cpp 等）所用的 CJK 字元，整理成精簡的靜態清單，使字型在 docs/content
 * 不可讀的退路上仍可運作。各區塊以註解標明對應的 UI 文字來源。
 */
inline const char* UiLiteralChars() {
    return
        // 結局畫面標題（A 真相大白／B 屠龍者終成惡龍／C 破財消災）——每個字皆保留
        // 於此，使卡片即便在內容不可讀的退路上仍正確顯示（部分如 討／厭 在
        // docs/content 下的 *.md 中根本不出現）。
        "雨過天晴傘還在你手上"
        "你成為了曾經最討厭的那種人"
        "這樣以後再也不會有拿錯的了"
        // 上述 UI 字面值所用的標點（CJK 引號／逗號／句號）——全形，與表意字一同
        // 收入圖集。
        "「」，。"
        // 對話框分頁提示：U+25BC ▼「還有更多」記號。非 ASCII 且不在任何內容 .md 中，
        // 缺它便會渲染成 raylib 無字符的 `?`（曾出現的方框缺字）。對話框只用向下記號，
        // 無向上 ▲ 記號，故不加。
        "\xE2\x96\xBC"  // U+25BC ▼
        // U+2190 ←／U+2192 →／U+2191 ↑／U+2193 ↓——首頁、選角與遊戲內選單提示行
        // 所用的方向箭頭。非 ASCII、不在 docs/content 中，缺它便成方框。
        "\xE2\x86\x90\xE2\x86\x92\xE2\x86\x91\xE2\x86\x93"
        // 首頁／遊戲內選單／五人選角的字串字面值（TitleScreen.cpp／
        // CharacterSelect.cpp／View.cpp 選單疊層）。保留於此，使每個新 UI 字在內容
        // 不可讀的退路上仍可顯示，與上方結局標題同理。含遊戲標題＋選單動詞＋角色
        // 名稱／簡介。
        "尋傘記政大山下篇開始遊戲離開選擇你的角色"
        "五位學生沒性別之分確認返首頁繼續重新動"
        "夜貓通宵K書黑眼圈是勳章social咖系活開心果"
        "邊緣人圖館落常駐民卷王GPA行事曆排深佛隨修"
        "課丟急金幣元遊戲選單"
        // 物品欄面板
        "物品欄空"
        // 建築名稱所用字元：涵蓋 include/Buildings.h kAll 中每個建築名的字。View.cpp
        // 會繪製 "Inside: " + World::CurrentBuildingName()（建築 HUD 行）；任何建築名
        // 表意字若同時不在 docs/content 下的 *.md 與此字面集中，便會落到 raylib 無字符的
        // `?`——即曾回報的「建築物名字出現 ? 缺字」缺陷（井／仁／勇／塘／夫／志／泳／
        // 雩 當時缺漏）。此處烘入完整 56 字的建築名集合（而非只補當時缺的 8 個），使
        // 日後 Buildings.h 改名不致悄悄重現缺字建築名。圖集機制與理由同上方 U+25BC ▼
        // 修正與結局標題區塊，亦保留於內容不可讀的退路上。
        "中井仁勇務友合商四圖堂場塘大夫學小希廊心志思操政新智"
        "書服果校樂樓正法泳活游研究綜維聞育舖英行訊資走門院集雩風館體"
        // 共用「遊戲說明」文字所用字元：涵蓋 include/GameHelp.h kGameHelpLines +
        // kGameHelpClosing 的每個字，顯示於首頁說明頁與遊戲內「說明」疊層。多數已
        // 出現在 docs/content 下的 *.md，但 訪（拜訪）不在，缺它說明面板便會缺字成 `?`
        // ——圖集機制與上方建築名區塊同理。此處烘入完整說明字集（而非只補 訪），使
        // 日後說明文案修改不致悄悄重現缺字。另附加 暫停凍結壓力計 供選單（M）「暫停
        // 凍結雨勢」提示行；結 已含於 終，力／計 不在舊集合中。
        "三下不並久了他作偷動取向品善局屠屬建待復惡或戲戶把拿"
        "撐撿標欄沖消淋災物相破種積終綠者花術被訪購躲透進選醜量錢鍵龍"
        "暫停凍結壓力計"
        // 暫停選單的無障礙開關（「減少動畫 [開/關]」／「擴大目標 [開/關]」）。動／標／
        // 開 已烘入於上；減／少／畫／擴／目／關 於此補上，使這兩列開關在內容不可讀的
        // 退路上亦正確顯示（兩列每個字都須進圖集——理由同建築名／說明區塊）。
        "減少畫擴目關"
        // 結局畫面三選項選單（EndingView）：回首頁／重新開始／結束，外加
        // 「← → 選擇   E 確認」導覽提示。多數字（首／頁／重／新／開／始／結／選／擇／
        // 確／認 與箭頭）已烘入於上；僅 回（回首頁）與 束（結束）為新增且不在任何
        // docs/content 下的 *.md 中，故只在此進圖集。對應的字符掃描測試會列舉這些選單
        // 標籤，遇任何未涵蓋字即讓建置失敗，故此處為驗證而非臆測——機制同前。
        "回束"
        // 加強版結局「畫面」（EndingView.cpp）與「遊戲說明」中可操作的結局段落
        // （GameHelp.h）會繪製少數不在任何 docs/content 下的 *.md 中的字，故只在此進圖集。
        // 字符掃描測試會列舉結局卡字串＋說明行＋View 字面值，遇缺字即建置失敗，故此
        // 區塊為驗證而非臆測。當時缺漏的三個（其餘已由廣泛的內容／UI 字集涵蓋）：
        //   U+2500 ─  「── 為什麼你走到這裡 ──」理由段落的分隔線。
        //   U+2713 ✓  結算清單「條件達成」勾記。
        //   U+58AE 墮  僅出現於結局 B 的路徑標籤「墮落結局」。
        "\xE2\x94\x80"      // U+2500 ─
        "\xE2\x9C\x93"      // U+2713 ✓
        "\xE5\xA2\xAE"      // U+58AE 墮
        // 擴大後的字符掃描現已涵蓋每個由程式碼構成的 UI 字串——HUD 目標文字、
        // ItemCatalog 名稱與說明、Vendor 提示片段、新增的【雨傘外觀】說明行與結局
        // 字串。下列少數字出現在這些程式碼字面值中，卻不在任何 docs/content 下的 *.md 中，
        // 故只在此進圖集；掃描會加以驗證。
        //   北 目標「地圖東北」   垂 說明「傘面下垂」   紫 說明「暗紫色」
        //   幸 圖鑑「小確幸」     血 圖鑑「三頁心血」
        "北垂紫幸血"
        // 結局 D 卡片（EndingView）、ItemCatalog 效果說明（CatalogStrings）與第四章
        // 結局「自白」對話新用到、且不在任何 docs/content 下的 *.md 中的字，故只在此進
        // 圖集。字符掃描驗證結局卡與圖鑑表面；自白字一併烘入，使遊戲內 DialogView
        // 亦可正確顯示（兩條路徑皆不缺字）。
        //   U+2212 −  「雨量 −15/−25/−35」說明中的排版減號
        //   磨 結局 D「把傘磨破了」   壯 自白「骨架紮實」（紮 已涵蓋）
        //   握 自白「你握著…」「你的手指扣上」  彈／擋 圖鑑「彈開」「擋雨」
        //   鳴 自白「發出細微的嗡鳴」
        "\xE2\x88\x92"      // U+2212 −
        "磨壯握彈擋鳴"
        // 新增的【道具須知】說明段落（GameHelp.h kGameHelpPage2）所用的字。多數亦
        // 出現在 docs/content，但——依上方建築名區塊的同一教訓——此處烘入完整的新提示
        // 字集，使說明文字即便在內容不可讀的退路上仍可顯示，且日後文案修改不致悄悄
        // 重現缺字。兩個全形標點需明確 UTF-8（！ U+FF01／； U+FF1B），其餘為表意字。
        // 字符掃描會列舉 kGameHelpPages，遇任何未涵蓋字即建置失敗，故此區塊為驗證
        // 而非臆測。
        "跨節保留其餘道具只該效市清當耗多數可緩得用必使頭冒就接來去業默決定"
        "\xEF\xBC\x81"      // U+FF01 ！
        "\xEF\xBC\x9B"      // U+FF1B ；
        // 永久性的「不缺字」防線。對應的字面值掃描測試會掃過 src/ 與 include/ 下每個
        // "…" 字串字面值中的每個 CJK 碼位、以及每個 docs/content 下的 *.md，除非每個碼位皆
        // 在有效圖集中、且每個「僅出現在原始碼字面值」的字（在任何 .md 中都沒有，故
        // 在全新 clone 上內容載入路徑無法涵蓋）皆明確存在於此集合，否則建置失敗。該
        // 掃描找出下列幾個是唯一尚未烘入於上的「僅原始碼字面值」字：
        //   敬 — DLC 預告 ShowMessage「DLC開發中 / 敬請期待」（DlcSign.cpp）；曾反覆
        //        回報的缺字→`?`。
        //   刺 — 攤販招呼語「螢光綠到刺眼」（ChapterVendors.cpp）。
        //   君 — 道具拾取「A 君的名字貼紙」（ChapterQuestItems.h）。
        //   含／扶／毫 — DialogOpener.cpp 的旁白分支。
        //   央 — 置中提示輔助字串（ChapterToast.h）。
        //   櫃 — 置物櫃對話（Chapter2Quest.cpp／DialogOpener.cpp）。
        //   牽／羊 — CursedUmbrella.cpp 拾取台詞「順手牽羊」。
        // 兩個 CJK 彎引號「“」「”」（U+201C/D）是 DialogLoader／VendorLoader 的剖析
        // token（顯示前已剝除，實際不會渲染）——仍一併烘入，使全字面值掃描完整，且
        // 日後若直接渲染它們亦不致缺字。
        "敬刺君含扶毫央櫃牽羊"
        "\xE2\x80\x9C"      // U+201C "
        "\xE2\x80\x9D"      // U+201D "
        // 載入畫面（src/ui/LoadingScreen.cpp）的標籤「載入中…」／「正在準備政大山下的
        // 雨天…」。除 載（U+8F09）外每個字皆已由 docs/content 或上方區塊涵蓋；載 不在
        // 任何 docs/content 下的 *.md 中，故只在此進圖集（否則全字面值掃描會讓建置失敗——
        // 機制同前）。烘入於此，使載入標籤即便在內容不可讀的退路上仍可顯示。
        "載";
}

/**
 * @brief 蒐集需載入的相異碼位集合。
 * @return 去重後的碼位向量（已套上安全上限）。
 *
 * 必含 ASCII 32..126，再加上每個 docs/content 下的 *.md 檔的每個碼位，以及寫死的 UI
 * 字面值。若內容資料夾無法讀取，則改加一段廣泛的常用 CJK 範圍，使文字大致仍可顯示。
 */
inline std::vector<int> CollectCodepoints() {
    std::set<int> cps;
    for (int c = 32; c <= 126; ++c) cps.insert(c);

    auto add_utf8 = [&cps](const char* utf8) {
        if (!utf8 || utf8[0] == '\0') return;
        int count = 0;
        int* decoded = ::LoadCodepoints(utf8, &count);
        for (int i = 0; i < count; ++i) {
            if (decoded[i] > 0) cps.insert(decoded[i]);
        }
        ::UnloadCodepoints(decoded);
    };

    static const char* kContentFiles[] = {
        "chapter1.md", "chapter2.md", "chapter3.md", "chapter4.md",
        "ending_a.md", "ending_b.md", "ending_c.md",
        "interlude_market.md", "voice_bible.md",
    };
    bool any_content = false;
    // 遊戲從專案根目錄執行（View.cpp 使用相同的相對基準）；額外再試一個上層前綴
    // 以求保險。
    static const char* kBases[] = {"docs/content/", "../docs/content/"};
    for (const char* base : kBases) {
        for (const char* name : kContentFiles) {
            std::string path = std::string{base} + name;
            if (!::FileExists(path.c_str())) continue;
            char* text = ::LoadFileText(path.c_str());
            if (!text) continue;
            add_utf8(text);
            ::UnloadFileText(text);
            any_content = true;
        }
        if (any_content) break;
    }

    add_utf8(UiLiteralChars());

    if (!any_content) {
        // 內容不可讀（CWD 沒有 docs/content——從檔案管理員／IDE／點兩下啟動，或在
        // EnsureAssetWorkingDir 修正前從 build 內執行）。切勿在此烘入整段
        // U+4E00..U+9FFF：那約 2 萬個字，LoadFontEx 在 32 px 下會把它們塞進約 8192
        // 寬的圖集，超過 Intel／較舊 Mac 的 GL_MAX_TEXTURE_SIZE，造成壞掉的材質並在
        // 第一次 DrawTextEx（首頁／選角畫面）崩潰（即曾回報的「進去選角 crash」）。
        // 關鍵在於：內容不可讀時對話載入器（同一批檔案）也載不到任何東西，故無任何
        // 執行期文字需要那些表意字；精心整理的 UiLiteralChars() 已涵蓋每個由程式碼
        // 構成的 UI 字串。因此退路只烘入少量標點範圍，使圖集維持在與正常（內容存在）
        // 路徑相同的安全 ~2048² 大小。
        for (int c = 0x3000; c <= 0x303F; ++c) cps.insert(c);  // CJK 標點
        for (int c = 0xFF00; c <= 0xFFEF; ++c) cps.insert(c);  // 全形
    }

    std::vector<int> out(cps.begin(), cps.end());
    // 對圖集大小設硬性安全上限。32 px 下一個字格約 36 px，故 N 個字大致塞成
    // sqrt(N)*36 的方形：4096 個碼位 → 約 2304² 圖集，安穩落在自約 2008 年起每張
    // GPU 都保證的 4096 GL_MAX_TEXTURE_SIZE 下限之內。正常（內容存在）字集約
    // 1500–2100 碼位，故此上限絕不裁掉實際使用——僅用以圈住異常／失控的集合，使圖集
    // 永不達到驅動程式拒絕配置的大小（歷來啟動崩潰的成因）。
    constexpr std::size_t kMaxCodepoints = 4096;
    if (out.size() > kMaxCodepoints) out.resize(kMaxCodepoints);
    return out;
}

/**
 * @brief 持有已載入的字型與其有效旗標。
 *
 * 若以此型別的 function-local static 持有，會在 main() 回傳之後（即 Window 解構已
 * 呼叫 ::CloseWindow() 並拆掉 GL context 之後）才解構，而 UnloadFont 會觸碰 GL。
 * 因此拆除採明確方式（ShutdownFont，於視窗仍存活時呼叫），絕不靠 static 解構。
 */
struct FontState {
    ::Font font{};            ///< 已載入的字型
    bool   attempted{false};  ///< 是否已嘗試載入（保證至多載入一次）
    bool   loaded{false};     ///< 是否成功載入
};

/** @brief 取得程序級唯一的字型狀態。@return FontState 參考。 */
inline FontState& State() {
    static FontState s;
    return s;
}

} // namespace detail

/**
 * @brief 載入 CJK 字型一次。
 *
 * 必須在 InitWindow 之後（raylib 需要 GL context 來建立字符圖集材質）、第一次文字
 * 繪製之前呼叫。可重複呼叫（至多載入一次）。視窗未就緒時為 no-op——這正是無頭測試
 * 建置能保持綠燈的關鍵：測試下沒有 GL context，故字型不會載入、文字退回 raylib
 * 預設（`?`），在該情境下符合預期且無害。
 */
inline void EnsureFont() {
    detail::FontState& s = detail::State();
    if (s.attempted) return;
    if (!::IsWindowReady()) return;   // 無頭／InitWindow 之前的防護
    s.attempted = true;

    std::vector<int> cps = detail::CollectCodepoints();
    constexpr int kFontSize = 32;     // 以大尺寸點陣化；DrawTextEx 再縮小

    // 逐一嘗試磁碟上存在的每個候選，保留第一個能載入且含真實字符者。raylib／
    // stb_truetype 無法點陣化的 CFF .ttc 會回傳 glyphCount==0——略過它並繼續往下，
    // 而非就此放棄（曾導致清單後方明明有可用字型、中文卻仍是 `?` 的問題）。
    for (const std::string& path : detail::FontCandidates()) {
        if (!::FileExists(path.c_str())) continue;
        ::Font f = ::LoadFontEx(path.c_str(), kFontSize,
                                cps.data(), static_cast<int>(cps.size()));
        if (f.texture.id == 0 || f.glyphCount == 0) {
            ::UnloadFont(f);          // 無法解析的字型 → 試下一個
            continue;
        }
        ::SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
        s.font   = f;
        s.loaded = true;
        return;
    }
    // 無處有可解析字型 → 保留 ASCII 預設。把真實 .ttf 放到
    // resources/assets/fonts/cjk.ttf 即可保證 CJK 渲染。
}

/** @brief CJK 字型是否已載入。@return 已載入回傳 true。 */
inline bool IsCJKFontLoaded() { return detail::State().loaded; }

/** @brief 取得已載入的 CJK 字型。@return 字型參考；僅在 IsCJKFontLoaded() 為真時有效。 */
inline const ::Font& CJKFont() { return detail::State().font; }

/**
 * @brief 明確拆除字型。
 *
 * 須於 main() 中、視窗關閉之前（GL context 仍存活時）呼叫。具冪等性。
 */
inline void ShutdownFont() {
    detail::FontState& s = detail::State();
    if (s.loaded) {
        ::UnloadFont(s.font);
        s.loaded = false;
    }
}

} // namespace nccu::engine::render

#endif // GFX_FONT_H_
