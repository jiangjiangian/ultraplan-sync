# Raylib Modern C++ Wrapper Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace every direct raylib call in the codebase with a header-only `nccu::gfx` wrapper layer that demonstrates RAII / Builder / Fluent / Facade / Adapter patterns, while keeping the visual game output identical.

**Architecture:** Header-only namespace `nccu::gfx` under `include/gfx/`. Wrapper types (`Color`, `Vec2`, `Rect`) share raylib's memory layout and convert internally. RAII for `Window` / `DrawScope`. Fluent helpers for `Renderer` / `TextBuilder`. Facades for `Input` / `Time`. Game code never `#include "raylib.h"` again.

**Tech Stack:** C++17, raylib 5.5, doctest, CMake 3.16+, ctest.

**Spec reference:** `docs/superpowers/specs/2026-05-11-raylib-modern-cpp-wrapper-design.md`

**Project hard rules (do not violate):**
- `Player` may NOT `#include` any concrete `TransparentUmbrella` subclass header
- `Item` / `TransparentUmbrella` may NOT call `DrawText` / `DrawTexture`
- Main loop may NOT delete `GameObject` mid-iteration; defer to end-of-frame sweep

**Naming hygiene rule (project memory):** No file content (source, headers, markdown, comments, commit messages) may reference: Claude / Codex / Claude Code / Anthropic / Gemini / nanobanana / "AI agent" / CLAUDE.md / superpowers / agent automation. Use neutral language. Grep before each commit.

**Build / verify commands:**
```bash
cmake -B build                # configure (idempotent)
cmake --build build           # build (zero warnings target)
(cd build && ctest --output-on-failure)
./build/OOP_Raylib_Lab        # smoke run; survive ≥5s
```

**Final verification gate (Task 15):**
1. `cmake --build build` succeeds with zero warnings on `-Wall -Wextra -Wpedantic`
2. `ctest` all green (existing 9 + new ≥ 4)
3. `./build/OOP_Raylib_Lab` launches and survives ≥ 5 seconds; visuals match pre-refactor (player blue square, four umbrella squares, HUD lines, E-key pickup, karma display)
4. `grep -rn "raylib.h" src/ tests/ include/ | grep -v "include/gfx/"` returns no matches

---

## File Structure

**New (created in this plan):**
```
include/gfx/
├── Color.h          Wrapper struct (R,G,B,A) + WithAlpha + constexpr Colors:: palette
├── Vec2.h           Wrapper struct (x,y) + arithmetic
├── Rect.h           Wrapper struct (x,y,w,h) + Contains(Vec2) + Intersects(Rect)
├── Key.h            enum class Key (W,A,S,D,E,...) mapped to raylib KEY_*
├── Time.h           Static facade — DeltaSeconds, FpsAvg
├── Input.h          Static facade — IsDown / IsPressed / IsReleased
├── Window.h         RAII + Builder
├── DrawScope.h      RAII for BeginDrawing / EndDrawing
├── Renderer.h       Fluent — Clear / Rect / Pixel returning Renderer&
└── TextBuilder.h    Builder — At / Size / Color / Draw

tests/
├── test_color.cpp
├── test_vec2.cpp
├── test_rect.cpp
└── test_text_builder.cpp
```

**Modified:**
- `include/EventBus.h` — `Event::position` and `Event::color` switch to `Vec2` / `Color` (gfx)
- `include/GameObject.h` — `position_` / `hitBox_` switch to `Vec2` / `Rect`; remove `raylib.h`
- `include/Character.h` — `direction_` switch to `Vec2`; `Move(Vec2 dir, float dt)`; remove `raylib.h`
- `include/Item.h` — header constructor signature uses `Vec2` / `Rect`; remove `raylib.h`
- `include/Player.h` — same migration
- `include/TransparentUmbrella.h` — `umbrellaTint_` is `Color`; `Vec2` for ctor
- `include/TrueUmbrella.h`, `FragileUmbrella.h`, `ProfessorTrapUmbrella.h`, `CursedUmbrella.h` — ctor takes `Vec2`
- `src/Player.cpp` — `Draw()` uses Renderer; `HandleInput()` uses Input/Key
- `src/TrueUmbrella.cpp`, `FragileUmbrella.cpp`, `ProfessorTrapUmbrella.cpp`, `CursedUmbrella.cpp`, `TransparentUmbrella.cpp` — Publish wrapper types
- `src/main.cpp` — Window/DrawScope/Renderer/TextBuilder/Input/Time
- `tests/test_factory.cpp`, `test_player.cpp` — pass `Vec2{}` instead of `{0,0}` literals where `Vector2` was implied
- `CMakeLists.txt` — no change needed (GLOB picks up `include/gfx/*.h`)

---

## Task 1: `nccu::gfx::Color` (wrapper struct)

**Files:**
- Create: `include/gfx/Color.h`
- Test: `tests/test_color.cpp`

- [ ] **Step 1: Write the failing test**

Create `tests/test_color.cpp`:
```cpp
#include "doctest/doctest.h"
#include "gfx/Color.h"

using nccu::gfx::Color;
using nccu::gfx::Colors;

TEST_CASE("Color: default-constructs to opaque black") {
    constexpr Color c;
    CHECK(c.r == 0);
    CHECK(c.g == 0);
    CHECK(c.b == 0);
    CHECK(c.a == 255);
}

TEST_CASE("Color: aggregate-init {r,g,b,a}") {
    constexpr Color c{10, 20, 30, 200};
    CHECK(c.r == 10);
    CHECK(c.g == 20);
    CHECK(c.b == 30);
    CHECK(c.a == 200);
}

TEST_CASE("Color::WithAlpha returns a new color with overridden alpha") {
    constexpr Color base{255, 0, 0, 255};
    constexpr Color faded = base.WithAlpha(128);
    CHECK(faded.r == 255);
    CHECK(faded.a == 128);
    CHECK(base.a == 255); // immutable
}

TEST_CASE("Colors:: palette has expected presets") {
    CHECK(Colors::Black.r == 0);
    CHECK(Colors::White.r == 255);
    CHECK(Colors::White.a == 255);
    CHECK(Colors::Blue.b > 200);
}

TEST_CASE("Color equality") {
    constexpr Color a{1, 2, 3, 4};
    constexpr Color b{1, 2, 3, 4};
    constexpr Color c{1, 2, 3, 5};
    CHECK(a == b);
    CHECK_FALSE(a == c);
}
```

