#include "ChapterVendors.h"
#include "VendorLoader.h"
#include "gfx/Vec2.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace nccu {

namespace {

std::string& VendorContentDir() {
    static std::string dir = "docs/content";
    return dir;
}

// Spatial layout is code's job (the .md owns content/dialogue, not
// positions — same split as NpcSpawns vs chapter dialog). Ten spots on
// the walkable 四維道 / central-campus band, NORTH of the south exit
// zone (y < kInterludeExitMinY=1900) so the player browses the market
// on the way down to leave. Reachability is a manual-verify item (same
// stance as every map-position table in this project). If the parser
// yields fewer stalls than positions the extra spots are simply unused;
// if more, the surplus stalls fall back to the last position.
const std::vector<nccu::gfx::Vec2>& InterludeStallPositions() {
    static const std::vector<nccu::gfx::Vec2> kPos = {
        {  400.0f, 1460.0f}, {  720.0f, 1460.0f}, { 1040.0f, 1460.0f},
        { 1360.0f, 1460.0f}, { 1660.0f, 1460.0f},
        {  400.0f, 1690.0f}, {  720.0f, 1690.0f}, { 1040.0f, 1690.0f},
        { 1360.0f, 1690.0f}, { 1660.0f, 1690.0f},
    };
    return kPos;
}

// Parsed-and-zipped Interlude placements, cached. std::optional so
// ReloadVendors() / SetVendorContentDir() can force a re-parse and the
// reference handed out by ChapterVendors() stays valid until then.
std::optional<std::vector<VendorPlacement>>& InterludeCache() {
    static std::optional<std::vector<VendorPlacement>> cache;
    return cache;
}

const std::vector<VendorPlacement>& BuildInterlude() {
    auto& cache = InterludeCache();
    if (cache) return *cache;

    std::vector<VendorPlacement> placements;
    const std::vector<VendorConfig> configs =
        nccu::vendor::LoadInterludeVendors(
            VendorContentDir() + "/interlude_market.md");
    const auto& pos = InterludeStallPositions();
    placements.reserve(configs.size());
    for (std::size_t i = 0; i < configs.size(); ++i) {
        const std::size_t p =
            pos.empty() ? 0
                        : (i < pos.size() ? i : pos.size() - 1);
        placements.push_back(VendorPlacement{
            configs[i],
            pos.empty() ? nccu::gfx::Vec2{0.0f, 0.0f} : pos[p]});
    }
    cache = std::move(placements);
    return *cache;
}

}  // namespace

const std::vector<VendorPlacement>& ChapterVendors(SemesterState state) {
    static const std::vector<VendorPlacement> kNone;
    if (state == SemesterState::Interlude_Market) return BuildInterlude();
    return kNone;  // chapters get incidental Vendors in S5c/d/e
}

void SetVendorContentDir(std::string dir) {
    VendorContentDir() = std::move(dir);
    InterludeCache().reset();   // dir change invalidates the parse
}

void ReloadVendors() {
    InterludeCache().reset();
}

}  // namespace nccu
