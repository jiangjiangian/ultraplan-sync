# resources/ — 執行期資產

本資料夾存放《尋傘記：政大山下篇》**實際隨附並由程式載入**的素材。
CMake 在建置時會把整個 `resources/` 複製到執行檔旁，遊戲再以相對工作目錄載入。
所有檔案都在 `resources/assets/` 之下。

## 目錄樹

```text
.
└── assets/                                 所有隨附素材（CMake 複製到執行檔旁）
    ├── sprites/                         (35)   角色圖
    │   ├── school_uniform_3/                         31 張 Pipoya 風格學生制服角色表（玩家／NPC）
    │   ├── npc/                                      shop_auntie / suit_senior / ta 三個劇情 NPC
    │   └── ATTRIBUTIONS.md                           素材出處標注
    ├── buildings_3d_trimmed/            (29)   政大校園各建築去白底 sprite（trim_3d.py 預處理）
    ├── Pixel Art Vending Machines Pack/ (45)   攤販／販賣機美術包（含授權 txt），供 game/vendor
    ├── decorations/                     (2)    場景裝飾 strip：cat_strip.png / chiikawa_strip.png
    ├── fonts/                           (2)    cjk.ttf 中日韓字型 + OFL.txt 授權
    └── maps/                            (9)    worldmap*.png 世界底圖／合成／縮圖 + collision_mask*.png 碰撞遮罩
```

> 這些是版控、會隨 clone 取得且遊戲執行期會載入的素材子集；
> `tools/` 下的 Python 腳本負責由原始素材生成上述產物，但本身非執行期相依。