- [ ] **Step 2: Run tests — they fail to compile (Color.h missing)**

Run: `cmake --build build 2>&1 | tail -10`
Expected: `fatal error: 'gfx/Color.h' file not found`

- [ ] **Step 3: Implement `gfx/Color.h`**

```cpp
#pragma once
#include "raylib.h"
#include <cstdint>

namespace nccu::gfx {

struct Color {
    std::uint8_t r{0};
    std::uint8_t g{0};
    std::uint8_t b{0};
    std::uint8_t a{255};

    constexpr Color WithAlpha(std::uint8_t newA) const noexcept {
        return Color{r, g, b, newA};
    }

    // Internal-only conversion to raylib type. Game code does not use this
    // directly because game code never includes raylib.h.
    constexpr operator ::Color() const noexcept {
        return ::Color{r, g, b, a};
    }
};

constexpr bool operator==(Color a, Color b) noexcept {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}
constexpr bool operator!=(Color a, Color b) noexcept { return !(a == b); }

namespace Colors {
inline constexpr Color Black     {  0,   0,   0, 255};
inline constexpr Color White     {255, 255, 255, 255};
inline constexpr Color RayWhite  {245, 245, 245, 255};
inline constexpr Color DarkGray  { 80,  80,  80, 255};
inline constexpr Color Blue      {  0, 121, 241, 255};
inline constexpr Color Red       {230,  41,  55, 255};
inline constexpr Color Green     {  0, 228,  48, 255};
inline constexpr Color Yellow    {253, 249,   0, 255};
inline constexpr Color Magenta   {255,   0, 255, 255};
}

} // namespace nccu::gfx
```

- [ ] **Step 4: Run tests — they pass**

Run: `cmake --build build && (cd build && ctest --output-on-failure -R unit_tests)`
Expected: PASS for all 5 Color cases; total assertions increase by ≥ 12.

- [ ] **Step 5: Commit**

```bash
git add include/gfx/Color.h tests/test_color.cpp
git commit -m "feat(gfx): add nccu::gfx::Color wrapper with constexpr palette"
```

---

## Task 2: `nccu::gfx::Vec2` (wrapper struct)

**Files:**
- Create: `include/gfx/Vec2.h`
- Test: `tests/test_vec2.cpp`

- [ ] **Step 1: Write failing test**

Create `tests/test_vec2.cpp`:
```cpp
#include "doctest/doctest.h"
#include "gfx/Vec2.h"
#include <cmath>

using nccu::gfx::Vec2;

TEST_CASE("Vec2: default-constructs to (0,0)") {
    constexpr Vec2 v;
    CHECK(v.x == doctest::Approx(0.0f));
    CHECK(v.y == doctest::Approx(0.0f));
}

TEST_CASE("Vec2: aggregate init {x,y}") {
    constexpr Vec2 v{3.0f, 4.0f};
    CHECK(v.x == doctest::Approx(3.0f));
    CHECK(v.y == doctest::Approx(4.0f));
}

TEST_CASE("Vec2 arithmetic operators") {
    constexpr Vec2 a{1, 2};
    constexpr Vec2 b{3, 5};
    CHECK((a + b).x == doctest::Approx(4.0f));
    CHECK((a + b).y == doctest::Approx(7.0f));
    CHECK((b - a).x == doctest::Approx(2.0f));
    CHECK((a * 2.0f).x == doctest::Approx(2.0f));
    CHECK((a * 2.0f).y == doctest::Approx(4.0f));
}

TEST_CASE("Vec2::Length returns Euclidean magnitude") {
    Vec2 v{3.0f, 4.0f};
    CHECK(v.Length() == doctest::Approx(5.0f));
}

TEST_CASE("Vec2::Normalized returns unit vector or zero for zero input") {
    Vec2 v{0.0f, 0.0f};
    auto n = v.Normalized();
    CHECK(n.x == doctest::Approx(0.0f));
    CHECK(n.y == doctest::Approx(0.0f));

    Vec2 u{3.0f, 4.0f};
    auto un = u.Normalized();
    CHECK(un.x == doctest::Approx(0.6f));
    CHECK(un.y == doctest::Approx(0.8f));
}
```

- [ ] **Step 2: Run — fails to compile**

Run: `cmake --build build 2>&1 | tail -10`
Expected: `fatal error: 'gfx/Vec2.h' file not found`

- [ ] **Step 3: Implement `gfx/Vec2.h`**

```cpp
#pragma once
#include "raylib.h"
#include <cmath>

namespace nccu::gfx {

struct Vec2 {
    float x{0.0f};
    float y{0.0f};

    constexpr operator ::Vector2() const noexcept { return ::Vector2{x, y}; }

    float Length() const noexcept { return std::sqrt(x * x + y * y); }

    Vec2 Normalized() const noexcept {
        float len = Length();
        if (len < 1e-6f) return Vec2{0.0f, 0.0f};
        return Vec2{x / len, y / len};
    }
};

constexpr Vec2 operator+(Vec2 a, Vec2 b) noexcept { return Vec2{a.x + b.x, a.y + b.y}; }
constexpr Vec2 operator-(Vec2 a, Vec2 b) noexcept { return Vec2{a.x - b.x, a.y - b.y}; }
constexpr Vec2 operator*(Vec2 a, float s) noexcept { return Vec2{a.x * s, a.y * s}; }
constexpr Vec2 operator*(float s, Vec2 a) noexcept { return a * s; }

} // namespace nccu::gfx
```

- [ ] **Step 4: Run — passes**

Run: `cmake --build build && (cd build && ctest --output-on-failure -R unit_tests)`
Expected: 5 new Vec2 cases pass.

