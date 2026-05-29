#ifndef GFX_TEXTURE_H_
#define GFX_TEXTURE_H_
#include "raylib.h"
#include <string>
#include <unordered_map>

namespace nccu::engine::render {

namespace detail { class TextureCache; }  // owns each cached texture once

// Move-only RAII handle for raylib's GPU texture.
//
// OWNERSHIP MODEL (UI-C-1). A Texture is EITHER an owner (owns_==true: its
// dtor ::UnloadTexture()s the GPU id) OR a non-owning VIEW (owns_==false:
// dtor is a no-op). Two OWNING Texture objects must never share one
// ::Texture2D id, or the id is Unloaded twice and crashes — but any number
// of VIEWS may alias one owner's id safely, because a view never Unloads.
//
// Load() no longer hits the GPU on every call. It routes through a process-
// lifetime cache (detail::TextureCache): the FIRST Load(path) reads from
// disk + uploads once and the cache OWNS that texture; that and every later
// Load(path) of the same file return a cheap non-owning VIEW into the cache.
// So an entity's sprite_ / the View's strips are views — their dtors never
// touch the shared GPU id (no double-free, no use-after-free), and a Restart
// that rebuilds World/View hits the warm cache (no disk stutter).
//
// The cache OUTLIVES every entity/View (it is a function-local static) and
// is torn down explicitly by ShutdownTextureCache() called from main.cpp
// while the GL context is still alive — exactly the Font.h discipline
// (gfx/Font.h ShutdownFont before ~Window/CloseWindow). See .claude/kb/
// raylib-core.md §1-2.
//
// MISSING-FILE CONTRACT (unchanged, do NOT alter): ::LoadTexture on an
// absent/garbage file returns {0} (rtextures.c: Texture2D{0} when LoadImage
// data is NULL), so the cache stores an invalid (id==0, owns==false) entry
// and Load() returns an invalid view — IsValid()==false, draws nothing, no
// crash. The empty-resources / headless-harness path stays a clean no-op.
class Texture {
public:
    // Load-once: a cache hit returns a non-owning view of the already-
    // uploaded texture; a miss uploads once (the cache keeps the owner).
    static Texture Load(const std::string& path);

    Texture(const Texture&)            = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(Texture&& o) noexcept : tex_(o.tex_), owns_(o.owns_) {
        o.tex_ = {};
        o.owns_ = false;
    }
    Texture& operator=(Texture&& o) noexcept {
        if (this != &o) {
            if (owns_) ::UnloadTexture(tex_);
            tex_ = o.tex_;
            owns_ = o.owns_;
            o.tex_ = {};
            o.owns_ = false;
        }
        return *this;
    }

    ~Texture() { if (owns_) ::UnloadTexture(tex_); }

    int  Width()   const noexcept { return tex_.width;  }
    int  Height()  const noexcept { return tex_.height; }
    bool IsValid() const noexcept { return tex_.id != 0; }

    // Internal accessor: only used by Renderer::Texture inside include/gfx/.
    const ::Texture2D& Raw() const noexcept { return tex_; }

private:
    // owning==true takes ownership of `t` (dtor will Unload it); owning==
    // false builds a non-owning view that aliases `t` but never Unloads.
    explicit Texture(::Texture2D t, bool owning) noexcept
        : tex_(t), owns_(owning) {}

    // A non-owning VIEW of an existing (cache-owned) texture. Aliases its
    // id/size for drawing; its dtor is a no-op so it can freely outlive
    // nothing and never double-frees the shared id.
    static Texture View(const ::Texture2D& t) noexcept {
        return Texture{t, /*owning=*/false};
    }

    ::Texture2D tex_{};
    bool        owns_{false};

    // The cache builds non-owning views via View() and owners via the
    // (Texture2D,bool) ctor, so it needs access to both.
    friend class detail::TextureCache;
};

namespace detail {

// The process-lifetime texture store. Keyed by path; each value is the ONE
// owning Texture for that file (uploaded at most once). A function-local
// static so it is created lazily after InitWindow (the first Load) and — by
// the static-dtor-after-main hazard (raylib-core.md §1) — is NOT relied on
// to self-destruct; ShutdownTextureCache() empties it explicitly while GL
// is alive. Move-only Texture values live fine in the map (node-based).
class TextureCache {
public:
    // Find-or-load `path`; return a non-owning view of the cached owner.
    // A missing file caches an invalid owner ({0}) and returns an invalid
    // view, so the no-op contract holds and we don't re-stat it every call.
    Texture Acquire(const std::string& path) {
        auto it = store_.find(path);
        if (it == store_.end()) {
            // Construct the sole owner from the disk/GPU load. owning is set
            // from id!=0 so a missing-file {0} owner Unloads nothing.
            ::Texture2D raw = ::LoadTexture(path.c_str());
            it = store_.emplace(path,
                                Texture{raw, /*owning=*/(raw.id != 0)}).first;
        }
        return ViewOf(it->second);
    }

    // Warm the cache for `path` without returning a handle (preload). Just
    // an Acquire whose view is discarded — the owner stays in the map.
    void Warm(const std::string& path) { (void)Acquire(path); }

    // Unload every cached texture (each owner's dtor ::UnloadTexture()s) and
    // empty the map. Must run while the GL context is still alive.
    void Clear() noexcept { store_.clear(); }

    std::size_t Size() const noexcept { return store_.size(); }

private:
    static Texture ViewOf(const Texture& owner) noexcept {
        return Texture::View(owner.Raw());
    }
    std::unordered_map<std::string, Texture> store_;
};

inline TextureCache& Textures() {
    static TextureCache cache;
    return cache;
}

} // namespace detail

// ---- free functions on the process texture cache -----------------------

// Warm the cache with one texture ahead of time (preload). Safe to call
// repeatedly; a path is uploaded at most once. A no-op-cheap miss when the
// file is absent (headless / empty resources).
inline void PreloadTexture(const std::string& path) {
    detail::Textures().Warm(path);
}

// Tear the cache down. Call from main() BEFORE ~Window/::CloseWindow (while
// GL is alive) — mirrors nccu::engine::render::ShutdownFont. Idempotent (clearing an empty
// map is a no-op), so a normal-exit path that never preloaded is fine too.
inline void ShutdownTextureCache() noexcept { detail::Textures().Clear(); }

// Test/diagnostic: how many distinct paths are currently cached.
inline std::size_t TextureCacheSize() noexcept {
    return detail::Textures().Size();
}

inline Texture Texture::Load(const std::string& path) {
    return detail::Textures().Acquire(path);
}

} // namespace nccu::engine::render

#endif // GFX_TEXTURE_H_
