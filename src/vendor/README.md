# src/vendor — vendor actor and loader

Implements the vendor NPCs. `VendorMessages.h` /
`VendorConfig.h` / `VendorSprite.h` are header-only constexpr tables;
they have no `.cpp` here.

## Files

- `Vendor.cpp` — Vendor actor; runs the buy/decline dialog
- `VendorLoader.cpp` — load vendor markdown into `VendorConfig`

## Dependency direction

- Depends on: controller, entities, gfx
- Used by: controller, quest, world
