---
id: oo-isp-roles
type: principle
title: 角色介面（ISP）
sources: [docs/UML/1-entities.md, docs/UML/8-solid.md]
---

# 角色介面（ISP） · OO 原則 / 技法

> Roles.h 把胖介面拆成 IUpdatable / IDrawable / IInteractable / IMortal，葉類別只實作扮演的角色。

## 是什麼 / 怎麼運作

`Roles.h` 把『一個 GameObject 該會什麼』從一個胖介面拆成四個小角色介面：`IUpdatable`（會動）、`IDrawable`（會畫）、`IInteractable`（可互動）、`IMortal`（有血量/會死）。道具不必實作 `Update`、看板不必 `IMortal`——只實作自己扮演的角色（ISP）。`IMortal` 是 Assignment #6 戰鬥的鋪路，目前只有 `Player` 扮演。

## 落點（程式碼）

| 檔案 | 類別 | 連結 |
|---|---|---|
| `include/engine/core/Roles.h` | `IUpdatable`, `IDrawable`, `IInteractable`, `IMortal`, `WithRoles` | [node](../../index.html#node=file:include/engine/core/Roles.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/core/Roles.h) |

## 相關概念

[CRTP static mixin](oo-crtp.md) · [Strategy / Pipeline](pat-strategy.md)

## 來源（設計文件）

[`docs/UML/1-entities.md`](../../../docs/UML/1-entities.md) · [`docs/UML/8-solid.md`](../../../docs/UML/8-solid.md)

---
[← wiki 索引](../index.md) · [🕸 互動圖譜](../../index.html#node=oo-isp-roles)