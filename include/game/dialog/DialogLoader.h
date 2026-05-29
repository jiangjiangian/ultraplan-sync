#pragma once
#include <map>
#include <string>
#include <vector>

namespace nccu::dialog {

// One parsed "### (x) ..." sub-block of an NPC section. Mirrors the
// metadata the build-time codegen (tools/gen_dialog.py) carries per Entry:
//   - subState : substate letter mapped to int (a->0 b->1 c->2 d->3),
//                where 'a' is the opener.
//   - lines    : ordered dialog lines for this substate (may be empty if
//                the heading carries no `- "line"` items).
//   - choiceLabel : menu label derived from the heading text. A 「…」
//                quoted span in the heading is the author's explicit
//                override and wins; otherwise the heading text after the
//                "(x)" marker with any trailing （…） parenthetical
//                stripped. "" = not a choice.
//   - karmaDelta : first non-zero `// karma ±N` found in this substate's
//                `>` blockquote notes (0 = none / explicit +0).
//   - setsFlag / flagValue : first `Flag_X = true|false` found in the same
//                blockquote notes ("" = none; flagValue defaults false).
struct SubEntry {
    int                      subState;
    std::vector<std::string> lines;
    std::string              choiceLabel;     // "" = not a choice
    int                      karmaDelta = 0;
    std::string              setsFlag;        // "" = none
    bool                     flagValue = false;
};

// In-memory representation of one chapter's NPC dialog content, parsed from
// a single docs/content/<chapter>.md file. The key is the Chinese NPC name
// (e.g. "西裝學長"); the value is that NPC's sub-blocks in ascending
// subState order.
struct LoadedChapter {
    std::map<std::string, std::vector<SubEntry>> npcs;
};

// Parses a single chapter markdown file at `path` and returns the parsed
// NPC dialog data. If the file cannot be opened, an empty LoadedChapter is
// returned (no exception thrown).
LoadedChapter LoadChapter(const std::string& path);

}  // namespace nccu::dialog
