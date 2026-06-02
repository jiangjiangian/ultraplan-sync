---
id: domain-app
type: domain
title: app
---

# 領域：app · 組裝根

> 組裝根與場景框架。`main.cpp`（composition root）把 Window / Font / Loading / 主迴圈組起來；`IScene` / `SceneManager` / 各 scene 提供場景切換骨架。相依方向最外層：**app → game / ui / engine**。

共 **14** 個檔案，分 2 個 bucket。[在互動圖譜中檢視 →](https://jiangjiangian.github.io/ultraplan-sync/#node=domain:app)

## app/(根)  (6)

| 檔案 | 類別 | 連結 |
|---|---|---|
| `include/app/IScene.h` | `SceneCommand`, `IScene` | [node](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/app/IScene.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/app/IScene.h) |
| `include/app/SceneBootstrap.h` | — | [node](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/app/SceneBootstrap.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/app/SceneBootstrap.h) |
| `include/app/SceneManager.h` | `SceneManager` | [node](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/app/SceneManager.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/app/SceneManager.h) |
| `src/app/SceneBootstrap.cpp` | — | [node](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/app/SceneBootstrap.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/app/SceneBootstrap.cpp) |
| `src/app/SceneManager.cpp` | — | [node](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/app/SceneManager.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/app/SceneManager.cpp) |
| `src/app/main.cpp` | — | [node](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/app/main.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/app/main.cpp) |

## app/scenes  (8)

| 檔案 | 類別 | 連結 |
|---|---|---|
| `include/app/scenes/CharacterSelectScene.h` | `CharacterSelectScene` | [node](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/app/scenes/CharacterSelectScene.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/app/scenes/CharacterSelectScene.h) |
| `include/app/scenes/GameplayScene.h` | `GameplayScene` | [node](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/app/scenes/GameplayScene.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/app/scenes/GameplayScene.h) |
| `include/app/scenes/LoadingScene.h` | `LoadingScene` | [node](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/app/scenes/LoadingScene.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/app/scenes/LoadingScene.h) |
| `include/app/scenes/TitleScene.h` | `TitleScene` | [node](https://jiangjiangian.github.io/ultraplan-sync/#node=file:include/app/scenes/TitleScene.h) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/include/app/scenes/TitleScene.h) |
| `src/app/scenes/CharacterSelectScene.cpp` | — | [node](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/app/scenes/CharacterSelectScene.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/app/scenes/CharacterSelectScene.cpp) |
| `src/app/scenes/GameplayScene.cpp` | — | [node](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/app/scenes/GameplayScene.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/app/scenes/GameplayScene.cpp) |
| `src/app/scenes/LoadingScene.cpp` | — | [node](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/app/scenes/LoadingScene.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/app/scenes/LoadingScene.cpp) |
| `src/app/scenes/TitleScene.cpp` | `MenuItem` | [node](https://jiangjiangian.github.io/ultraplan-sync/#node=file:src/app/scenes/TitleScene.cpp) · [src](https://github.com/jiangjiangian/ultraplan-sync/blob/main/src/app/scenes/TitleScene.cpp) |

---
[← wiki 索引](../index.md)