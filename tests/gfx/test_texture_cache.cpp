#include "doctest/doctest.h"
#include "gfx/Texture.h"
#include <string>

// NB: do NOT `using namespace nccu::gfx;` here — raylib.h (pulled in via
// gfx/Texture.h) declares a global C `Texture` struct, so an unqualified
// `Texture` would be ambiguous against nccu::gfx::Texture. Fully qualify.
namespace ng = nccu::gfx;

// UI-C-1 regression: the load-once texture cache. These run HEADLESS (no GL
// context under ctest), so raylib's ::LoadTexture returns Texture2D{0} for
// every path (rtextures.c: {0} when LoadImage data is NULL — see
// .claude/kb/raylib-core.md §2). That is exactly the empty-resources /
// missing-file path, so the tests pin the CACHE IDENTITY + the missing-file
// NO-OP contract without needing any real asset:
//   * a second Load(path) of the same file does NOT grow the cache (a hit,
//     not a re-load) and yields the SAME texture id as the first;
//   * a missing file is still a clean no-op (id==0, IsValid()==false), and
//     a non-owning view of it never double-frees;
//   * distinct paths each add exactly one cache entry.
// Unique synthetic paths keep these independent of any other test's loads.

TEST_CASE("Texture cache: repeated Load of one path is a hit (no re-load)") {
    const std::string p = "test::__cache_probe_alpha.png";

    const std::size_t before = ng::TextureCacheSize();
    ng::Texture a = ng::Texture::Load(p);          // miss → one new entry
    const std::size_t afterFirst = ng::TextureCacheSize();
    CHECK(afterFirst == before + 1);

    ng::Texture b = ng::Texture::Load(p);          // hit → no new entry
    CHECK(ng::TextureCacheSize() == afterFirst);

    // Same cached texture id handed to both callers (here both 0, since
    // headless has no GL; the point is they ALIAS the one cache entry).
    CHECK(a.Raw().id == b.Raw().id);
    CHECK(a.Width()  == b.Width());
    CHECK(a.Height() == b.Height());
}

TEST_CASE("Texture cache: a missing file is a clean no-op (unchanged contract)") {
    // No GL / no such file → invalid texture, exactly as before the cache.
    ng::Texture t = ng::Texture::Load("test::__definitely_absent_zzz.png");
    CHECK_FALSE(t.IsValid());
    CHECK(t.Raw().id == 0);
    // A second handle to the same absent path is still invalid AND must not
    // crash on scope exit: the cached owner is non-owning ({0}), and both
    // handles are non-owning views — so neither dtor calls ::UnloadTexture.
    ng::Texture t2 = ng::Texture::Load("test::__definitely_absent_zzz.png");
    CHECK_FALSE(t2.IsValid());
}

TEST_CASE("Texture cache: distinct paths each add exactly one entry") {
    const std::size_t before = ng::TextureCacheSize();
    ng::Texture a = ng::Texture::Load("test::__cache_probe_beta.png");
    ng::Texture b = ng::Texture::Load("test::__cache_probe_gamma.png");
    CHECK(ng::TextureCacheSize() == before + 2);
    // Re-loading either is still a hit (count steady).
    ng::Texture a2 = ng::Texture::Load("test::__cache_probe_beta.png");
    ng::Texture b2 = ng::Texture::Load("test::__cache_probe_gamma.png");
    CHECK(ng::TextureCacheSize() == before + 2);
}

TEST_CASE("Texture cache: PreloadTexture warms without returning a handle") {
    const std::string p = "test::__cache_preload_delta.png";
    const std::size_t before = ng::TextureCacheSize();
    ng::PreloadTexture(p);                 // warm
    CHECK(ng::TextureCacheSize() == before + 1);
    // A subsequent Load of a preloaded path is a hit (already warm).
    ng::Texture t = ng::Texture::Load(p);
    CHECK(ng::TextureCacheSize() == before + 1);
    CHECK_FALSE(t.IsValid());              // still the missing-file no-op
}
