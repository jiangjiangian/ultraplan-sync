# src/gfx — empty by design

The gfx headers are header-only RAII wrappers (`Camera2D`, `Texture`,
`DrawScope`, `CameraScope`, `TextBuilder`, `Vec2`, `Rect`, `Bounds`,
`Color`, `Key`, `Time`). Any helper that needed implementation went
into the raylib-backed renderer or into world/entities directly, so
there are no `.cpp` files in this directory.

The directory exists to keep the layout symmetric with `include/gfx/`
and `tests/gfx/`. Adding a `.cpp` here is fine — `CMakeLists.txt` uses
`GLOB_RECURSE` and will pick it up at next configure.
