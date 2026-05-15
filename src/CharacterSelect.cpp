#include "CharacterSelect.h"
#include "gfx/DrawScope.h"
#include "gfx/Renderer.h"
#include "gfx/TextBuilder.h"
#include "gfx/Input.h"
#include "gfx/Key.h"
#include "gfx/Texture.h"
#include "gfx/Color.h"
#include "gfx/Vec2.h"
#include "gfx/Rect.h"
#include <cstdio>
#include <vector>

namespace nccu {
namespace {

constexpr int kWinW       = 800;
constexpr int kWinH       = 450;
constexpr int kFrameSize  = 32;  // each Pipoya cell is 32x32
constexpr int kIdleCol    = 1;   // middle column of the 3-frame walk strip
constexpr int kDownRow    = 0;   // first row faces the camera

constexpr int kFemaleCount = 18;
constexpr int kMaleCount   = 13;

constexpr gfx::Color kHighlight{255, 153, 0, 255};
constexpr gfx::Color kDim      {160, 160, 160, 255};

std::string SpritePath(bool female, int index1based) {
    char buf[96];
    std::snprintf(buf, sizeof(buf),
                  "resources/assets/sprites/school_uniform_3/%s_%02d.png",
                  female ? "female" : "male", index1based);
    return buf;
}

gfx::Rect IdleSrc() {
    return gfx::Rect{
        static_cast<float>(kIdleCol * kFrameSize),
        static_cast<float>(kDownRow * kFrameSize),
        static_cast<float>(kFrameSize),
        static_cast<float>(kFrameSize)};
}

// Phase 1. Returns true if the player confirmed; false if the window was
// closed. On confirm, writes the picked gender to `out_female`.
bool RunGenderPhase(gfx::Window& win,
                    const gfx::Texture& femalePreview,
                    const gfx::Texture& malePreview,
                    bool& out_female) {
    using namespace gfx;
    bool female = true;
    while (!win.ShouldClose()) {
        if (Input::IsPressed(Key::Left)  || Input::IsPressed(Key::A)) female = true;
        if (Input::IsPressed(Key::Right) || Input::IsPressed(Key::D)) female = false;
        if (Input::IsPressed(Key::Enter)) { out_female = female; return true; }
        {
            DrawScope frame;
            Renderer r;
            r.Clear(Colors::RayWhite);
            TextBuilder{"CHOOSE YOUR GENDER"}
                .At(Vec2{255, 40}).Size(28).Color(Colors::DarkGray).Draw();

            const Rect src = IdleSrc();
            const Rect destFemale{200, 150, 128, 128};
            const Rect destMale  {472, 150, 128, 128};
            r.TextureRect(femalePreview, src, destFemale);
            r.TextureRect(malePreview,   src, destMale);
            r.RectLines(female ? destFemale : destMale, kHighlight, 3.0f);

            TextBuilder{"Female"}.At(Vec2{232, 290}).Size(20)
                .Color(female ? Colors::Black : kDim).Draw();
            TextBuilder{"Male"}.At(Vec2{518, 290}).Size(20)
                .Color(female ? kDim : Colors::Black).Draw();
            TextBuilder{"<- ->  toggle      Enter  confirm"}
                .At(Vec2{215, 380}).Size(18).Color(Colors::DarkGray).Draw();
        }
    }
    return false;
}

// Phase 2. Returns true if the player either confirmed (back_pressed=false,
// out_index = chosen 0-based slot) or pressed Esc to back out
// (back_pressed=true). Returns false only if the window was closed.
bool RunFigurePhase(gfx::Window& win,
                    const std::vector<gfx::Texture>& sheets,
                    bool female,
                    int& out_index,
                    bool& back_pressed) {
    using namespace gfx;
    const int count = static_cast<int>(sheets.size());
    constexpr int kCols = 6;
    constexpr int kTile = 64;
    constexpr int kGap  = 8;
    const int kRows = (count + kCols - 1) / kCols;
    const int gridW = kCols * kTile + (kCols - 1) * kGap;
    const int gridH = kRows * kTile + (kRows - 1) * kGap;
    const int gridX = (kWinW - gridW) / 2;
    const int gridY = 90;
    int cursor = 0;

    while (!win.ShouldClose()) {
        if (Input::IsPressed(Key::Right) || Input::IsPressed(Key::D))
            cursor = (cursor + 1) % count;
        if (Input::IsPressed(Key::Left)  || Input::IsPressed(Key::A))
            cursor = (cursor - 1 + count) % count;
        if (Input::IsPressed(Key::Down)  || Input::IsPressed(Key::S)) {
            const int next = cursor + kCols;
            if (next < count) cursor = next;
        }
        if (Input::IsPressed(Key::Up)    || Input::IsPressed(Key::W)) {
            const int next = cursor - kCols;
            if (next >= 0) cursor = next;
        }
        if (Input::IsPressed(Key::Enter))  { out_index = cursor; back_pressed = false; return true; }
        if (Input::IsPressed(Key::Escape)) { out_index = -1;     back_pressed = true;  return true; }

        {
            DrawScope frame;
            Renderer r;
            r.Clear(Colors::RayWhite);
            const std::string title =
                std::string{"CHOOSE YOUR FIGURE ("} + (female ? "Female" : "Male") + ")";
            TextBuilder{title}.At(Vec2{225, 35}).Size(24).Color(Colors::DarkGray).Draw();

            const Rect src = IdleSrc();
            for (int i = 0; i < count; ++i) {
                const int col = i % kCols;
                const int row = i / kCols;
                const Rect dest{
                    static_cast<float>(gridX + col * (kTile + kGap)),
                    static_cast<float>(gridY + row * (kTile + kGap)),
                    static_cast<float>(kTile),
                    static_cast<float>(kTile)};
                r.TextureRect(sheets[i], src, dest);
                if (i == cursor)
                    r.RectLines(dest, kHighlight, 3.0f);
            }
            TextBuilder{"Arrows navigate    Enter confirm    Esc back"}
                .At(Vec2{195.0f, static_cast<float>(gridY + gridH + 30)}).Size(18)
                .Color(Colors::DarkGray).Draw();
        }
    }
    return false;
}

} // namespace

CharacterSelectResult RunCharacterSelect(gfx::Window& win) {
    using namespace gfx;
    CharacterSelectResult result;

    Texture femalePreview = Texture::Load(SpritePath(true,  1));
    Texture malePreview   = Texture::Load(SpritePath(false, 1));

    while (!win.ShouldClose()) {
        bool female = true;
        if (!RunGenderPhase(win, femalePreview, malePreview, female)) {
            result.closed = true;
            return result;
        }
        std::vector<Texture> sheets;
        const int count = female ? kFemaleCount : kMaleCount;
        sheets.reserve(static_cast<size_t>(count));
        for (int i = 1; i <= count; ++i)
            sheets.push_back(Texture::Load(SpritePath(female, i)));
        int picked = -1;
        bool back = false;
        if (!RunFigurePhase(win, sheets, female, picked, back)) {
            result.closed = true;
            return result;
        }
        if (back) continue;  // sheets unload at scope end, return to gender phase
        result.spritePath = SpritePath(female, picked + 1);
        return result;
    }
    result.closed = true;
    return result;
}

} // namespace nccu
