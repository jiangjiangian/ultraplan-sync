# Docs Knowledge Graph — 《尋傘記》

Domain-specific extractor (`tools/docs_graph.py`). Walks `docs/`,
parses heading tree + `Flag_*` references + NPC headings + karma
annotations. **No LLM required.**

## 1. Cross-reference graph (edges = shared `Flag_*` between files)

```mermaid
flowchart LR
    docs_FILE_ORGANIZATION_md["FILE_ORGANIZATION"]
    docs_ROADMAP_md["ROADMAP"]
    docs_SCRIPT_HANDOFF_md["SCRIPT_HANDOFF"]
    docs_SOLID_REVIEW_md["SOLID_REVIEW"]
    docs_STRICT_REVIEW_md["STRICT_REVIEW"]
    docs_STRICT_REVIEW_R3_md["STRICT_REVIEW_R3"]
    docs_content_chapter1_md["chapter1"]
    docs_content_chapter2_md["chapter2"]
    docs_content_chapter3_md["chapter3"]
    docs_content_chapter4_md["chapter4"]
    docs_content_ending_a_md["ending_a"]
    docs_content_ending_b_md["ending_b"]
    docs_content_ending_c_md["ending_c"]
    docs_content_interlude_market_md["interlude_market"]
    docs_content_voice_bible_md["voice_bible"]
    docs_ROADMAP_md -- 11 --- docs_SCRIPT_HANDOFF_md
    docs_ROADMAP_md -- 8 --- docs_content_chapter3_md
    docs_SCRIPT_HANDOFF_md -- 8 --- docs_content_chapter3_md
    docs_ROADMAP_md -- 7 --- docs_content_chapter4_md
    docs_SCRIPT_HANDOFF_md -- 7 --- docs_content_chapter4_md
    docs_content_chapter3_md -- 7 --- docs_content_chapter4_md
    docs_ROADMAP_md -- 7 --- docs_content_chapter1_md
    docs_SCRIPT_HANDOFF_md -- 7 --- docs_content_chapter1_md
    docs_ROADMAP_md -- 7 --- docs_content_chapter2_md
    docs_SCRIPT_HANDOFF_md -- 7 --- docs_content_chapter2_md
    docs_content_chapter2_md -- 7 --- docs_content_chapter3_md
    docs_content_chapter1_md -- 6 --- docs_content_chapter2_md
    docs_content_chapter1_md -- 6 --- docs_content_chapter3_md
    docs_content_chapter2_md -- 6 --- docs_content_chapter4_md
    docs_ROADMAP_md -- 5 --- docs_content_voice_bible_md
    docs_SCRIPT_HANDOFF_md -- 5 --- docs_content_voice_bible_md
    docs_content_chapter1_md -- 5 --- docs_content_voice_bible_md
    docs_content_chapter1_md -- 5 --- docs_content_chapter4_md
    docs_content_chapter2_md -- 4 --- docs_content_voice_bible_md
    docs_content_chapter3_md -- 4 --- docs_content_voice_bible_md
    docs_content_chapter4_md -- 4 --- docs_content_voice_bible_md
    docs_ROADMAP_md -- 3 --- docs_content_ending_a_md
    docs_SCRIPT_HANDOFF_md -- 3 --- docs_content_ending_a_md
    docs_content_chapter1_md -- 3 --- docs_content_ending_a_md
    docs_content_chapter2_md -- 3 --- docs_content_ending_a_md
    docs_content_chapter3_md -- 3 --- docs_content_ending_a_md
    docs_content_chapter4_md -- 3 --- docs_content_ending_a_md
    docs_content_ending_a_md -- 3 --- docs_content_voice_bible_md
    docs_ROADMAP_md -- 1 --- docs_content_interlude_market_md
    docs_SCRIPT_HANDOFF_md -- 1 --- docs_content_interlude_market_md
    docs_content_chapter2_md -- 1 --- docs_content_interlude_market_md
    docs_content_chapter3_md -- 1 --- docs_content_interlude_market_md
    docs_content_chapter4_md -- 1 --- docs_content_interlude_market_md
```

## 2. Per-file stats

