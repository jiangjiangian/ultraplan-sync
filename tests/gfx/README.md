# tests/gfx — pure-value gfx helper tests

Header-only POD wrappers, so these are fast no-GL unit tests.

## Files

- `test_vec2.cpp` — 2D float vector ops
- `test_rect.cpp` — float rectangle helpers
- `test_bounds.cpp` — AABB containment / clamping
- `test_color.cpp` — RGBA color round-trip
- `test_camera2d.cpp` — Camera2D math
- `test_camera2d_clamp.cpp` — Camera2D bounds clamping
- `test_text_builder.cpp` — fluent text layout builder

## Dependency direction

- Depends on: only the headers in `include/gfx/`
- Used by: nothing (test leaf)
