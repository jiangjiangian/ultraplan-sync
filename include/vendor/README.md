# vendor — market vendors (shop NPCs)

Special NPCs that sell items. Decoupled from generic NPC because they
own inventory, prices, decline dialogue, and a centred-cluster
positioning rule for the market scene.

## Files

- `Vendor.h` — vendor actor; owns inventory + price list
- `VendorConfig.h` — per-vendor config struct
- `VendorLoader.h` — load `docs/content/vendors_*.md` into VendorConfig
- `VendorMessages.h` — greeting / decline / sold-out strings
- `VendorSprite.h` — sprite-selector lookup table

## Dependency direction

- Depends on: entities, gfx, quest (headers); plus controller in
  src/vendor/
- Used by: controller, quest, world
