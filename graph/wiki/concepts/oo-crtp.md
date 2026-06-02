---
id: oo-crtp
type: principle
title: CRTP static mixin
sources: [docs/UML/1-entities.md, docs/UML/8-solid.md]
---

# CRTP static mixin · OO 原則 / 技法

> WithRoles<Derived,Base> 以 std::derived_from + if constexpr 在編譯期實作 As*() 能力查詢，取代 dynamic_cast。

## 是什麼 / 怎麼運作

`WithRoles<Derived, Base>` 是插在 `Character`/`Item` 與葉類別之間的 CRTP mixin。它在**編譯期**用 `std::derived_from` + `if constexpr` 判斷 `Derived` 扮演哪些角色介面，產生 `AsUpdatable()`/`AsDrawable()`/`AsInteractable()`/`AsMortal()`：命中回傳 typed pointer、未扮演回傳 `nullptr`——全程**無 `dynamic_cast`**、無 RTTI 成本。場景容器只持 `GameObject*`，靠這些 `As*()` 做多型分派（LSP）。

## 落點（程式碼）

| 檔案 | 類別 | 連結 |
|---|---|---|
| `include/engine/core/Roles.h` | `IUpdatable`, `IDrawable`, `IInteractable`, `IMortal`, `WithRoles` | [node](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/engine/core/Roles.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/core/Roles.h) |

## 相關概念

[角色介面（ISP）](oo-isp-roles.md) · [MVC 核心](arch-mvc.md)

## 來源（設計文件）

[`docs/UML/1-entities.md`](../../../docs/UML/1-entities.md) · [`docs/UML/8-solid.md`](../../../docs/UML/8-solid.md)

---
[← wiki 索引](../index.md) · [🕸 互動圖譜](https://jiangjiangian.github.io/ultraplan-sync/#node=oo-crtp)