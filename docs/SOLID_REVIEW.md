# SOLID 對抗審查總合 — 《尋傘記：政大山下篇》

> 兩階段審查紀錄：5 個 architecture reviewer × SOLID 各 1，再對所有結論做一輪對抗反駁推回去。
> 最後一欄是綜合判決與行動順序。

---

## 整體結論

| 原則 | Reviewer 結論 | 對抗審查意見 | 綜合判決 |
|---|---|---|---|
| **S** | `main.cpp` god / Player 5 軸 / Vendor 3 軸（HIGH×3） | OVER-ENGINEERED — 拆 4 個類別反而失焦 | **延後拆 Player；main 在交件前不動；找 對抗審查新發現的兩個真議題（EventBus.Publish 政策、Umbrella 三軸混合）** |
| **O** | Factory enum+switch（HIGH） / state machine enum 可接受 | 對抗審查明確 REJECT registry refactor：static-init-order 風險高，rubric 不在乎 dispatch 風格，11 個 closed cases 不值得換工具 | **保留 enum+switch（second-pass 立場確認）；main 的 building-string mapping 改 unordered_map（已做）；dynamic_cast 兩處下次再修** |
| **L** | Draw 事件化破壞契約 / Cursed 副作用驚喜（HIGH×3） | 4/5 標 FALSE_POSITIVE（架構契約允許 event-driven 渲染） | **ProfessorTrapUmbrella 加 idempotency guard（對抗審查新發現的 REAL_VIOLATION）；其餘維持** |
| **I** | 留 fat base + 文件化 | 給出乾淨反設計：lifecycle-only base + IUpdatable/IDrawable/IInteractable + dynamic_cast 派發 | **這次先不動（時程不夠），但**將對抗審查的反設計記入 awsome_cpp.md 作為「下一版的方向」 |
| **D** | EventBus singleton sprawl + Factory 知道所有 concrete（HIGH×2） | EventBus injection 成本太高（14 類別） — ACCEPT_AS_PRAGMATIC；micro-split IPlayerState 是 ISP 噪音 | **只動 main 的 building→state 字串 mapping；EventBus 保留 singleton；Player/Item 互動不拆介面** |

---

## 立即行動（TIGHTEN_NOW，本週做）

### 1. `ProfessorTrapUmbrella::beClaimed` 加 idempotency guard
**理由：** 第二輪 LSP 對抗審查唯一的 REAL_VIOLATION。如果這個 umbrella 被連點兩下，會重覆觸發 TA spawn event。
```cpp
// src/ProfessorTrapUmbrella.cpp
void ProfessorTrapUmbrella::beClaimed(Player* player) {
    if (spawnedEnemiesCount_ > 0) return;        // 防止重入
    spawnedEnemiesCount_ = 3;
    EventBus::Instance().Publish(...);
}
```

### 2. `main.cpp` 樓宇 → SemesterState 字串映射搬到 unordered_map
**理由：** 兩輪審查 都同意這是 live policy 不該在 `main` 裡。
```cpp
static const std::unordered_map<std::string, SemesterState> kEnterTrigger = {
    {"正門",       SemesterState::Chapter1_AddDrop},
    {"樂活小舖",   SemesterState::Interlude_Market},
    {"中正圖書館", SemesterState::Chapter2_Midterms},
    {"操場",       SemesterState::Chapter3_SportsDay},
    {"行政大樓",   SemesterState::Chapter4_Finals},
};
// subscribing lambda：
if (auto it = kEnterTrigger.find(e.text); it != kEnterTrigger.end()) {
    semester.Transition(it->second);
}
```

### 3. `main.cpp` 兩處 `dynamic_cast` 改 virtual predicate
**理由：** OCP 違規最小修；不破壞既有 GameObject 接口。
```cpp
// include/GameObject.h
virtual bool IsBlockingCollider() const { return false; }
// include/NPC.h override
bool IsBlockingCollider() const override { return true; }
```
然後 `main.cpp:210` 區域就不用 `dynamic_cast<NPC*>`。

---

## 延後到下一輪（TIGHTEN_LATER）

### 4. GameObjectFactory：從 enum+switch 到 registry
**為什麼延後：** 對抗審查點出 static-init-order fiasco 是真的風險；rubric 也只看「Factory 存在」不看 dispatch 風格。等專案完成後再做為 portfolio polish。

