# gfx — engine-facing graphics + input thin wrappers

Thin RAII / value-type wrappers over raylib and assets. Everything else
in the codebase depends on these abstractions instead of calling raylib
directly, so swapping the backend (e.g. for headless tests) stays cheap.

## Files

- `Bounds.h` — axis-aligned box used by camera clamping
- `Camera2D.h` — Camera2D value type (matches raylib but POD)
- `CameraScope.h` — RAII Begin/EndMode2D guard
- `Color.h` — RGBA color value
- `DrawScope.h` — RAII Begin/EndDrawing guard
- `Font.h` — CJK font atlas helpers
- `IRenderer.h` — render interface (decouples View from raylib)
- `Input.h` — keyboard input abstraction (real or scripted)
- `Key.h` — Key enum mirroring raylib keys
- `MaskLoader.h` — load greyscale collision masks
- `RaylibRenderer.h` — IRenderer implementation backed by raylib
- `Rect.h` — float rectangle helper
- `Renderer.h` — legacy renderer entry point
- `TextBuilder.h` — fluent layout for text draws
- `Texture.h` — RAII texture handle (no-ops on missing file)
- `Time.h` — fixed-step delta-time helper
- `Vec2.h` — 2D float vector
- `Window.h` — RAII window lifetime

## Dependency direction

- Depends on: world (only `MaskLoader` knows tile coords)
- Used by: every other bucket (this is the lowest layer)