- [ ] **Step 5: Commit**

```bash
git add include/gfx/Vec2.h tests/test_vec2.cpp
git commit -m "feat(gfx): add nccu::gfx::Vec2 wrapper with arithmetic + Length/Normalized"
```

---

## Task 3: `nccu::gfx::Rect` (wrapper struct)

**Files:**
- Create: `include/gfx/Rect.h`
- Test: `tests/test_rect.cpp`

- [ ] **Step 1: Write failing test**

Create `tests/test_rect.cpp`:
```cpp
#include "doctest/doctest.h"
#include "gfx/Rect.h"
#include "gfx/Vec2.h"

using nccu::gfx::Rect;
using nccu::gfx::Vec2;

TEST_CASE("Rect: default-constructs to zero") {
    constexpr Rect r;
    CHECK(r.x == doctest::Approx(0.0f));
    CHECK(r.width == doctest::Approx(0.0f));
}

TEST_CASE("Rect: aggregate init") {
    constexpr Rect r{10.0f, 20.0f, 30.0f, 40.0f};
    CHECK(r.x == doctest::Approx(10.0f));
    CHECK(r.width == doctest::Approx(30.0f));
}

TEST_CASE("Rect::Contains true when point inside") {
    Rect r{10, 10, 20, 20};
    CHECK(r.Contains(Vec2{15, 15}));
    CHECK_FALSE(r.Contains(Vec2{5, 5}));
    CHECK_FALSE(r.Contains(Vec2{31, 15}));
}

TEST_CASE("Rect::Contains: edge inclusive on top-left, exclusive on bottom-right") {
    Rect r{0, 0, 10, 10};
    CHECK(r.Contains(Vec2{0, 0}));
    CHECK_FALSE(r.Contains(Vec2{10, 10}));
}

TEST_CASE("Rect::Intersects: overlapping rects") {
    Rect a{0, 0, 10, 10};
    Rect b{5, 5, 10, 10};
    CHECK(a.Intersects(b));
    CHECK(b.Intersects(a));
}

TEST_CASE("Rect::Intersects: disjoint rects") {
    Rect a{0, 0, 10, 10};
    Rect b{20, 20, 5, 5};
    CHECK_FALSE(a.Intersects(b));
}
```

- [ ] **Step 2: Run — fails**

Run: `cmake --build build 2>&1 | tail -10`
Expected: file-not-found.

- [ ] **Step 3: Implement `gfx/Rect.h`**

```cpp
#pragma once
#include "raylib.h"
#include "gfx/Vec2.h"

namespace nccu::gfx {

struct Rect {
    float x{0.0f};
    float y{0.0f};
    float width{0.0f};
    float height{0.0f};

    constexpr operator ::Rectangle() const noexcept {
        return ::Rectangle{x, y, width, height};
    }

    constexpr bool Contains(Vec2 p) const noexcept {
        return p.x >= x && p.x < x + width
            && p.y >= y && p.y < y + height;
    }

    constexpr bool Intersects(Rect o) const noexcept {
        return !(o.x >= x + width
              || o.x + o.width <= x
              || o.y >= y + height
              || o.y + o.height <= y);
    }
};

} // namespace nccu::gfx
```

- [ ] **Step 4: Run — passes**

Run: `cmake --build build && (cd build && ctest --output-on-failure -R unit_tests)`
Expected: 6 new Rect cases pass.

- [ ] **Step 5: Commit**

```bash
git add include/gfx/Rect.h tests/test_rect.cpp
git commit -m "feat(gfx): add nccu::gfx::Rect wrapper with Contains/Intersects"
```

---

## Task 4: `nccu::gfx::Key` (type-safe enum)

**Files:**
- Create: `include/gfx/Key.h`

(No unit test — pure mapping, validated by Input tests in Task 6 indirectly and by the smoke run.)

- [ ] **Step 1: Implement `gfx/Key.h`**

```cpp
#pragma once
#include "raylib.h"

namespace nccu::gfx {

enum class Key : int {
    A = KEY_A, B = KEY_B, C = KEY_C, D = KEY_D, E = KEY_E,
    F = KEY_F, G = KEY_G, H = KEY_H, I = KEY_I, J = KEY_J,
    K = KEY_K, L = KEY_L, M = KEY_M, N = KEY_N, O = KEY_O,
    P = KEY_P, Q = KEY_Q, R = KEY_R, S = KEY_S, T = KEY_T,
    U = KEY_U, V = KEY_V, W = KEY_W, X = KEY_X, Y = KEY_Y,
    Z = KEY_Z,
    Space  = KEY_SPACE,
    Enter  = KEY_ENTER,
    Escape = KEY_ESCAPE,
    Up     = KEY_UP,
    Down   = KEY_DOWN,
    Left   = KEY_LEFT,
    Right  = KEY_RIGHT,
};

constexpr int ToRaylibKey(Key k) noexcept { return static_cast<int>(k); }

} // namespace nccu::gfx
```

- [ ] **Step 2: Build — verify zero warnings**

Run: `cmake --build build 2>&1 | tail -5`
Expected: clean build.

- [ ] **Step 3: Commit**

```bash
git add include/gfx/Key.h
git commit -m "feat(gfx): add type-safe Key enum mapping raylib KEY_*"
```

---

## Task 5: `nccu::gfx::Time` (facade)

**Files:**
- Create: `include/gfx/Time.h`

(No unit test — depends on raylib init; smoke-validated.)

- [ ] **Step 1: Implement**

```cpp
#pragma once
#include "raylib.h"

namespace nccu::gfx {

struct Time {
    static float DeltaSeconds() noexcept { return ::GetFrameTime(); }
    static int   FpsAvg()       noexcept { return ::GetFPS(); }
};

} // namespace nccu::gfx
```

- [ ] **Step 2: Build**

Run: `cmake --build build 2>&1 | tail -5`
Expected: clean.

- [ ] **Step 3: Commit**

```bash
git add include/gfx/Time.h
git commit -m "feat(gfx): add Time facade for delta time + fps"
```

---

## Task 6: `nccu::gfx::Input` (facade)

