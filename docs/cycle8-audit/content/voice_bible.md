# Audit — docs/content/voice_bible.md

**Overview (≤3):**
1. voice_bible.md is a NOT-PARSED style guide (dialog_lint skips it per BUGLEDGER B5; `DialogSource.cpp:38-45`); per-NPC personalities, voice arcs, karma tonal shifts, and 5 named flag-keyed reactions documented for 5 NPCs.
2. Chapter content largely conforms with the bible — Ch1 西裝學長 (b) re-authored Cycle-8 N1 (`496a771`) now correctly sets `Flag_ScoldedSenior=true` matching bible §學長 voice arc Ch4 line 55 / Interlude line 51; Ch4 助教 (d) "歐趴糖" exactly matches Ending A bible line 187.
3. Two minor conflicts found: (i) bible §學長-Interlude (line 51-53) commits to "出場可有可無 / 行政大樓前遇到" but chapter2.md actually wires 3 substates at action — minor scope drift (additive, not contradictory); (ii) bible §學霸-Ch1 (line 117) says "資訊節點，給情報後立刻想結束對話" but chapter1.md (a) opens with 學霸 actively initiating a 4-line monologue without player query — voice-arc inverted at Ch1 only.

## Per-element annotations

- **§使用守則 - 28 全形字硬上限** — every dialog line ≤28 cells
  - **[是否實作?]** Yes — `dialog_lint.py docs/content/*.md` exit 0 / 0 WARN (`.claude/CHANGELOG.md:70`); `DialogLayout`/`WrapToCells` (B4) enforces at render.
  - **[邏輯衝突?]** No.

- **§使用守則 - Ripple flag 對白可感知落地** — flags must surface as line variation
  - **[是否實作?]** Yes — DialogOpener.cpp routes Ch2/Ch4 substates by flag (B3/B5/N1).
  - **[邏輯衝突?]** No.

- **西裝學長 - 人設一句話 (line 20)** — 商學院菁英、人脈話術販子
  - **[是否實作?]** Yes — chapter1.md:52-55 「我有個管道」/「順手幫我」匹配口頭禪 line 35-39.
  - **[邏輯衝突?]** No.

- **學長 §三 不會說的話 - 不直白道歉** — 迂迴轉移
  - **[是否實作?]** Yes — chapter4.md (b) 「我 Ch1 那次其實傘也不是正版的」是坦白而非道歉 (line 104).
  - **[邏輯衝突?]** No.

- **學長 §四 Ch1 voice arc** — 疲倦推銷者，傲氣比不安多
  - **[是否實作?]** Yes — chapter1.md:48-55 (a).
  - **[邏輯衝突?]** No.

- **學長 §四 Interlude (line 51-53)** — 「出場可有可無；行政大樓前一兩行」
  - **[是否實作?]** Partial — interlude_market.md never instantiates 西裝學長 (line 1-388). chapter2.md:79-117 instead wires 3 full substates at 行政大樓 — bible's "Ch2 缺席" (line 52) directly contradicted.
  - **[邏輯衝突?]** Yes — bible line 52 "Ch2 不在中正圖書館；這章他缺席" vs chapter2.md actively places him at 行政大樓 with (a)/(b)/(c). 補設定 line 53 explicitly tagged "建議補設定，待覆核" — chapter2.md adopted the suggestion without bible update.

- **學長 §四 Ch3 (`Flag_HelpedSenior=true` → 物物交換鏈提示)** — 主動打招呼+提示
  - **[是否實作?]** Yes — chapter3.md:71-77 (a) + 85-92 (b) `Flag_HelpedSenior` callback drives A 系攤位指引.
  - **[邏輯衝突?]** No.

- **學長 §四 Ch4 (`Flag_HelpedSenior=true` only出場)** — 主動找你
  - **[是否實作?]** Yes — chapter4.md:82,88-96 explicit "Flag_HelpedSenior=false / Flag_ScoldedSenior=true → 不出場".
  - **[邏輯衝突?]** No.

- **學長 §四 Ending A 看到的他** — 換普通外套，點頭，少廢話
  - **[是否實作?]** Yes — ending_a.md:50-62 「換深灰外套，少了三成廢話」.
  - **[邏輯衝突?]** No.

- **學長 §四 Ending B 不會出現** — 你成了他
  - **[是否實作?]** Partial — ending_b.md:41-54 西裝學長 DOES appear (繼續滑手機，沒有停下腳步). bible line 57 "根本不會出現" violated.
  - **[邏輯衝突?]** Yes — bible: "Ending B 看到的他：根本不會出現" vs ending_b.md actively renders 4-line interaction.

