#pragma once
#include <map>
#include <string>
#include <vector>

namespace nccu::dialog {

// In-memory representation of one chapter's NPC dialog content, parsed from
// a single docs/content/<chapter>.md file. The outer key is the Chinese NPC
// name (e.g. "西裝學長"); the inner key is the substate letter ('a'..'d')
// taken from the "### (x) ..." section header; the value is the ordered
// list of dialog lines for that substate.
struct LoadedChapter {
    std::map<std::string, std::map<char, std::vector<std::string>>> npcs;
};

// Parses a single chapter markdown file at `path` and returns the parsed
// NPC dialog data. If the file cannot be opened, an empty LoadedChapter is
// returned (no exception thrown).
LoadedChapter LoadChapter(const std::string& path);

}  // namespace nccu::dialog