**Files:**
- Create: `include/gfx/Input.h`

- [ ] **Step 1: Implement**

```cpp
#pragma once
#include "raylib.h"
#include "gfx/Key.h"

namespace nccu::gfx {

struct Input {
    static bool IsDown(Key k) noexcept     { return ::IsKeyDown(ToRaylibKey(k)); }
    static bool IsPressed(Key k) noexcept  { return ::IsKeyPressed(ToRaylibKey(k)); }
    static bool IsReleased(Key k) noexcept { return ::IsKeyReleased(ToRaylibKey(k)); }
};

} // namespace nccu::gfx
```

- [ ] **Step 2: Build**

Run: `cmake --build build 2>&1 | tail -5`
Expected: clean.

- [ ] **Step 3: Commit**

```bash
git add include/gfx/Input.h
git commit -m "feat(gfx): add Input facade over raylib key polling"
```

---

## Task 7: `nccu::gfx::Window` (RAII + Builder)

**Files:**
- Create: `include/gfx/Window.h`

- [ ] **Step 1: Implement**

```cpp
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

    Window(const Window&) = delete;
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
```

- [ ] **Step 2: Build**

Run: `cmake --build build 2>&1 | tail -5`
Expected: clean.

- [ ] **Step 3: Commit**

```bash
git add include/gfx/Window.h
git commit -m "feat(gfx): add Window RAII with fluent Builder"
```

---

## Task 8: `nccu::gfx::DrawScope` (RAII)

**Files:**
- Create: `include/gfx/DrawScope.h`

- [ ] **Step 1: Implement**

```cpp
#pragma once
#include "raylib.h"

namespace nccu::gfx {

class DrawScope {
public:
    DrawScope() noexcept { ::BeginDrawing(); }
    ~DrawScope() { ::EndDrawing(); }

    DrawScope(const DrawScope&) = delete;
    DrawScope& operator=(const DrawScope&) = delete;
    DrawScope(DrawScope&&) = delete;
    DrawScope& operator=(DrawScope&&) = delete;
};

} // namespace nccu::gfx
```

- [ ] **Step 2: Build**

Run: `cmake --build build 2>&1 | tail -5`
Expected: clean.

- [ ] **Step 3: Commit**

```bash
git add include/gfx/DrawScope.h
git commit -m "feat(gfx): add DrawScope RAII for BeginDrawing/EndDrawing"
```

---

## Task 9: `nccu::gfx::Renderer` (fluent draw helper)

**Files:**
- Create: `include/gfx/Renderer.h`

- [ ] **Step 1: Implement**

```cpp
#pragma once
#include "raylib.h"
#include "gfx/Color.h"
#include "gfx/Rect.h"
#include "gfx/Vec2.h"

namespace nccu::gfx {

class Renderer {
public:
    Renderer& Clear(Color c) noexcept {
        ::ClearBackground(c);
        return *this;
    }

    Renderer& Rect(struct Rect r, Color c) noexcept {
        ::DrawRectangleRec(r, c);
        return *this;
    }

    Renderer& RectLines(struct Rect r, Color c, float thickness = 1.0f) noexcept {
        ::DrawRectangleLinesEx(r, thickness, c);
        return *this;
    }

    Renderer& Pixel(Vec2 p, Color c) noexcept {
        ::DrawPixelV(p, c);
        return *this;
    }
};

} // namespace nccu::gfx
```

- [ ] **Step 2: Build**

Run: `cmake --build build 2>&1 | tail -5`
Expected: clean.

- [ ] **Step 3: Commit**

```bash
git add include/gfx/Renderer.h
git commit -m "feat(gfx): add fluent Renderer for primitive drawing"
```

---

## Task 10: `nccu::gfx::TextBuilder` (Builder pattern)

**Files:**
- Create: `include/gfx/TextBuilder.h`
- Test: `tests/test_text_builder.cpp`

- [ ] **Step 1: Write failing test**

Create `tests/test_text_builder.cpp`:
```cpp
#include "doctest/doctest.h"
#include "gfx/TextBuilder.h"
#include "gfx/Color.h"
#include "gfx/Vec2.h"

using nccu::gfx::TextBuilder;
using nccu::gfx::Vec2;
using nccu::gfx::Colors;

TEST_CASE("TextBuilder fluent setters preserve state") {
    TextBuilder t{"hi"};
    t.At(Vec2{10, 20}).Size(16).Color(Colors::Red);
    CHECK(t.GetPosition().x == doctest::Approx(10.0f));
    CHECK(t.GetPosition().y == doctest::Approx(20.0f));
    CHECK(t.GetSize() == 16);
    CHECK(t.GetColor() == Colors::Red);
}

TEST_CASE("TextBuilder defaults: pos (0,0), size 10, color black") {
    TextBuilder t{"x"};
    CHECK(t.GetSize() == 10);
    CHECK(t.GetColor() == Colors::Black);
}

TEST_CASE("TextBuilder chaining returns self by reference") {
    TextBuilder t{"x"};
    auto& ref = t.At(Vec2{1, 1}).Size(5).Color(Colors::Blue);
    CHECK(&ref == &t);
}
```

- [ ] **Step 2: Run — fails**

Run: `cmake --build build 2>&1 | tail -10`
Expected: file-not-found.

- [ ] **Step 3: Implement**

```cpp
#pragma once
#include "raylib.h"
#include "gfx/Color.h"
#include "gfx/Vec2.h"
#include <string>

namespace nccu::gfx {

class TextBuilder {
public:
    explicit TextBuilder(std::string text) : text_(std::move(text)) {}

    TextBuilder& At(Vec2 p)       noexcept { position_ = p; return *this; }
    TextBuilder& Size(int s)      noexcept { size_ = s; return *this; }
    TextBuilder& Color(struct Color c) noexcept { color_ = c; return *this; }

    void Draw() const {
        ::DrawText(text_.c_str(),
                   static_cast<int>(position_.x),
                   static_cast<int>(position_.y),
                   size_,
                   color_);
    }

    Vec2          GetPosition() const noexcept { return position_; }
    int           GetSize()     const noexcept { return size_; }
    struct Color  GetColor()    const noexcept { return color_; }

private:
    std::string text_;
    Vec2 position_{0.0f, 0.0f};
    int  size_{10};
    struct Color color_{0, 0, 0, 255};
};

} // namespace nccu::gfx
```

