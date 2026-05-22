#ifndef GFX_WINDOW_H_
#define GFX_WINDOW_H_
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
            // ESC must never abruptly quit the program — quitting is the
            // 離開 menu item (title + in-game pause menu). Disabling
            // raylib's default exit key keeps ESC inert on the title /
            // character-select / 說明 screens (player request); only the
            // window close button still ends WindowShouldClose().
            ::SetExitKey(KEY_NULL);
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

#endif // GFX_WINDOW_H_
