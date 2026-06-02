---
id: file:include/game/vendor/VendorConfig.h
type: header
path: include/game/vendor/VendorConfig.h
domain: game
bucket: vendor
loc: 85
classes: [VendorItem, VendorConfig]
sources: ["include/game/vendor/VendorConfig.h"]
---
# `VendorConfig.h`

> **一句定位**：攤販的純資料 POD 結構體定義——`VendorItem`（單品）與 `VendorConfig`（整攤）——刻意解耦遊戲執行期標頭，讓內容層能以字面值建構攤位。

## 職責

`VendorConfig.h` 定義兩個 POD 結構體，是攤販系統的資料核心。設計目標是讓內容層（`main.cpp`、腳本載入器）能以最小的相依建出攤位庫存表，無需引入任何遊戲執行期標頭。

`VendorItem` 描述單一可販售商品：`itemId`（商品代號，如 `"HotPack"`、`"TrueUmbrella"`）、`price`（金錢花費）、`stockLeft`（-1=無限；>0 每次購買遞減；0=售罄，TryBuy 在扣款前即失敗）、`setsFlag`（購買成功時 SetFlag 的旗標，預設空字串）。`setsFlag` 讓集英樓醜傘能設 `Flag_BoughtUglyUmbrella`（Ending_C 觸發條件）。

`VendorConfig` 聚合整攤設定，前三個成員（`name`、`greeting`、`stock`）是核心且順序固定（`VendorConfig{name, greeting, stock}` 的聚合初始化為釘住的契約，test_vendor 與 GameObjectFactory 依賴此）。後續成員皆帶明確預設值（`{}`/`= 0`），確保 3 欄位字面值不觸發 `-Wmissing-field-initializers` 警告。附加欄位包含：`stallKeeper`（攤主名）、`tier`（設計分組 1–4）、`mechanic`（buy/donate/sell/game/flavor 機制）、`karmaOnInteract`（成交業力）、`greetingLines`（多行問候）、`onPurchase`（成交台詞）、`onLeave`（道別台詞）、`spriteOverride`（整張圖 sprite 路徑，空時用 Pipoya 人物退路）、`npcId`（透傳給基底 NPC 的身分，空時與舊版一致）。

`npcId` 欄位尤其重要：設定後攤販能參與以 `npcId` 為鍵的任務「!」判定——Ch2 圖書館地下室自販機用 `kNpcCh2Vendor` 在玩家缺提神飲料時亮燈指引。

## 關鍵內容（類別 / 函式 / 資料）

- `struct VendorItem`：`itemId`（string）、`price`（int）、`stockLeft`（int，預設-1）、`setsFlag`（string，預設空）。
- `struct VendorConfig`：`name`/`greeting`/`stock`（核心三成員，聚合初始化契約）＋ 7 個帶預設值的附加欄位（`stallKeeper`、`tier`、`mechanic`、`karmaOnInteract`、`greetingLines`、`onPurchase`、`onLeave`）＋ `spriteOverride`＋ `npcId`。

## 相依與在架構中的位置

- **#include（往外）**：僅標準庫 `<string>`、`<vector>`（無遊戲執行期標頭）
- **被誰使用（往內）**：`include/game/quest/ChapterVendors.h`（`VendorPlacement`）、`include/game/vendor/Vendor.h`、`include/game/vendor/VendorLoader.h`；`src/game/controller/GameObjectFactory.cpp`；測試（`test_roles.cpp`、`test_vendor.cpp`、`test_vendor_inventory.cpp`）
- **繼承 / 實作 / 體現**：—
- **每幀管線 / MVC 角色**：Model 層純資料——攤販配置的最底層 DTO，由 `Vendor` 建構式消費，被 `ChapterVendors` 用於配置表，被 `VendorLoader` 解析填充。

## OO 概念與設計重點

「解耦內容層與執行期」的設計：通過無任何遊戲標頭依賴的 POD 設計，讓 `main.cpp` 或腳本載入器能直接以聚合初始化字面值建出攤位，不污染建構依賴圖。`VendorItem::stockLeft` 與 `VendorConfig` 附加欄位的「帶預設值成員」模式是抑制 `-Wmissing-field-initializers` 警告的標準 C++ 技巧，注釋中明確說明「切勿移除 `{}`/`=`」以防止未來維護者誤刪。聚合初始化契約（前三成員固定順序）是一種向後相容保證，讓舊版 3 欄位字面值在加入新欄位後仍能原封不動編譯。

## 連結
[🕸 圖譜節點](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/game/vendor/VendorConfig.h) · [↗ 原始碼](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/vendor/VendorConfig.h) · [← 全檔索引](../files-index.md)
