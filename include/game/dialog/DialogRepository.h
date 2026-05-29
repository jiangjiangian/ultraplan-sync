#ifndef GAME_DIALOG_DIALOG_REPOSITORY_H_
#define GAME_DIALOG_DIALOG_REPOSITORY_H_
#include "game/dialog/DialogLoader.h"
#include "game/state/SemesterState.h"

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace nccu::dialog {

// Blueprint Phase 2.5 — encapsulate the per-process dialog cache + content
// dir that used to live as file-static mutable state in DialogSource.cpp
// (Cache() / ContentDir() Meyers-singletons). DialogRepository is an
// INSTANCE-scoped owner of those mutables: each instance has its own
// per-state LoadedChapter cache and its own content-dir override, so a
// test can construct a private repository for isolation while production
// continues to use the process default via the dialog::Repository() seam
// (same shape as nccu::events::Sink — the deliberate harness-swappable
// seam pattern the blueprint phase-2 audit endorses).
//
// What does NOT move into instance state: the two pure-data tables
// (NpcNameTable, ChapterFileTable) stay file-static in DialogSource.cpp
// — they are immutable lookups, sharing them across instances is
// strictly an optimisation and never a coupling problem.
class DialogRepository {
public:
    DialogRepository();

    // Returns NPC `npcId`'s sub-blocks for the chapter tied to `state`,
    // in ascending subState order. Empty vector if the npcId, the
    // mapped section name, or the chapter file is unknown / missing /
    // absent. Reference is stable until the next Reload() on THIS
    // instance — node-stable std::map storage means inserting a new
    // state's chapter does not move older vectors.
    const std::vector<SubEntry>& Entries(std::string_view npcId,
                                         SemesterState state);

    // Drops every cached chapter on THIS instance so the next Entries()
    // call re-reads the markdown from disk. The hot-reload hook for
    // writers; tests use it for inter-case isolation.
    void Reload();

    // Overrides the directory the chapter files are read from on THIS
    // instance (default "docs/content", resolved relative to the process
    // working directory). Takes effect on the next (re)load.
    void SetContentDir(std::string dir);

    [[nodiscard]] const std::string& ContentDir() const noexcept {
        return contentDir_;
    }

private:
    const LoadedChapter& ChapterFor(SemesterState state);

    std::string                            contentDir_;
    std::map<SemesterState, LoadedChapter> cache_;
};

} // namespace nccu::dialog

#endif // GAME_DIALOG_DIALOG_REPOSITORY_H_