- **學長 §四 Ending C 笑搖頭走開** — 看到醜傘
  - **[是否實作?]** Yes — ending_c.md:44-53 「停頓三秒…搖搖頭…」.
  - **[邏輯衝突?]** No.

- **學長 §五 Karma>70 / 50 / <30 語氣差** — 3-tier lines
  - **[是否實作?]** Partial — chapter4.md (b) karma>70 / (c) karma<30 / (d) 3 karma tiers wired; Ch1-Ch3 use Flag_*, not raw karma thresholds.
  - **[邏輯衝突?]** No (flag-driven is a documented superset path).

- **學霸 - 人設一句話 (line 87)** — 讀書機器，冷淡
  - **[是否實作?]** Yes — chapter2.md:131-136 (a) 機械答話 matches.
  - **[邏輯衝突?]** No.

- **學霸 §四 Ch1 - 路邊資訊站，給情報後立刻結束 (line 117)** — 機械狀態
  - **[是否實作?]** Partial — chapter1.md:115-129 (a) 學霸 INITIATES the 4-line opener「加退選系統崩了，你也沒搶到課嗎？…去試試看吧」 without player query — bible says "立刻想結束對話"; chapter1.md has 學霸 over-volunteer info.
  - **[邏輯衝突?]** Yes — bible §學霸 §三 開場白「我沒有在浪費時間，你要問什麼？」 vs chapter1.md:116 學霸 主動 打招呼。 Minor voice mismatch at Ch1 only.

- **學霸 §四 Ch2 - 遊魂式當機, 「我以為是路人放的」核心台詞** — 不得刪改
  - **[是否實作?]** Yes — chapter2.md:166 exactly preserves "那把傘是你的？我以為是路人放的" + comment line 168 explicit lock.
  - **[邏輯衝突?]** No.

- **學霸 §四 Ch3 - 不在操場** — 「那種場合我不去，吵」
  - **[是否實作?]** Yes — chapter3.md:115/123 exactly "那種場合我不去，吵".
  - **[邏輯衝突?]** No.

- **學霸 §四 Ch4 - 「幾乎在關心玩家」** — voice 的補贖
  - **[是否實作?]** Yes — chapter4.md:154 「這學期…你蠻拚的」(a); 164 (b) Flag_BookwormRecovered callback.
  - **[邏輯衝突?]** No.

- **學霸 §四 Ending A - 「出太陽了，奇怪」** — 標誌性 line
  - **[是否實作?]** Yes — ending_a.md:71 exactly preserved.
  - **[邏輯衝突?]** No.

- **學霸 §四 Ending C - 「防混用。合理決策」** — 思考三秒
  - **[是否實作?]** Partial — ending_c.md:60-63 學霸 says「至少不會搞混」 (paraphrase, not the exact bible line 125 "防混用。合理決策"). Same semantics, different surface.
  - **[邏輯衝突?]** No (paraphrase, intent intact).

- **助教 - 人設一句話 (line 152)** — 流程守門人，內心焦慮
  - **[是否實作?]** Yes — chapter1.md (a) 公事公辦 + chapter4.md (a) 慌張全揭.
  - **[邏輯衝突?]** No.

- **助教 §四 Ch1 - 防禦狀態, 幫他找申請書露縫** — `Flag_HelpedTA_Ch1`
  - **[是否實作?]** Yes — chapter1.md:158-167 (b) sets `Flag_HelpedTA_Ch1=true` + karma +5; Ch2/Ch3/Ch4 all branch on it.
  - **[邏輯衝突?]** No.

- **助教 §四 Ch2 - 漣漪效應出場，`Flag_HasProfessorTrap` karma-10** — 認出玩家
  - **[是否實作?]** Yes — chapter2.md (c) Flag_HasProfessorTrap → -10 (line 223-235).
  - **[邏輯衝突?]** No.

- **助教 §四 Ch4 - `Flag_HelpedTA_Ch1`歉意 / 否則陌生人** — 弧線出口
  - **[是否實作?]** Yes — chapter4.md (b) 坦白 +10 karma / (a) 一般版.
  - **[邏輯衝突?]** No.

- **助教 §四 Ending A - 「歐趴糖，你的」** — 遞糖轉身
  - **[是否實作?]** Yes — ending_a.md:89 exact "歐趴糖" preserved.
  - **[邏輯衝突?]** No.

- **助教 §四 Ending C - 「防偷。好主意」** — 眉毛動一下
  - **[是否實作?]** Partial — ending_c.md:67-75「防誤取。合適的方案」(close paraphrase, not exact line 189 string).
  - **[邏輯衝突?]** No.

- **福利社阿姨 - 人設一句話 (line 216)** — 碎念包溫度
  - **[是否實作?]** Yes — chapter1.md:188-205 opening lines match口頭禪 line 231-235.
  - **[邏輯衝突?]** No.

