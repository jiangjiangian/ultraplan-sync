# tests/vendor — vendor actor + loader

## Files

- `test_vendor.cpp` — core vendor behaviour
- `test_vendor_loader.cpp` — markdown loader
- `test_vendor_inventory.cpp` — inventory mutation on purchase
- `test_vendor_decline.cpp` — decline / sold-out path
- `test_vendor_centred_cluster.cpp` — market layout rule

## Dependency direction

- Depends on: controller, dialog, entities, gfx, quest, state, world
- Used by: nothing (test leaf)