- [ ] **Step 4: Run — passes**

Run: `cmake --build build && (cd build && ctest --output-on-failure -R unit_tests)`
Expected: 3 new TextBuilder cases pass.

- [ ] **Step 5: Commit**

```bash
git add include/gfx/TextBuilder.h tests/test_text_builder.cpp
git commit -m "feat(gfx): add TextBuilder with chained setters + Draw"
```

---

## Task 11: Migrate game headers to wrapper types

This task is the riskiest — it changes types in headers used by every cpp file. Do it in one commit so build never breaks.

**Files modified:**
- `include/EventBus.h`
- `include/GameObject.h`
- `include/Character.h`
- `include/Item.h`
- `include/Player.h`
- `include/TransparentUmbrella.h`
- `include/TrueUmbrella.h`, `FragileUmbrella.h`, `ProfessorTrapUmbrella.h`, `CursedUmbrella.h`
- `src/TransparentUmbrella.cpp`, `TrueUmbrella.cpp`, `FragileUmbrella.cpp`, `ProfessorTrapUmbrella.cpp`, `CursedUmbrella.cpp`
- `src/Player.cpp` (signature only — full migration later)
- `src/EventBus.cpp` (if any raylib type leaks in implementation)
- `tests/test_factory.cpp`, `test_player.cpp` — adjust literal types if needed

- [ ] **Step 1: Update `include/EventBus.h`**

Replace contents:
```cpp
#pragma once
#include "gfx/Color.h"
#include "gfx/Vec2.h"
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

enum class EventType {
    RenderRequested,
    UmbrellaClaimed,
    KarmaChanged,
    ShowMessage,
};

struct Event {
    EventType            type;
    nccu::gfx::Vec2      position;
    nccu::gfx::Color     color;
    std::string          text;
};

class EventBus {
public:
    using Handler = std::function<void(const Event&)>;

    static EventBus& Instance();

    void Subscribe(EventType type, Handler handler);
    void Publish(const Event& event) const;
    void Clear();

private:
    EventBus() = default;
    std::unordered_map<EventType, std::vector<Handler>> handlers_;
};
```

- [ ] **Step 2: Update `include/GameObject.h`**

Replace `Vector2` → `Vec2`, `Rectangle` → `Rect`, drop raylib include:
```cpp
#pragma once
#include "gfx/Vec2.h"
#include "gfx/Rect.h"

class Player;

class GameObject {
public:
    GameObject(nccu::gfx::Vec2 position, nccu::gfx::Rect hitBox)
        : position_(position), hitBox_(hitBox), isActive_(true), collisionLayer_(0) {}

    virtual ~GameObject() = default;

    virtual void Update(float deltaTime) = 0;
    virtual void Draw() const = 0;
    virtual void Interact(Player* initiator) = 0;

    bool CheckCollision(nccu::gfx::Rect other) const {
        return hitBox_.Intersects(other);
    }

    bool IsActive() const { return isActive_; }
    void Deactivate() { isActive_ = false; }
    nccu::gfx::Vec2 GetPosition() const { return position_; }

protected:
    nccu::gfx::Vec2 position_;
    nccu::gfx::Rect hitBox_;
    bool isActive_;
    int collisionLayer_;
};
```

- [ ] **Step 3: Update `include/Character.h`**

```cpp
#pragma once
#include "GameObject.h"
#include "gfx/Vec2.h"

class Character : public GameObject {
public:
    Character(nccu::gfx::Vec2 position, nccu::gfx::Rect hitBox, float speed)
        : GameObject(position, hitBox), speed_(speed),
          direction_({0.0f, 0.0f}), currentFrame_(0) {}

    void Move(nccu::gfx::Vec2 dir, float deltaTime) {
        nccu::gfx::Vec2 n = dir.Normalized();
        position_.x += n.x * speed_ * deltaTime;
        position_.y += n.y * speed_ * deltaTime;
        hitBox_.x = position_.x;
        hitBox_.y = position_.y;
        direction_ = n;
    }

protected:
    float speed_;
    nccu::gfx::Vec2 direction_;
    int currentFrame_;
};
```

- [ ] **Step 4: Update `include/Item.h`**

```cpp
#pragma once
#include "GameObject.h"
#include <string>

class Item : public GameObject {
public:
    Item(nccu::gfx::Vec2 position, nccu::gfx::Rect hitBox, std::string name)
        : GameObject(position, hitBox), itemName_(std::move(name)), isPickable_(true) {}

    virtual void OnPickup(Player* player) = 0;

    const std::string& GetName() const { return itemName_; }
    bool IsPickable() const { return isPickable_; }

protected:
    std::string itemName_;
    bool isPickable_;
};
```

- [ ] **Step 5: Update `include/Player.h`**

```cpp
#pragma once
#include "Character.h"

class Player : public Character {
public:
    explicit Player(nccu::gfx::Vec2 position);

    void Update(float deltaTime) override;
    void Draw() const override;
    void Interact(Player* initiator) override;

    void HandleInput(float deltaTime);
    void decreaseKarma(int amount);
    void resetRainMeter();

    int GetKarma() const { return karma_; }
    float GetRainMeter() const { return rainMeter_; }
    bool HasUmbrella() const { return hasUmbrella_; }
    void SetHasUmbrella(bool v) { hasUmbrella_ = v; }

private:
    float rainMeter_;
    int karma_;
    bool hasUmbrella_;
};
```

- [ ] **Step 6: Update `include/TransparentUmbrella.h`**

