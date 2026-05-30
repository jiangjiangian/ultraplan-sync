# resources/ — 執行期資產

本資料夾存放《尋傘記：政大山下篇》**實際隨附並由程式載入**的素材。
CMake 在建置時會把整個 `resources/` 複製到執行檔旁，遊戲再以相對工作目錄載入。
所有檔案都在 `resources/assets/` 之下。

## resources/assets/ 子目錄

- **sprites/**（35）— 角色圖。`school_uniform_3/`（31 張 Pipoya 風格學生制服角色表，
  供玩家／NPC 使用）與 `npc/`（shop_auntie / suit_senior / ta 三個劇情 NPC），
  另含 `ATTRIBUTIONS.md` 標注出處。
- **buildings_3d_trimmed/**（29）— 政大校園各建築的去背（去白底）sprite，
  如商學院、四維堂、行政大樓、體育館…，由 `tools/trim_3d.py` 預先處理。
- **Pixel Art Vending Machines Pack/**（45）— 攤販／販賣機美術包（含授權 txt），
  供 `game/vendor` 的攤販呈現使用。
- **decorations/**（2）— 場景裝飾動畫 strip：`cat_strip.png`、`chiikawa_strip.png`。
- **fonts/**（2）— `cjk.ttf` 中日韓字型（介面與對白文字渲染所需），加上 `OFL.txt` 授權。
- **maps/**（9）— 世界地圖與碰撞資料：`worldmap*.png`（世界底圖／合成圖／縮圖）
  與 `collision_mask*.png`（`game/world` 載入用的碰撞遮罩）。

> 這些是版控、會隨 clone 取得且遊戲執行期會載入的素材子集；
> `tools/` 下的 Python 腳本負責由原始素材生成上述產物，但本身非執行期相依。
