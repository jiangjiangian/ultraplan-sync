---
id: file:include/engine/render/Font.h
type: header
path: include/engine/render/Font.h
domain: engine
bucket: render
loc: 341
classes: [FontState]
sources: ["include/engine/render/Font.h"]
---
# `Font.h`

> **一句定位**：程序級 CJK 字型管理器，透過候選清單與動態碼位蒐集載入一份涵蓋遊戲全部實際顯示字元的字型，解決跨平台 CJK 渲染與大圖集崩潰問題。

## 職責

此標頭是整個 CJK 文字渲染系統的核心，解決了多個歷史 bug：raylib 內建字型僅含 ASCII，直接顯示中文字串會渲染成 `?` / 方框。

**字型候選清單**（`detail::FontCandidates()`）：按優先序列出候選字型路徑（使用者自備 `resources/assets/fonts/cjk.ttf` / `.otf` / `.ttc` > macOS 系統字型（TrueType 輪廓者優先，CFF 輪廓的 PingFang / Hiragino 排後） > Linux Noto）。raylib 的 stb_truetype 無法點陣化 CFF 輪廓（PingFang / Hiragino 使用）會回傳 `glyphCount==0`；`EnsureFont` 逐一嘗試並跳過 `glyphCount==0` 者，保留第一個確實有字符的字型。

**碼位蒐集**（`detail::CollectCodepoints()`）：必含 ASCII 32..126；讀取 `docs/content/` 下所有 `.md` 對話檔（`chapter1~4 / ending_a~c / interlude_market / voice_bible`）的碼位；加入 `UiLiteralChars()` 的 UI 字面值集合（HUD / 結局卡 / 選單 / 建築名 / 說明文字等每個 CJK 字元均逐一收入並以詳細注解標明來源）。若內容不可讀（退路），只加 CJK 標點 / 全形範圍（不加 U+4E00..U+9FFF，防止 GL 圖集超出 `GL_MAX_TEXTURE_SIZE`）。碼位上限硬限為 4096，確保圖集在歷史最低 `GL_MAX_TEXTURE_SIZE=4096` 下安全。

**`UiLiteralChars()`**：極為詳盡的 UI 字元集，按模組逐區塊收錄，涵蓋結局標題、HUD 目標、選單動詞、角色名稱、建築名（56 字完整集）、遊戲說明、物品欄、暫停選單、結局選項、DLC 預告、攤販台詞、對話旁白等每一個不在 `docs/content` 中出現的字。每區塊均有注解說明「為何這個字需要在此烘入而非靠內容讀取路徑」。

**`EnsureFont()`**：主入口，`IsWindowReady()` 保護（無頭 / 測試不載入），最多載入一次（`attempted` 旗標），逐一嘗試候選，對 `glyphCount==0` 的候選略過繼續。字型以 32px 點陣化，`TEXTURE_FILTER_BILINEAR` 使縮小時平滑。

**`FontState`**（struct）：持有 `::Font font`、`attempted`、`loaded`；以 function-local static 形式存在（`detail::State()`），但拆除採明確方式（`ShutdownFont()`，在視窗關閉前呼叫），絕不靠 static 解構（`::UnloadFont` 需要 GL context 仍存活）。

## 關鍵內容（類別 / 函式 / 資料）

- **`nccu::engine::render::detail::FontCandidates() → const vector<string>&`**（inline）：CJK 字型候選清單（static local）。
- **`nccu::engine::render::detail::UiLiteralChars() → const char*`**（inline）：遊戲 UI 字面值 CJK 字元集（含詳盡的逐區塊注解）。
- **`nccu::engine::render::detail::CollectCodepoints() → vector<int>`**（inline）：蒐集去重後的碼位集（ASCII + 內容 .md + UI 字面值，上限 4096）。
- **`nccu::engine::render::detail::FontState`**（struct）：`font`（`::Font`）、`attempted`（bool）、`loaded`（bool）。
- **`nccu::engine::render::detail::State() → FontState&`**（inline）：程序級唯一 FontState。
- **`nccu::engine::render::EnsureFont() → void`**（inline）：主字型載入入口；冪等。
- **`nccu::engine::render::IsCJKFontLoaded() → bool`**（inline）。
- **`nccu::engine::render::CJKFont() → const ::Font&`**（inline）：取得已載入字型（僅 `IsCJKFontLoaded` 時有效）。
- **`nccu::engine::render::ShutdownFont() → void`**（inline）：明確拆除；須在視窗關閉前呼叫。

## 相依與在架構中的位置

- **#include（往外）**：`raylib.h`（`LoadFontEx / UnloadFont / FileExists / LoadCodepoints / SetTextureFilter` 等）；標準庫 `<algorithm>`, `<array>`, `<set>`, `<string>`, `<vector>`——所有 raylib 使用均限制在此標頭（引擎 render 層）。
- **被誰使用（往內）**：`include/engine/render/TextBuilder.h`（使用 `CJKFont`）、`src/app/main.cpp`（呼叫 `EnsureFont / ShutdownFont`）；測試 `test_font_ui_glyph_scan / test_font_ui_glyphs / test_font_ui_literal_scan`（驗證 UI 字元集的完整性）。
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：engine/render 層；程式啟動時呼叫一次（`EnsureFont`），關閉時呼叫一次（`ShutdownFont`），不參與每幀管線。

## OO 概念與設計重點

`FontState` 搭配 function-local static（`detail::State()`）是一個受控的懶加載 Singleton——但特意避免「static 解構在 GL context 消失後」的問題，改用明確的 `ShutdownFont()` 拆除，體現了「RAII 不一定靠 scope 結束，可以是明確 shutdown」的設計決策。

`EnsureFont` 的「逐候選嘗試 + `glyphCount == 0` 跳過」策略解決了跨平台 CJK 字型相容性問題；碼位上限 4096 + 精確蒐集（而非整段 U+4E00..U+9FFF）解決了歷史 `GL_MAX_TEXTURE_SIZE` 崩潰。字符掃描測試（`test_font_ui_literal_scan`）把「UI 字符完整性」轉成自動化驗證，這種「測試是唯一保證」的設計使 `UiLiteralChars()` 中每個字的存在都有依據。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/engine/render/Font.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/render/Font.h) · [← 全檔索引](../files-index.md) · 相關概念：[DIP：IRenderer](../concepts/arch-dip-renderer.md) · [Singleton](../concepts/pat-singleton.md)