```cpp
#pragma once
#include "Item.h"
#include "gfx/Color.h"

class TransparentUmbrella : public Item {
public:
    TransparentUmbrella(nccu::gfx::Vec2 position, std::string name, nccu::gfx::Color tint)
        : Item(position, nccu::gfx::Rect{position.x, position.y, 20.0f, 20.0f}, std::move(name)),
          umbrellaTint_(tint) {}

    void Update(float /*deltaTime*/) override {}
    void Draw() const override;
    void Interact(Player* initiator) override { beClaimed(initiator); }
    void OnPickup(Player* player) override { beClaimed(player); }

    virtual void beClaimed(Player* player) = 0;

protected:
    nccu::gfx::Color umbrellaTint_;
};
```

- [ ] **Step 7: Update each concrete umbrella header**

`include/TrueUmbrella.h`:
```cpp
#pragma once
#include "TransparentUmbrella.h"

class TrueUmbrella : public TransparentUmbrella {
public:
    explicit TrueUmbrella(nccu::gfx::Vec2 position)
        : TransparentUmbrella(position, "TrueUmbrella", nccu::gfx::Color{180, 230, 255, 255}) {}

    void beClaimed(Player* player) override;
};
```

`include/FragileUmbrella.h`:
```cpp
#pragma once
#include "TransparentUmbrella.h"

class FragileUmbrella : public TransparentUmbrella {
public:
    explicit FragileUmbrella(nccu::gfx::Vec2 position)
        : TransparentUmbrella(position, "FragileUmbrella", nccu::gfx::Color{200, 220, 235, 255}),
          leakRate_(0.5f) {}

    void beClaimed(Player* player) override;

    float GetLeakRate() const { return leakRate_; }

private:
    float leakRate_;
};
```

`include/ProfessorTrapUmbrella.h`:
```cpp
#pragma once
#include "TransparentUmbrella.h"

class ProfessorTrapUmbrella : public TransparentUmbrella {
public:
    explicit ProfessorTrapUmbrella(nccu::gfx::Vec2 position)
        : TransparentUmbrella(position, "ProfessorTrapUmbrella", nccu::gfx::Color{210, 200, 230, 255}),
          spawnedEnemiesCount_(0) {}

    void beClaimed(Player* player) override;

    int GetSpawnedEnemiesCount() const { return spawnedEnemiesCount_; }

private:
    int spawnedEnemiesCount_;
};
```

`include/CursedUmbrella.h`:
```cpp
#pragma once
#include "TransparentUmbrella.h"

class CursedUmbrella : public TransparentUmbrella {
public:
    explicit CursedUmbrella(nccu::gfx::Vec2 position)
        : TransparentUmbrella(position, "CursedUmbrella", nccu::gfx::Color{120, 100, 140, 255}),
          karmaPenalty_(50) {}

    void beClaimed(Player* player) override;

    int GetKarmaPenalty() const { return karmaPenalty_; }

private:
    int karmaPenalty_;
};
```

- [ ] **Step 8: Update `include/GameObjectFactory.h`**

```cpp
#pragma once
#include "GameObject.h"
#include "gfx/Vec2.h"
#include <memory>

enum class ObjectType {
    Player,
    TrueUmbrella,
    FragileUmbrella,
    ProfessorTrapUmbrella,
    CursedUmbrella,
};

class GameObjectFactory {
public:
    static std::unique_ptr<GameObject> Create(ObjectType type, nccu::gfx::Vec2 position);
};
```

- [ ] **Step 9: Update `src/GameObjectFactory.cpp` signature**

Change first line of `Create` to `nccu::gfx::Vec2 position`.

- [ ] **Step 10: Update each umbrella `.cpp` to construct `Vec2` / `Color` payloads**

For `src/TrueUmbrella.cpp`:
```cpp
#include "TrueUmbrella.h"
#include "Player.h"
#include "EventBus.h"
#include "gfx/Color.h"

void TrueUmbrella::beClaimed(Player* player) {
    if (!player) return;
    player->SetHasUmbrella(true);
    isActive_ = false;
    EventBus::Instance().Publish(Event{
        EventType::UmbrellaClaimed,
        position_,
        umbrellaTint_,
        "TrueUmbrella"
    });
    EventBus::Instance().Publish(Event{
        EventType::ShowMessage,
        position_,
        nccu::gfx::Colors::White,
        "你撿到了 TrueUmbrella，雨停了。"
    });
}
```

Apply the same pattern (replace `WHITE` literal with `nccu::gfx::Colors::White`) to:
- `src/FragileUmbrella.cpp`
- `src/ProfessorTrapUmbrella.cpp`
- `src/CursedUmbrella.cpp`

- [ ] **Step 11: Update `src/TransparentUmbrella.cpp`**

```cpp
#include "TransparentUmbrella.h"
#include "EventBus.h"

void TransparentUmbrella::Draw() const {
    EventBus::Instance().Publish(Event{
        EventType::RenderRequested,
        position_,
        umbrellaTint_,
        itemName_
    });
}
```

(No raylib include — `position_` is now `Vec2`, `umbrellaTint_` is `Color`.)

- [ ] **Step 12: Update `src/Player.cpp` constructor signature only**

Change first line of constructor to `Player::Player(nccu::gfx::Vec2 position)` and replace literal `{position.x, position.y, 24.0f, 24.0f}` with `nccu::gfx::Rect{position.x, position.y, 24.0f, 24.0f}`. (Full Draw/Input migration in Task 12.)

- [ ] **Step 13: Update `tests/test_factory.cpp` and `test_player.cpp`**

Replace `{0, 0}` / `{100, 100}` etc. literals where they constructed `Vector2` with explicit `nccu::gfx::Vec2{0, 0}`. doctest aggregate-init normally allows `{0, 0}` to bind to `Vec2`, so usually no change needed; verify by build.

- [ ] **Step 14: Build and run tests**

Run: `cmake --build build 2>&1 | tail -20`
Run: `(cd build && ctest --output-on-failure)`
Expected: clean build (zero warnings), all tests pass.

If a test compile fails because aggregate-init became ambiguous, change the literal to `nccu::gfx::Vec2{x, y}`.

- [ ] **Step 15: Commit**

