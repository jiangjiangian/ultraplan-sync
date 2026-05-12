# Test Fixture：DialogLoader Sample

## 章節 metadata

- SemesterState: `TestState`
- 此區塊位於第一個 `## NPC：` 之前，應被忽略。

## 場景旁白

- "這行 narration 不屬於任何 NPC，必須被忽略。"

## NPC：學長

> 場景：測試走廊。

### (a) 初次接觸

- "嗨，學弟。"
- "你也來測試嗎？"

### (b) 二次接觸

- "又見面了。"

## NPC：學妹

> 提供 CJK 引號版本以驗證 parser。

### (a) 招呼

- “學長你好。”
- “今天天氣真好。”

### (c) 沒台詞的 substate

## 章節結尾

- "這行不屬於任何 NPC，必須被忽略。"
