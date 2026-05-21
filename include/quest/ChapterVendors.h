#ifndef CHAPTER_VENDORS_H_
#define CHAPTER_VENDORS_H_
#include "vendor/VendorConfig.h"
#include "state/SemesterState.h"
#include "gfx/Vec2.h"
#include <string>
#include <vector>

namespace nccu {

// Per-state Vendor roster, the price-table sibling of ChapterNpcSpawns.
// A Vendor is NOT an NpcSpawn (it needs a VendorConfig, not a sprite
// path + npcId), so it gets its own placement table. World iterates this
// alongside ChapterNpcSpawns inside RespawnChapterRoster.
//
// S5b-3: the Interlude lineup is no longer hand-written literals — it is
// parsed at runtime from interlude_market.md §10 via LoadInterludeVendors
// (the .md is the single source of truth, exactly like chapter dialog)
// and zipped with a code-side position table (spatial layout is code's
// job, like NpcSpawns). The result is cached (first call parses; the
// reference stays valid until ReloadVendors()). Other states return an
// empty static vector — chapters get incidental Vendors in S5c/d/e.
struct VendorPlacement {
    VendorConfig    config;
    nccu::gfx::Vec2 pos;
};

// Cached per-state placements. Interlude is parser-backed; everything
// else is empty for now.
const std::vector<VendorPlacement>& ChapterVendors(SemesterState state);

// Where LoadInterludeVendors looks (default "docs/content", mirroring
// dialog::SetContentDir). Tests point this at TEST_CONTENT_DIR. Changing
// it invalidates the cache.
void SetVendorContentDir(std::string dir);

// Drops the parse cache so the next ChapterVendors() re-reads the .md
// (hot-reload, sibling of dialog::Reload()).
void ReloadVendors();

} // namespace nccu

#endif // CHAPTER_VENDORS_H_