```bash
git add include/EventBus.h include/GameObject.h include/Character.h \
        include/Item.h include/Player.h include/TransparentUmbrella.h \
        include/TrueUmbrella.h include/FragileUmbrella.h \
        include/ProfessorTrapUmbrella.h include/CursedUmbrella.h \
        include/GameObjectFactory.h \
        src/GameObjectFactory.cpp \
        src/TransparentUmbrella.cpp src/TrueUmbrella.cpp \
        src/FragileUmbrella.cpp src/ProfessorTrapUmbrella.cpp \
        src/CursedUmbrella.cpp \
        src/Player.cpp \
        tests/test_factory.cpp tests/test_player.cpp
git commit -m "refactor: migrate game types to nccu::gfx::Vec2/Rect/Color"
```

---

## Task 12: Migrate `Player.cpp` Draw + HandleInput

**Files modified:**
- `src/Player.cpp`

- [ ] **Step 1: Replace `src/Player.cpp` body**

```cpp
#include "Player.h"
#include "gfx/Renderer.h"
#include "gfx/Input.h"
#include "gfx/Key.h"
#include "gfx/Color.h"

Player::Player(nccu::gfx::Vec2 position)
    : Character(position, nccu::gfx::Rect{position.x, position.y, 24.0f, 24.0f}, 180.0f),
      rainMeter_(0.0f), karma_(50), hasUmbrella_(false) {}

void Player::Update(float deltaTime) {
    HandleInput(deltaTime);
}

void Player::Draw() const {
    nccu::gfx::Renderer{}.Rect(hitBox_, nccu::gfx::Colors::Blue);
}

void Player::Interact(Player* /*initiator*/) {
    // Player does not respond to other Players in MVP
}

void Player::HandleInput(float deltaTime) {
    using nccu::gfx::Input;
    using nccu::gfx::Key;
    nccu::gfx::Vec2 dir{0.0f, 0.0f};
    if (Input::IsDown(Key::W)) dir.y -= 1.0f;
    if (Input::IsDown(Key::S)) dir.y += 1.0f;
    if (Input::IsDown(Key::A)) dir.x -= 1.0f;
    if (Input::IsDown(Key::D)) dir.x += 1.0f;
    Move(dir, deltaTime);
}

void Player::decreaseKarma(int amount) {
    karma_ -= amount;
}

void Player::resetRainMeter() {
    rainMeter_ = 0.0f;
}
```

- [ ] **Step 2: Build + test**

Run: `cmake --build build 2>&1 | tail -8`
Run: `(cd build && ctest --output-on-failure)`
Expected: clean build, all tests pass.

- [ ] **Step 3: Commit**

```bash
git add src/Player.cpp
git commit -m "refactor(player): use gfx::Renderer + gfx::Input wrappers"
```

---

## Task 13: Migrate `main.cpp` to use the wrapper layer

**Files modified:**
- `src/main.cpp`

- [ ] **Step 1: Replace `src/main.cpp` body**

```cpp
#include "GameObject.h"
#include "GameObjectFactory.h"
#include "Player.h"
#include "EventBus.h"
#include "gfx/Window.h"
#include "gfx/DrawScope.h"
#include "gfx/Renderer.h"
#include "gfx/TextBuilder.h"
#include "gfx/Input.h"
#include "gfx/Key.h"
#include "gfx/Time.h"
#include "gfx/Color.h"
#include "gfx/Vec2.h"
#include "gfx/Rect.h"
#include <memory>
#include <vector>
#include <iostream>
#include <algorithm>

int main() {
    using namespace nccu::gfx;

    auto win = Window::Builder()
                   .Title("Lost Umbrella - MVP")
                   .Size(800, 450)
                   .Fps(60)
                   .Open();

    EventBus::Instance().Subscribe(EventType::ShowMessage,
        [](const Event& e) { std::cout << "[UI] " << e.text << '\n'; });
    EventBus::Instance().Subscribe(EventType::UmbrellaClaimed,
        [](const Event& e) { std::cout << "[Game] Claimed: " << e.text << '\n'; });
    EventBus::Instance().Subscribe(EventType::RenderRequested,
        [](const Event& e) {
            Renderer{}.Rect(Rect{e.position.x, e.position.y, 20, 20}, e.color);
        });

    std::vector<std::unique_ptr<GameObject>> objects;
    objects.push_back(GameObjectFactory::Create(ObjectType::Player,                Vec2{400, 225}));
    objects.push_back(GameObjectFactory::Create(ObjectType::TrueUmbrella,          Vec2{150, 100}));
    objects.push_back(GameObjectFactory::Create(ObjectType::FragileUmbrella,       Vec2{300, 100}));
    objects.push_back(GameObjectFactory::Create(ObjectType::ProfessorTrapUmbrella, Vec2{500, 100}));
    objects.push_back(GameObjectFactory::Create(ObjectType::CursedUmbrella,        Vec2{650, 100}));

    Player* player = dynamic_cast<Player*>(objects.front().get());

    while (!win.ShouldClose()) {
        float dt = Time::DeltaSeconds();

        for (auto& obj : objects) {
            if (obj && obj->IsActive()) obj->Update(dt);
        }

        if (Input::IsPressed(Key::E) && player) {
            for (auto& obj : objects) {
                if (!obj || !obj->IsActive() || obj.get() == player) continue;
                Rect pHit{player->GetPosition().x, player->GetPosition().y, 24, 24};
                if (obj->CheckCollision(pHit)) {
                    obj->Interact(player);
                }
            }
        }

        {
            DrawScope frame;
            Renderer{}.Clear(Colors::RayWhite);
            for (auto& obj : objects) {
                if (obj && obj->IsActive()) obj->Draw();
            }
            TextBuilder{"WASD: move    E: pick up"}
                .At(Vec2{10, 10}).Size(16).Color(Colors::DarkGray).Draw();
            if (player) {
                char buf[64];
                std::snprintf(buf, sizeof(buf), "karma: %d   umbrella: %s",
                    player->GetKarma(), player->HasUmbrella() ? "yes" : "no");
                TextBuilder{buf}
                    .At(Vec2{10, 30}).Size(16).Color(Colors::DarkGray).Draw();
            }
        }

        // End-of-frame sweep: deferred deletion to avoid iterator invalidation
        objects.erase(
            std::remove_if(objects.begin(), objects.end(),
                [](const std::unique_ptr<GameObject>& o) {
                    return !o || !o->IsActive();
                }),
            objects.end());
        if (player && std::find_if(objects.begin(), objects.end(),
                [player](const std::unique_ptr<GameObject>& o) { return o.get() == player; })
            == objects.end()) {
            player = nullptr;
        }
    }

    return 0;
}
```

