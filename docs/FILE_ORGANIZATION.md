# 檔案組織提案 — include/ 與 src/ 子資料夾

## TL;DR

- 30+ 個 header 散在 `include/` 底層已經難掃 — **建議分子資料夾**。
- **CMakeLists 不用改半行**（`GLOB_RECURSE` 已經會遞迴撈 `.h` / `.cpp`）。
- 唯一的成本：每個 `#include "Foo.h"` 要改成 `#include "subfolder/Foo.h"` —— 一行 `sed` 就解掉。
- `include/gfx/` 已經是這種佈局的先例。

---

## 為什麼 CMakeLists 不會壞

看 `CMakeLists.txt:19-22`：

```cmake
file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
    "${SRC_DIR}/*.cpp"
    "${INCLUDE_DIR}/*.h"
)
```

- `GLOB_RECURSE` = 遞迴掃整棵樹，**深度不限**。把 `Player.h` 移到 `include/entities/player/Player.h` 都會被撈到。
- `target_include_directories(... PUBLIC "${INCLUDE_DIR}")` 只有 **一個** include root，所以引用方一律寫相對於 `include/` 的路徑。

實證：`include/gfx/Color.h` 在 src 端就是 `#include "gfx/Color.h"`，不需要在 CMakeLists 多加 include path。

---

## 提案 A：4 桶 + 已存在的 gfx（最小變動，推薦）

```
include/
├── gfx/           (已存在 — raylib wrapper)
├── entities/      GameObject, Character, Player, NPC, Item, TransparentUmbrella + 4衍生,
│                  ConsumableItem + 3衍生, CashPickup, Vendor, VendorConfig
├── world/         Buildings, Obstacles, Physics, BuildingTracker, WorldConfig
├── state/         SemesterState, SemesterStateMachine, Chapter1AddDrop, Chapter2Midterms,
│                  Chapter3SportsDay, Chapter4Finals, InterludeMarket
├── event/         EventBus
├── factory/       GameObjectFactory
└── ui/            CharacterSelect (一次性畫面)
```

- **遷移成本**：~30 headers → 改 ~50 處 `#include`。`sed` 一條搞定。
- **心智負擔**：低。每個資料夾名字直接對應「我在找這個概念」的問題。

---

## 提案 B：對映 SOLID review 的「Model / View / Controller」（理論最純，工程量大）

```
include/
├── gfx/
├── model/         GameObject, Character, Player, NPC, Item, Umbrella 群, Consumable 群,
│                  CashPickup, Vendor, VendorConfig, WorldConfig
├── view/          (空，未來放 renderer / HUD 模組)
├── controller/    EventBus, GameObjectFactory, SemesterStateMachine, BuildingTracker,
│                  Chapter 群, InterludeMarket, CharacterSelect
└── infra/         Buildings, Obstacles, Physics (純資料 + 靜態幾何)
```

- 直接讓助教看到 MVC 分層。
- 缺點：`controller` 變成大鍋飯（Lab9 警告 controller 易變 God），長期還是要再切。

---

## 不建議：扁平 + 後綴前綴命名

例：`Player_State.h`, `Umbrella_True.h`, `Chapter_AddDrop.h`。Windows 風格，掃起來反而更難。

---

## 遷移腳本（提案 A）

```bash
# 1. 建子資料夾
mkdir -p include/{entities,world,state,event,factory,ui}

# 2. git mv (保留歷史)
git mv include/Player.h include/entities/
git mv include/Character.h include/entities/
git mv include/GameObject.h include/entities/
git mv include/NPC.h include/entities/
git mv include/Item.h include/entities/
git mv include/TransparentUmbrella.h include/entities/
git mv include/{True,Fragile,ProfessorTrap,Cursed}Umbrella.h include/entities/
git mv include/ConsumableItem.h include/entities/
git mv include/{HotPack,WaterproofSpray,EnergyDrink}.h include/entities/
git mv include/CashPickup.h include/entities/
git mv include/Vendor.h include/entities/
git mv include/VendorConfig.h include/entities/

git mv include/{Buildings,Obstacles,Physics,BuildingTracker,WorldConfig}.h include/world/
git mv include/SemesterState{,Machine}.h include/state/
git mv include/Chapter{1AddDrop,2Midterms,3SportsDay,4Finals}.h include/state/
git mv include/InterludeMarket.h include/state/
git mv include/EventBus.h include/event/
git mv include/GameObjectFactory.h include/factory/
git mv include/CharacterSelect.h include/ui/

# 3. 對映 src 端
mkdir -p src/{entities,world,state,event,factory,ui}
# ... (git mv 同名 .cpp 進去；CMake 仍會掃到)

# 4. 一次性改 #include — 整個 src / tests / include 全掃
# 對每個移動過的 header 名字，把 #include "Foo.h" 換成 #include "subfolder/Foo.h"
# 範例（Player.h → entities/Player.h）：
grep -rl '#include "Player.h"' include/ src/ tests/ | xargs sed -i '' 's|#include "Player.h"|#include "entities/Player.h"|g'
# (重複 30 次，或寫 python 腳本一次解掉)

# 5. 重新 cmake configure（GLOB_RECURSE 抓新檔）
cmake -B build -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build build
ctest --test-dir build --output-on-failure
```

---

## 建議流程

1. **先別動** —— 等下一波功能 ready 再一次重組，避免合併衝突。
2. 一旦要動，**選提案 A**，分一個 PR 專門做 reorg，不要混 feature。
3. 用 `git mv` 保留歷史；用全自動 sed 改 `#include`；CMake 重新 configure 一次就行。
4. 寫到 commit message：`refactor(layout): group headers by domain (entities/world/state/...)`

---

## 注意事項

- **gfx 已經是子資料夾**，提案 A 直接沿用同個哲學，整體一致。
- `include/` 同名衝突風險：`Player.cpp` 找 `Player.h`，分開後寫 `entities/Player.h` 就一清二楚。
- 別碰 `resources/` —— 那是執行期 raylib 載入路徑，動就壞。
