#ifndef GFX_WINDOW_H_
#define GFX_WINDOW_H_
#include "raylib.h"
#include <string>
#include <utility>

namespace nccu::engine::render {

/**
 * @file Window.h
 * @brief raylib 視窗的 RAII 控制代碼與其 Builder：以建構器設定後開啟視窗，解構時自動關閉。
 */

/**
 * @brief raylib 視窗的 move-only RAII 控制代碼。
 *
 * 經由內嵌 Builder 設定標題／尺寸／FPS 後 Open() 開啟視窗；解構時若仍持有視窗則
 * ::CloseWindow()。move-only 確保視窗生命週期由單一擁有者掌控，不會重複關閉。
 */
class Window {
public:
    /** @brief 視窗建構器：流暢設定標題／尺寸／FPS 後 Open()。 */
    class Builder {
    public:
        /** @brief 設定視窗標題。@param[in] t 標題。@return *this。 */
        Builder& Title(std::string t) noexcept { title_ = std::move(t); return *this; }
        /** @brief 設定視窗尺寸。@param[in] w 寬。@param[in] h 高。@return *this。 */
        Builder& Size(int w, int h)   noexcept { width_ = w; height_ = h; return *this; }
        /** @brief 設定目標 FPS。@param[in] f 每秒幀數。@return *this。 */
        Builder& Fps(int f)           noexcept { fps_ = f; return *this; }

        /** @brief 依設定開啟視窗。@return 持有該視窗的 Window。 */
        Window Open() {
            ::InitWindow(width_, height_, title_.c_str());
            // ESC 絕不可驟然結束程式——結束是「離開」選單項（首頁與遊戲內暫停選單）。
            // 停用 raylib 預設離開鍵，使 ESC 在首頁／選角／說明畫面皆無作用；只有視窗
            // 關閉鈕仍會讓 WindowShouldClose() 成立。
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

    /** @brief 視窗是否應關閉（關閉鈕被按）。@return 應關閉回傳 true。 */
    bool ShouldClose() const noexcept { return ::WindowShouldClose(); }

private:
    explicit Window(bool owns) : owns_(owns) {}
    bool owns_{false};   ///< 是否持有視窗（負責關閉）
};

} // namespace nccu::engine::render

#endif // GFX_WINDOW_H_
