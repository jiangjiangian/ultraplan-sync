#ifndef INVENTORY_VIEW_H_
#define INVENTORY_VIEW_H_
#include <string>
#include <unordered_map>

namespace nccu {
namespace gfx { class IRenderer; }

// Tab inventory overlay: a centred panel listing every held consumable
// as "<itemId> x<count>", or a single "（空）" line when nothing is
// held. Self-contained and spy-testable like DrawEndingCard / DrawDialog
// — View draws it on top of the (frozen) world when World::InventoryOpen
// is true. No raylib here; all drawing goes through the injected
// IRenderer. Pure function of the count map -> deterministic to test.
void DrawInventory(nccu::gfx::IRenderer& r,
                   const std::unordered_map<std::string, int>& items,
                   float screenW, float screenH);

} // namespace nccu

#endif // INVENTORY_VIEW_H_
