---
id: domain-engine
type: domain
title: engine
---

# 領域：engine · 引擎層

> 與遊戲內容無關、可重用的引擎層：`core`（GameObject / Roles 基礎型別）、`events`（EventBus）、`render`（IRenderer / Raylib 包裝）、`input`、`math`、`audio`、`platform`（harness / Time）。**鐵律：engine 不反向相依 game / ui。**

共 **35** 個檔案，分 7 個 bucket。[在互動圖譜中檢視 →](../../index.html#node=domain:engine)

## engine/audio  (4)

| 檔案 | 類別 | 連結 |
|---|---|---|
| `include/engine/audio/AudioDevice.h` | `AudioDevice` | [node](../../index.html#node=file:include/engine/audio/AudioDevice.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/audio/AudioDevice.h) |
| `include/engine/audio/AudioManager.h` | `AudioManager` | [node](../../index.html#node=file:include/engine/audio/AudioManager.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/audio/AudioManager.h) |
| `src/engine/audio/AudioDevice.cpp` | — | [node](../../index.html#node=file:src/engine/audio/AudioDevice.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/engine/audio/AudioDevice.cpp) |
| `src/engine/audio/AudioManager.cpp` | — | [node](../../index.html#node=file:src/engine/audio/AudioManager.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/engine/audio/AudioManager.cpp) |

## engine/core  (2)

| 檔案 | 類別 | 連結 |
|---|---|---|
| `include/engine/core/GameObject.h` | `GameObject` | [node](../../index.html#node=file:include/engine/core/GameObject.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/core/GameObject.h) |
| `include/engine/core/Roles.h` | `IUpdatable`, `IDrawable`, `IInteractable`, `IMortal`, `WithRoles` | [node](../../index.html#node=file:include/engine/core/Roles.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/core/Roles.h) |

## engine/events  (5)

| 檔案 | 類別 | 連結 |
|---|---|---|
| `include/engine/events/EventBus.h` | `Event`, `EventBus`, `Subscription`, `Slot` | [node](../../index.html#node=file:include/engine/events/EventBus.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/events/EventBus.h) |
| `include/engine/events/EventSink.h` | — | [node](../../index.html#node=file:include/engine/events/EventSink.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/events/EventSink.h) |
| `include/engine/events/HudSlot.h` | — | [node](../../index.html#node=file:include/engine/events/HudSlot.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/events/HudSlot.h) |
| `src/engine/events/EventBus.cpp` | — | [node](../../index.html#node=file:src/engine/events/EventBus.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/engine/events/EventBus.cpp) |
| `src/engine/events/EventSink.cpp` | — | [node](../../index.html#node=file:src/engine/events/EventSink.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/engine/events/EventSink.cpp) |

## engine/input  (2)

| 檔案 | 類別 | 連結 |
|---|---|---|
| `include/engine/input/Input.h` | `InputSource`, `LiveInput`, `Input` | [node](../../index.html#node=file:include/engine/input/Input.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/input/Input.h) |
| `include/engine/input/Key.h` | — | [node](../../index.html#node=file:include/engine/input/Key.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/input/Key.h) |

## engine/math  (3)

| 檔案 | 類別 | 連結 |
|---|---|---|
| `include/engine/math/Color.h` | `Color` | [node](../../index.html#node=file:include/engine/math/Color.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/math/Color.h) |
| `include/engine/math/Rect.h` | `Rect` | [node](../../index.html#node=file:include/engine/math/Rect.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/math/Rect.h) |
| `include/engine/math/Vec2.h` | `Vec2` | [node](../../index.html#node=file:include/engine/math/Vec2.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/math/Vec2.h) |

## engine/platform  (7)

| 檔案 | 類別 | 連結 |
|---|---|---|
| `include/engine/platform/Harness.h` | `Harness` | [node](../../index.html#node=file:include/engine/platform/Harness.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/platform/Harness.h) |
| `include/engine/platform/ScriptInput.h` | `ScriptInput`, `Directive`, `Step` | [node](../../index.html#node=file:include/engine/platform/ScriptInput.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/platform/ScriptInput.h) |
| `include/engine/platform/Time.h` | `Time` | [node](../../index.html#node=file:include/engine/platform/Time.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/platform/Time.h) |
| `include/engine/platform/WorkingDir.h` | — | [node](../../index.html#node=file:include/engine/platform/WorkingDir.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/platform/WorkingDir.h) |
| `src/engine/platform/Harness.cpp` | `HarnessState` | [node](../../index.html#node=file:src/engine/platform/Harness.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/engine/platform/Harness.cpp) |
| `src/engine/platform/ScriptInput.cpp` | — | [node](../../index.html#node=file:src/engine/platform/ScriptInput.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/engine/platform/ScriptInput.cpp) |
| `src/engine/platform/ScriptResolver.cpp` | — | [node](../../index.html#node=file:src/engine/platform/ScriptResolver.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/engine/platform/ScriptResolver.cpp) |

## engine/render  (12)

| 檔案 | 類別 | 連結 |
|---|---|---|
| `include/engine/render/Camera2D.h` | `Camera2D` | [node](../../index.html#node=file:include/engine/render/Camera2D.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/render/Camera2D.h) |
| `include/engine/render/CameraScope.h` | `CameraScope` | [node](../../index.html#node=file:include/engine/render/CameraScope.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/render/CameraScope.h) |
| `include/engine/render/DrawScope.h` | `DrawScope` | [node](../../index.html#node=file:include/engine/render/DrawScope.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/render/DrawScope.h) |
| `include/engine/render/Font.h` | `FontState` | [node](../../index.html#node=file:include/engine/render/Font.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/render/Font.h) |
| `include/engine/render/IRenderer.h` | `IRenderer` | [node](../../index.html#node=file:include/engine/render/IRenderer.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/render/IRenderer.h) |
| `include/engine/render/ImageDecoder.h` | `DecodedImage` | [node](../../index.html#node=file:include/engine/render/ImageDecoder.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/render/ImageDecoder.h) |
| `include/engine/render/RaylibRenderer.h` | `RaylibRenderer` | [node](../../index.html#node=file:include/engine/render/RaylibRenderer.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/render/RaylibRenderer.h) |
| `include/engine/render/Renderer.h` | `Renderer`, `nccu`, `nccu`, `nccu` | [node](../../index.html#node=file:include/engine/render/Renderer.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/render/Renderer.h) |
| `include/engine/render/TextBuilder.h` | `TextBuilder`, `nccu`, `nccu`, `nccu` | [node](../../index.html#node=file:include/engine/render/TextBuilder.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/render/TextBuilder.h) |
| `include/engine/render/Texture.h` | `Texture`, `TextureCache` | [node](../../index.html#node=file:include/engine/render/Texture.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/render/Texture.h) |
| `include/engine/render/Window.h` | `Window`, `Builder` | [node](../../index.html#node=file:include/engine/render/Window.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/engine/render/Window.h) |
| `src/engine/render/ImageDecoder.cpp` | — | [node](../../index.html#node=file:src/engine/render/ImageDecoder.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/engine/render/ImageDecoder.cpp) |

---
[← wiki 索引](../index.md)