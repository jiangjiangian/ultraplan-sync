---
id: arch-harness
type: architecture
title: 決定性 autoplay（Harness）
sources: [docs/UML/5-harness.md, docs/UML/8-solid.md]
---

# 決定性 autoplay（Harness） · 架構元件

> InputSource 抽象（LiveInput/ScriptInput）＋ Time 固定 1/60 步 ⇒ 同腳本 byte-identical state.jsonl；正常遊玩旁路。

## 是什麼 / 怎麼運作

感知＋致動的縫合層，預設關閉（無 `UMBRELLA_SCRIPT` 環境變數時旁路，正常遊玩 bit-for-bit 不變）。啟用時把輸入換成 deterministic 的 `ScriptInput`（經 `InputSource` 抽象，對應 `LiveInput`）、時間換成 `Time` 固定 1/60 步，每幀輸出一行 JSON 狀態 ⇒ 同腳本產生 byte-identical `state.jsonl`，讓自動遊玩可重播、可回歸測試。

## 落點（程式碼）

| 檔案 | 類別 | 連結 |
|---|---|---|
| `include/engine/input/Input.h` | `InputSource`, `LiveInput`, `Input` | [node](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/engine/input/Input.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/input/Input.h) |
| `include/engine/platform/Harness.h` | `Harness` | [node](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/engine/platform/Harness.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/platform/Harness.h) |
| `include/engine/platform/ScriptInput.h` | `ScriptInput`, `Directive`, `Step` | [node](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/engine/platform/ScriptInput.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/platform/ScriptInput.h) |
| `include/engine/platform/Time.h` | `Time` | [node](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/engine/platform/Time.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/platform/Time.h) |

## 相關概念

[MVC 核心](arch-mvc.md) · [RAII / 記憶體安全](oo-raii.md)

## 來源（設計文件）

[`docs/UML/5-harness.md`](../../../docs/UML/5-harness.md) · [`docs/UML/8-solid.md`](../../../docs/UML/8-solid.md)

---
[← wiki 索引](../index.md) · [🕸 互動圖譜](https://jiangjiangian.github.io/ultraplan-sync/#node=arch-harness)