| File | Lines | Headings | Flags | NPCs | karma Σ (+/−) |
|------|-------|----------|-------|------|---------------|
| `docs/FILE_ORGANIZATION.md` | 109 | 7 | 0 | 0 | +0 (+0/+0) |
| `docs/ROADMAP.md` | 189 | 15 | 11 | 0 | +0 (+0/+0) |
| `docs/SCRIPT_HANDOFF.md` | 182 | 22 | 12 | 0 | +5 (+5/+0) |
| `docs/SOLID_REVIEW.md` | 133 | 17 | 0 | 0 | +0 (+0/+0) |
| `docs/STRICT_REVIEW.md` | 423 | 51 | 0 | 0 | +0 (+0/+0) |
| `docs/STRICT_REVIEW_R3.md` | 312 | 22 | 0 | 0 | +0 (+0/+0) |
| `docs/content/chapter1.md` | 301 | 28 | 7 | 0 | -47 (+21/-68) |
| `docs/content/chapter2.md` | 402 | 33 | 7 | 0 | -32 (+16/-48) |
| `docs/content/chapter3.md` | 357 | 40 | 8 | 0 | -1 (+19/-20) |
| `docs/content/chapter4.md` | 411 | 35 | 7 | 0 | +60 (+105/-45) |
| `docs/content/ending_a.md` | 220 | 12 | 4 | 0 | +0 (+0/+0) |
| `docs/content/ending_b.md` | 203 | 10 | 0 | 0 | +0 (+0/+0) |
| `docs/content/ending_c.md` | 167 | 12 | 0 | 0 | +0 (+0/+0) |
| `docs/content/interlude_market.md` | 396 | 59 | 2 | 0 | +0 (+0/+0) |
| `docs/content/voice_bible.md` | 364 | 39 | 5 | 0 | -10 (+0/-10) |

## 3. Global flag-usage matrix

| Flag | Files using it |
|------|----------------|
| `Flag_BookwormRecovered` | ROADMAP, SCRIPT_HANDOFF, chapter3, chapter4 |
| `Flag_BoughtCoffeeForAuntie_Ch1` | ROADMAP, SCRIPT_HANDOFF, chapter1, voice_bible |
| `Flag_BoughtUglyUmbrella` | ROADMAP, SCRIPT_HANDOFF, chapter1, chapter2, chapter3, chapter4 |
| `Flag_EndingA_True` | ending_a |
| `Flag_HasProfessorTrap` | ROADMAP, SCRIPT_HANDOFF, chapter2, chapter3, chapter4, interlude_market |
| `Flag_HelpedSenior` | ROADMAP, SCRIPT_HANDOFF, chapter1, chapter2, chapter3, chapter4, ending_a, voice_bible |
| `Flag_HelpedTA_Ch1` | ROADMAP, SCRIPT_HANDOFF, chapter1, chapter2, chapter3, chapter4, ending_a, voice_bible |
| `Flag_KnowsUglyUmbrella` | SCRIPT_HANDOFF |
| `Flag_LeaveInterlude` | interlude_market |
| `Flag_PromisedVictim` | ROADMAP, SCRIPT_HANDOFF, chapter1, chapter2, chapter3, chapter4, ending_a, voice_bible |
| `Flag_SawVictim_Ch1` | ROADMAP, SCRIPT_HANDOFF |
| `Flag_ScoldedSenior` | ROADMAP, SCRIPT_HANDOFF, chapter1, chapter2, chapter3, chapter4, voice_bible |
| `Flag_TookCursedUmbrella` | ROADMAP, SCRIPT_HANDOFF, chapter1, chapter2, chapter3 |
| `Flag_xxx` | ROADMAP, SCRIPT_HANDOFF |

## 4. Karma flow summary

- Total karma annotations across `docs/content/`: **46**
- Net karma if a player triggered every annotation: **-25**
- Positive routes sum: **+166** · Negative routes sum: **-191**
- Starting karma per GDD: **50** → Ending A gate `karma > 80` requires
  **30+** net positive picks; Ending B gate `karma < 0` requires
  **−50+** net negative picks. The above totals show whether either gate
  is reachable purely via dialog karma.

## 5. Generation

`python3 tools/docs_graph.py` rewrites this file. GitHub renders the
mermaid block natively. No API key, no LLM, no external deps.
