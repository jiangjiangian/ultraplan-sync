#pragma once
#include "raylib.h"
#include <string>

namespace nccu::gfx {

// Move-only RAII handle for raylib's GPU texture. Two Texture objects
// must NEVER share a single ::Texture2D id, or the destructor will
// Unload twice and crash. Static Load() is the only public factory.
class Texture {
public:
    static Texture Load(const std::string& path) {
        return Texture{::LoadTexture(path.c_str())};
    }

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
    explicit Texture(::Texture2D t) noexcept
        : tex_(t), owns_(t.id != 0) {}

    ::Texture2D tex_{};
    bool        owns_{false};
};

} // namespace nccu::gfx
