#pragma once
#include "raylib.h"
#include <string>
#include <utility>

namespace nccu::gfx {

class Window {
public:
    class Builder {
    public:
        Builder& Title(std::string t) noexcept { title_ = std::move(t); return *this; }
        Builder& Size(int w, int h)   noexcept { width_ = w; height_ = h; return *this; }
        Builder& Fps(int f)           noexcept { fps_ = f; return *this; }

        Window Open() {
            ::InitWindow(width_, height_, title_.c_str());
            if (fps_ > 0) ::SetTargetFPS(fps_);
            return Window{true};
        }

    private:
        std::string title_{"Window"};
        int width_{800};
        int height_{450};
        int fps_{60};
    };

    Window(const Window&)            = delete;
    Window& operator=(const Window&) = delete;

    Window(Window&& o) noexcept : owns_(o.owns_) { o.owns_ = false; }
    Window& operator=(Window&& o) noexcept {
        if (this != &o) {
            if (owns_) ::CloseWindow();
            owns_ = o.owns_;
            o.owns_ = false;
        }
        return *this;
    }

    ~Window() { if (owns_) ::CloseWindow(); }

    bool ShouldClose() const noexcept { return ::WindowShouldClose(); }

private:
    explicit Window(bool owns) : owns_(owns) {}
    bool owns_{false};
};

} // namespace nccu::gfx