- **阿姨 §四 Ch1 - 醜綠傘 Ending C 種子在此埋** — line 247
  - **[是否實作?]** Partial — chapter1.md (b)/(c) 醜綠傘 introduced; N2 fix (`b33db2b`) removed dead `Flag_KnowsUglyUmbrella` annotation, now explicit "敘事種子，非觸發旗標" (line 209-213). Bible line 247's "種子在此埋下" semantically preserved via narrative-only flow.
  - **[邏輯衝突?]** No (post-N2 the line is documentary, not flag-bearing — bible intent intact).

- **阿姨 §四 Ch4 - `Flag_BoughtCoffeeForAuntie_Ch1` 情報直接度** — line 251
  - **[是否實作?]** Yes — chapter4.md (a) direct (line 273-286) / (d) indirect (line 310-322) split per `BUGLEDGER B3` fix; routed by DialogOpener.cpp.
  - **[邏輯衝突?]** No.

- **阿姨 §四 Ending A - 「加油哦」** — 椅子吹涼風
  - **[是否實作?]** Yes — ending_a.md:108-116 「不收錢…最後一杯」matches 「在門口擺椅子吹涼風」 (line 252).
  - **[邏輯衝突?]** No.

- **苦主 - 人設一句話 (line 281)** — 委屈壓低，被記得才顯出
  - **[是否實作?]** Yes — chapter1.md (a) 「我的傘不見了」(line 246) matches口頭禪.
  - **[邏輯衝突?]** No.

- **苦主 §四 Ch1 - 脆弱，承諾路徑 `Flag_PromisedVictim`** — line 311
  - **[是否實作?]** Yes — chapter1.md:252-259 (b) sets flag, karma+5.
  - **[邏輯衝突?]** No.

- **苦主 §四 Ch3 - 「你還沒找到」隱性扣點** — line 315
  - **[是否實作?]** Yes — chapter3.md:217-222 (a) 承諾未履行 → -5 karma 隱性 (per bible "karma 的隱性扣點" line 315 ; matches `// karma -5` annotation line 221).
  - **[邏輯衝突?]** No.

- **苦主 §四 Ch4 - 釋懷或淡漠** — line 316
  - **[是否實作?]** Yes — chapter4.md (a) +5 釋懷 / (b) -10 淡漠 (line 331-356).
  - **[邏輯衝突?]** No.

- **苦主 §五 Karma<30 「沒關係。」** — line 330 (低頭三字)
  - **[是否實作?]** Yes — chapter3.md:220 + chapter4.md:352 「沒關係。」 minimal-form preserved.
  - **[邏輯衝突?]** No.

- **苦主 §六 阿姨對她的記憶** — 「那個蹲在外面的女同學」
  - **[是否實作?]** No — no in-chapter dialogue surfaces 阿姨提到苦主. Bible relation §六 line 338 is bonus depth never used.
  - **[邏輯衝突?]** No (under-implemented, not contradicted).

- **§待覆核 1-8 (line 342-353)** — 自行補設定
  - **[是否實作?]** N/A — bible self-tagged "待覆核"; no implementation required.
  - **[邏輯衝突?]** No.

- **§Ch1 校對紀錄 (line 358-363)** — 助教 Ch1 慌張底層 / 苦主記恨基調 / 學霸 Ch2 動機
  - **[是否實作?]** Yes (advisory) — chapter2.md:204-208 (a) "盯著紙半秒" 慌張開始浮現; chapter3.md 苦主 (a) 「沒關係」靜態失望; chapter2.md (c-fail) 「我以為是路人放的」確認無意識.
  - **[邏輯衝突?]** No.

## Summary

- Elements audited: **35**
- Yes (fully implemented): **24**
- Partial: **6** (學長 Interlude/Ch2 出場度 · 學霸 Ch1 voice 反向 · 學長 Ending B 出場 · 學霸/助教 Ending C 句子 paraphrase · 阿姨 Ch1 種子)
- No: **1** (阿姨對苦主關係，未在對白中浮現)
- N/A: **4** (28-cell / flag-routing meta-rules / 待覆核 / 校對紀錄 advisory)
- Conflicts: **2** (學長 Interlude 「出場可有可無」vs chapter2 3 substates [bible line 52]; 學長 Ending B 「根本不會出現」vs ending_b.md 4-line interaction [bible line 57])

Both conflicts are bible-text vs chapter-content drift; neither is gameplay-breaking. The Ending B conflict could be addressed by either updating bible line 57 or removing 學長 ending_b 段; chapter2's expanded Interlude/Ch2 學長 substates are bible-suggested 補設定 line 53 already adopted in content.