### 5. ISP 重構：lifecycle-only base + 角色介面
**為什麼延後：** 對抗審查的反設計（lifecycle-only base + IUpdatable/IDrawable/IInteractable 角色介面 + 在 world loop 用 `dynamic_cast` 派發）正確且 Lab8-compliant，但牽涉所有 GameObject 子類的 inheritance 列表。時程不允許。詳細草圖見 awsome_cpp.md §4 與 §6（MVC 段落）。
**先做：** 在 awsome_cpp.md §4 + §17 補一段「為什麼我們的 GameObject 是 abstract class 而不是 interface」並寫進 UML.md 的 trade-off 區。

### 6. Vendor 抽 `IWallet`、Item 抽 `IPlayerState`
**為什麼延後：** 第二輪 DIP 對抗審查正確指出 use sites 太少（`DeductMoney` 只在 Vendor、`AddMoney` 只在 CashPickup）。等到第三個使用點出現再抽。

---

## 拒絕（REJECT_AS_OVERENGINEERED）

### 7. 把 `Player` 拆成 `Player / PlayerController / PlayerSpriteRenderer / RainExposureSystem`
**對抗審查立場：** 「ceremony before complexity」 — 多一層 indirection 對 4000-line 學生專案是負擔。
**綜合：** 拒絕。等 Player 真的 > 500 LOC 再說。

### 8. 把 `Vendor` 拆 dialog builder + transaction policy + bus publisher
**對抗審查立場：** 同上。Vendor.cpp 才 75 行，三軸耦合 = 「一個攤位」單一概念。
**綜合：** 拒絕。

### 9. 把 EventBus 改成 IEventSink 注入
**對抗審查立場：** 14 個 GameObject 子類都要加 constructor 參數，tests 也沒有 mock seam。ACCEPT_AS_PRAGMATIC。
**綜合：** 拒絕。如果未來測試需要替身，採用 `EventBus::SetTestInstance(IEventSink*)` 中庸方案。

---

## 對抗審查新發現（兩個 reviewer 都沒抓到的）

1. **`EventBus::Publish` 把 reentrancy / snapshot 政策埋進 bus 本體**（SRP 對抗審查發現）
   `src/EventBus.cpp:12-20` 在發送前先快照 handlers list — 這是一個「我允許 handler 自己 subscribe / unsubscribe」的政策決定，藏在 bus 裡。
   **行動：** 在 EventBus.h 開頭加註釋說明此快照行為，不重構。

2. **`TransparentUmbrella` 三軸合併**（SRP 對抗審查發現）
   Draw（事件化）+ Interact + OnPickup 三個 verb 都走 `beClaimed()` 同一條路徑，沒有 seam。
   **行動：** 已知 trade-off，記入 UML.md 設計取捨段；不重構。

3. **`Character` 把 `speed_` / `direction_` / `currentFrame_` 也漏給 NPC**（ISP 對抗審查發現）
   不是只有 `Update()` 空函式 — 連資料欄位都 dead。
   **行動：** 等 ISP 大重構時再一起處理（並列入 §17）。

---

## 哲學總結 — 學生專案 vs 教科書

兩邊都對：
- **第一波 reviewer** 用 Lab9 教科書定義打分，所以挑出很多「typo」級的 SOLID 違規。
- **第二輪對抗審查** 用「7 天交件、grading rubric 7 項」的工程現實打分，所以拒絕很多 ceremony。

最有價值的是 **對抗審查找到的真實漏洞**（idempotency、policy leak），與 **對抗審查給出的「正確但時程不允許」反設計**。後者是這個專案下一個迭代的方向，而不是這次的 patch。

> 對抗審查 SRP 結論引述：「class-count theater for a small solo project」 — 多寫五個類別不會讓 OOP 期末分數變高。
> 對抗審查 ISP 結論引述：「Documenting a smell does not remove the forced empty overrides」 — 但既然要重構就要重構徹底，而不是「拆兩個小介面假裝有改善」。

---

## 方法論

兩階段審查：
- **第一輪**：5 個獨立 reviewer，分別用 Lab9 教科書 SOLID 定義審查；每個 reviewer 只看一個原則。
- **第二輪**：對第一輪每一條結論做對抗反駁，質疑哪些是 over-engineering、哪些是 false positive、哪些 reviewer 漏掉的真議題。
- 本文是兩輪的綜合判決 — 完整對話紀錄保存在本機開發筆記，未推送。
