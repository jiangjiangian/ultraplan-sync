#include "world/WorldOptions.h"

#include <cstdlib>
#include <cstring>

namespace nccu {

WorldOptions ReadWorldOptionsFromEnv() {
    WorldOptions opts;
    if (const char* env = std::getenv("UMBRELLA_REDUCED_MOTION");
        env != nullptr && std::strcmp(env, "1") == 0) {
        opts.reducedMotion = true;
    }
    if (const char* env = std::getenv("UMBRELLA_LARGE_TARGETS");
        env != nullptr && std::strcmp(env, "1") == 0) {
        opts.largeTargets = true;
    }
    return opts;
}

} // namespace nccu
