---
id: pat-command
type: pattern
title: Command / Table（資料化）
sources: [docs/UML/3-mvc-isystem.md, docs/UML/7-gof.md]
---

# Command / Table（資料化） · 設計模式 (GoF)

> QuestHookTable 把 ~14 個內嵌互動鉤子改為一張有序、自我把關的資料表（OCP）。

## 是什麼 / 怎麼運作

E 互動的任務副作用原本是約 14 個內嵌 `TryXxx()` 呼叫；改成一張資料化的 `QuestHookTable`：每個鉤子是一筆 `QuestHook`（條件 + 動作），由 `RunInteractHooks(player, npcId, state, returnTo)` 依序、自我把關地執行。新增章節劇情＝`RegisterHook` 加一行，不動既有控制流（OCP）。

## 落點（程式碼）

| 檔案 | 類別 | 連結 |
|---|---|---|
| `include/game/quest/QuestHookTable.h` | `QuestHook` | [node](../../index.html#node=file:include/game/quest/QuestHookTable.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/game/quest/QuestHookTable.h) |

## 相關概念

[State](pat-state.md) · [MVC 核心](arch-mvc.md)

## 來源（設計文件）

[`docs/UML/3-mvc-isystem.md`](../../../docs/UML/3-mvc-isystem.md) · [`docs/UML/7-gof.md`](../../../docs/UML/7-gof.md)

---
[← wiki 索引](../index.md) · [🕸 互動圖譜](../../index.html#node=pat-command)