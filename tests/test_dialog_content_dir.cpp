#include "DialogSource.h"
#ifndef TEST_CONTENT_DIR
#error "TEST_CONTENT_DIR must be defined by the build system"
#endif

// Test-support translation unit (no TEST_CASE). The behaviour tests that
// open dialog now reach the filesystem through DialogSource, which
// resolves the chapter markdown relative to its content dir (default
// "docs/content" relative to cwd). CTest runs the test binary with cwd =
// the build dir, so the default would not find the file and those tests
// would silently get empty dialog. This global static initializer runs
// before doctest's main() (all TUs' static init completes first), so
// every test that opens dialog resolves the real content dir.
namespace {
struct DialogContentDirInit {
    DialogContentDirInit() { nccu::dialog::SetContentDir(TEST_CONTENT_DIR); }
};
const DialogContentDirInit kDialogContentDirInit;
}  // namespace
