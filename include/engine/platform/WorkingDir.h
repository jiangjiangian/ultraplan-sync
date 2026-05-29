#ifndef ENGINE_PLATFORM_WORKING_DIR_H_
#define ENGINE_PLATFORM_WORKING_DIR_H_
#include "raylib.h"

// Working-directory normalisation for the game's relative asset paths.
// raylib.h is confined to the engine layer (CONVENTIONS R5), so the chdir
// lives here behind a typed wrapper and main() (app layer) just calls it.
namespace nccu::engine::platform {

// Ensure the process working directory is one from which the game's
// relative asset paths (resources/..., docs/content/...) resolve.
//
// Every texture/font/map/dialog file is loaded with a path relative to the
// CWD. Run the documented way (./build/OOP_Raylib_Lab from the project
// root) that is fine, but a Finder / IDE / double-click launch — or running
// the binary from inside build/ — leaves the CWD somewhere without those
// folders. On macOS raylib also disables GLFW's chdir-to-bundle, so nothing
// fixes it for us, and the missing docs/content used to send the font
// loader down its oversized-atlas fallback (a startup crash on some GPUs).
//
// CMake copies BOTH resources/ and docs/content/ next to the executable, so
// when the CWD lacks them we switch to the executable's own directory where
// they are guaranteed to be. Idempotent and conservative: when the assets
// already resolve from the CWD (project-root run, ctest, harness-from-root)
// the CWD is left untouched, so normal play / the harness stay unaffected.
// Call once at startup, before the first asset load (EnsureFont).
inline void EnsureAssetWorkingDir() {
    if (::DirectoryExists("resources") && ::DirectoryExists("docs/content"))
        return;                                // already runnable from here
    const char* appDir = ::GetApplicationDirectory();
    if (appDir != nullptr && appDir[0] != '\0')
        ::ChangeDirectory(appDir);             // .../build, where CMake put both
}

} // namespace nccu::engine::platform

#endif // ENGINE_PLATFORM_WORKING_DIR_H_
