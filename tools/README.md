# tools/ — 資產與地圖管線（Python）

本資料夾收錄離線的 **資產／地圖前處理腳本**，用於從原始素材生成遊戲實際載入的
sprite、世界地圖與碰撞遮罩，並產生分析圖。這些都是**本地一次性工具**，**非執行期相依**：
遊戲執行時不會呼叫它們，產物已預先烘焙進 `resources/`。

## 目錄樹（8 支腳本）

```text
.
├── tiled_to_world.py                     由 Tiled 地圖輸出 Buildings.h 的 kAll[] 並烘焙地形碰撞遮罩
├── composite_worldmap.py                 把所有建築 tile 合成到世界底圖，產出完整 worldmap
├── trim_tiles.py                         一次性去背：俯視角建築 PNG 去灰底（buildings_topdown/）
├── trim_3d.py                            一次性去背：3D 建築 PNG 去白底 → buildings_3d_trimmed/
├── strip_plots.py                        清掉 worldmap_base.png 上的米色建築基地，改塗草地（保留底圖供重組）
├── gif_to_strip.py                       把動態 GIF 轉成遊戲用的水平 sprite strip（橫向連續幀）PNG
├── text_map.py                           產生「文字 ↔ 旗標 ↔ 程式碼」相依關係圖
└── docs_graph.py                         從文件擷取關聯結構的知識圖譜萃取器
```
