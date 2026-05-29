#include "game/world/WorldOptions.h"

#include <cstdlib>
#include <cstring>

/**
 * @file WorldOptions.cpp
 * @brief 無障礙選項的環境變數解析實作（嚴格比對 "1"，其餘維持預設關閉）。
 */

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
