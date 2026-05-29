#pragma once
#include "game/dialog/DialogLoader.h"
#include "game/state/SemesterState.h"

#include <string>
#include <string_view>
#include <vector>

namespace nccu::dialog {

// Runtime dialog provider. Maps an English NPC id + the current
// SemesterState to that NPC's parsed sub-blocks for the matching
// chapter, loading the chapter markdown on demand through LoadChapter
// and caching one LoadedChapter per state.
//
// Why this exists: the dialog content used to be frozen into a
// generated header at build time, so any text tweak meant
// regenerating and recompiling. This provider reads the authored
// docs/content/*.md straight at runtime and exposes Reload() so
// writers can edit a chapter file and see the change on the next
// dialog without a rebuild.
//
// English npcId -> Chinese section name and SemesterState -> chapter
// filename are fixed lookups kept in the .cpp. Unknown id / name /
// file never throws — it returns an empty vector, mirroring
// LoadChapter's no-throw contract.
//
// Reference lifetime: Entries() hands back a reference into the
// per-state cache. It stays valid until the next Reload() (which
// clears the cache); a later Entries() call for a not-yet-loaded
// state only appends and does not invalidate earlier references.

// Returns NPC `npcId`'s sub-blocks for the chapter tied to `state`,
// in ascending subState order. Empty vector if the npcId, the mapped
// section name, or the chapter file is unknown / missing / absent.
const std::vector<SubEntry>& Entries(std::string_view npcId,
                                     SemesterState state);

// Drops every cached chapter so the next Entries() call re-reads the
// markdown from disk. This is the hot-reload hook: edit a
// docs/content/*.md, call Reload(), and the new text is served.
void Reload();

// Overrides the directory the chapter files are read from (default
// "docs/content", resolved relative to the process working
// directory). Lets a unit test point at a fixed content dir without
// depending on cwd. Takes effect on the next (re)load of a state.
void SetContentDir(std::string dir);

}  // namespace nccu::dialog