- [ ] **Step 2: Add the `<cstdio>` include if `snprintf` warns**

If the build warns about `snprintf`, add `#include <cstdio>` after `<algorithm>` in main.cpp.

- [ ] **Step 3: Build**

Run: `cmake --build build 2>&1 | tail -8`
Expected: zero warnings.

- [ ] **Step 4: Smoke run for ≥ 5 s**

```bash
./build/OOP_Raylib_Lab > /tmp/run.log 2>&1 &
PID=$!
sleep 5
if kill -0 $PID 2>/dev/null; then
  kill $PID; wait $PID 2>/dev/null
  echo "OK: survived 5s"
else
  echo "CRASHED"
  tail -20 /tmp/run.log
  exit 1
fi
```
Expected: `OK: survived 5s`.

- [ ] **Step 5: Commit**

```bash
git add src/main.cpp
git commit -m "refactor(main): use Window/DrawScope/Renderer/TextBuilder/Input/Time"
```

---

## Task 14: Strip transitive raylib types from EventBus implementation if any

`src/EventBus.cpp` should be raylib-free already — verify.

- [ ] **Step 1: Check current contents**

Run: `grep -n "raylib\|Vector2\|Rectangle" src/EventBus.cpp || echo CLEAN`
Expected: `CLEAN`. If matches appear, replace with the wrapper equivalents and re-build/test.

- [ ] **Step 2: If modified, commit**

If anything changed:
```bash
git add src/EventBus.cpp
git commit -m "refactor(eventbus): drop transitive raylib dependency"
```

If nothing changed, skip the commit.

---

## Task 15: Final verification gate

- [ ] **Step 1: Self-containment grep**

Run:
```bash
grep -rn "raylib.h" src/ tests/ include/ \
  --include="*.cpp" --include="*.h" \
  | grep -v "include/gfx/"
```
Expected: empty output. If any line appears, edit that file to use `gfx/*.h` instead, rebuild, re-test, and commit.

- [ ] **Step 2: Naming-hygiene grep (forbidden tokens in tracked files)**

Run:
```bash
git ls-files | xargs grep -l -E "Claude|Codex|claude-code|superpowers|nanobanana|Gemini|Anthropic|AI agent|CLAUDE\.md|AGENTS\.md" 2>/dev/null || echo CLEAN
```
Expected: `CLEAN`. If any tracked file matches, edit it to remove the term and amend or commit a fix.

- [ ] **Step 3: Build with strict warnings**

Run: `cmake --build build --clean-first 2>&1 | tee /tmp/build.log | tail -10`
Expected: zero warnings, exit 0. `grep -c warning /tmp/build.log` should be 0.

- [ ] **Step 4: Run all tests**

Run: `(cd build && ctest --output-on-failure)`
Expected: all cases pass; total assertions ≥ original 17 + new ≥ 25.

- [ ] **Step 5: 5-second smoke run**

Run: same harness as Task 13 Step 4. Expected: `OK: survived 5s`.

- [ ] **Step 6: Inspect window manually for visual parity (optional but recommended)**

Run: `./build/OOP_Raylib_Lab` for a few seconds with a real keyboard. Verify:
- Player blue square at center
- Four umbrella-tinted squares in a row near the top
- HUD lines "WASD: move    E: pick up" and "karma: 50   umbrella: no"
- Walking with WASD moves the blue square; diagonal not √2× faster
- Stepping onto an umbrella + pressing E removes it and prints `[Game] Claimed: …`
- Picking up `CursedUmbrella` decreases karma display

Close window. No final commit unless you fixed something during this manual check.

- [ ] **Step 7 (optional): Squash worktree commits before merging back to main**

If you choose to squash:
```bash
# from worktree
git rebase -i $(git merge-base HEAD main)
# pick the first, fixup the rest into a single "feat(gfx): introduce nccu::gfx wrapper layer" commit
```
Otherwise leave the granular commits — they trace the TDD process.

---

## Self-Review Notes

**Coverage of spec (`docs/superpowers/specs/2026-05-11-raylib-modern-cpp-wrapper-design.md`):**

| Spec section | Plan task |
|---|---|
| §4 Architecture (10 wrapper headers) | Tasks 1–10 |
| §5 Pattern map | Tasks 1 (Adapter+Fluent), 2-3 (Adapter), 6 (Facade), 7 (RAII+Builder), 8 (RAII), 9 (Fluent), 10 (Builder) |
| §6 Touch points (existing files) | Task 11 (headers + cpp shells), 12 (Player.cpp body), 13 (main.cpp) |
| §7 Tests | Tasks 1, 2, 3, 10 (new test files), Task 11 step 13 (existing test adjustment if needed) |
| §8 Verification gate | Task 15 |
| §9 Implementation order | Tasks 1→4→5→6→7→8→9→10→2→3 (note: Vec2/Rect actually needed for Renderer, so the spec's bottom-up order is preserved as 1, 2, 3, 4, 5, 6, 7, 8, 9, 10) |
| §10 Risks (DrawScope ordering, Window move-only, no Singleton, internal-only conversions, EventBus type migration) | Task 7 (move-only), Task 8 (DrawScope non-movable), Task 11 (EventBus migration) |

**Type consistency:** `Vec2`, `Rect`, `Color` namespaces match throughout (`nccu::gfx::*`). `Player::Player(nccu::gfx::Vec2)` declared in Task 11 step 5, defined in Task 12 step 1 — signatures match.

No placeholders. Every code block is complete and copy-pastable.
