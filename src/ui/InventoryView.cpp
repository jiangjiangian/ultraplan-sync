#include "ui/InventoryView.h"
#include "gfx/IRenderer.h"
#include "gfx/Rect.h"
#include "gfx/Vec2.h"
#include "gfx/Color.h"

#include <algorithm>
#include <string>
#include <vector>

namespace nccu {

using namespace nccu::gfx;

void DrawInventory(IRenderer& r,
                   const std::unordered_map<std::string, int>& items,
                   float screenW, float screenH) {
    // Dim the (frozen) world, then a centred panel — same overlay idiom
    // as the dialog box / ending card.
    r.DrawRect(Rect{0.0f, 0.0f, screenW, screenH}, Color{0, 0, 0, 140});

    const float pw = 320.0f;
    const float ph = 240.0f;
    const float px = screenW * 0.5f - pw * 0.5f;
    const float py = screenH * 0.5f - ph * 0.5f;
    r.DrawRect(Rect{px, py, pw, ph}, Color{30, 30, 40, 235});

    r.DrawText("物品欄", Vec2{px + 16.0f, py + 14.0f}, 24, Colors::White);

    // Deterministic order (unordered_map iteration is unspecified): sort
    // by itemId so the panel — and the test — are stable. A zero count
    // is skipped (ConsumeOne erases at 0, but be defensive).
    std::vector<std::pair<std::string, int>> rows;
    rows.reserve(items.size());
    for (const auto& kv : items)
        if (kv.second > 0) rows.emplace_back(kv.first, kv.second);
    std::sort(rows.begin(), rows.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    float y = py + 56.0f;
    if (rows.empty()) {
        r.DrawText("（空）", Vec2{px + 16.0f, y}, 18,
                   Color{200, 200, 200, 255});
        return;
    }
    for (const auto& [id, n] : rows) {
        r.DrawText(id + " x" + std::to_string(n),
                   Vec2{px + 16.0f, y}, 18, Colors::White);
        y += 28.0f;
    }
}

} // namespace nccu
