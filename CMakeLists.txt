cmake_minimum_required(VERSION 3.14)
project(OOP_Raylib_Lab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")

include(FetchContent)

# doctest v2.4.11 declares cmake_minimum_required(VERSION 3.0); CMake >= 4.0
# dropped policy-<3.5 compatibility and aborts configure on it. Bake CMake's
# documented escape hatch in so a bare `cmake -B build` works without the
# caller passing -DCMAKE_POLICY_VERSION_MINIMUM=3.5 (a user -D still wins).
if(NOT DEFINED CMAKE_POLICY_VERSION_MINIMUM)
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
endif()

file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
    "${SRC_DIR}/*.cpp"
    "${INCLUDE_DIR}/*.h"
)
list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")

find_package(raylib 5 QUIET)
if(NOT raylib_FOUND)
    message(STATUS "raylib not found locally. Fetching raylib 5.5 from GitHub...")
    set(FETCHCONTENT_QUIET OFF)
    FetchContent_Declare(raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(raylib)
endif()

add_executable(${PROJECT_NAME} "${SRC_DIR}/app/main.cpp")
# Two include roots:
#   ${INCLUDE_DIR}        — engine + ui + game/* in their full layered form
#                           (every file under include/, with the file's actual
#                           on-disk path matching its #include string)
#   ${INCLUDE_DIR}/game   — the composition root's short-form shortcut, so
#                           main.cpp reads "world/World.h" instead of
#                           "game/world/World.h". The game/ leaf is the
#                           app's "domain root"; main.cpp gets the short form
#                           since the WHOLE program is built from there.
target_include_directories(${PROJECT_NAME} PRIVATE
    "${INCLUDE_DIR}"
    "${INCLUDE_DIR}/game")

# 可執行的遊戲輸出為 ./build/umbrella、測試二進位為 ./build/umbrella_test（皆以 OUTPUT_NAME
# 指定，與 README 的指令一致）；CMake 內部目標名仍為 OOP_Raylib_Lab / OOP_Raylib_Lab_tests。
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME umbrella)

# Add a static lib for the gameplay code IF any non-main .cpp files exist.
# Early in development the lib may be empty; we add it lazily so configure succeeds.
# When the lib exists, raylib is linked to it (PUBLIC) and the executable
# inherits raylib transitively — never link raylib twice, otherwise the
# linker emits a duplicate-libraries warning.
if(GAME_SOURCES)
    add_library(${PROJECT_NAME}_lib ${GAME_SOURCES})
    target_include_directories(${PROJECT_NAME}_lib PUBLIC "${INCLUDE_DIR}")
    target_link_libraries(${PROJECT_NAME}_lib PUBLIC raylib)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_lib)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE raylib)
endif()

if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework Cocoa")
endif()

# doctest for unit tests on pure-data logic
FetchContent_Declare(doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.11
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(doctest)

enable_testing()

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/tests/*.cpp")
if(TEST_SOURCES AND TARGET ${PROJECT_NAME}_lib)
    add_executable(${PROJECT_NAME}_tests ${TEST_SOURCES})
    set_target_properties(${PROJECT_NAME}_tests PROPERTIES OUTPUT_NAME umbrella_test)
    target_link_libraries(${PROJECT_NAME}_tests PRIVATE ${PROJECT_NAME}_lib doctest::doctest)
    target_compile_definitions(${PROJECT_NAME}_tests PRIVATE
        TEST_FIXTURES_DIR="${CMAKE_SOURCE_DIR}/tests/fixtures"
        TEST_CONTENT_DIR="${CMAKE_SOURCE_DIR}/docs/content"
        # UI-B-1: the all-literal glyph-coverage gate scans every "…" string
        # literal under src/ + include/ at TEST time (the files are not shipped
        # at runtime), so it needs the project source root, the same way the
        # dialog tests get the content dir. CTest runs with cwd = build dir.
        TEST_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
    add_test(NAME unit_tests COMMAND ${PROJECT_NAME}_tests)
endif()

# Copy resources next to the executable
function(copy_resources_to_target TARGET_NAME)
    if(EXISTS "${CMAKE_SOURCE_DIR}/resources")
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${CMAKE_SOURCE_DIR}/resources"
                    "$<TARGET_FILE_DIR:${TARGET_NAME}>"
            COMMENT "Copying assets to output directory")
    endif()
endfunction()
if(EXISTS "${CMAKE_SOURCE_DIR}/resources")
    copy_resources_to_target(${PROJECT_NAME})
endif()

# Copy docs/content (the runtime-loaded dialog Markdown) next to the
# executable too, so the game finds it regardless of launch directory
# (Finder / IDE / run-from-build). The Font + DialogLoader probes are
# CWD-relative, and EnsureAssetWorkingDir() chdirs the process here when
# launched from elsewhere — both then resolve docs/content/*.md, so the
# safe content-present font-atlas path is taken instead of the oversized
# fallback. Mirrors copy_resources_to_target.
function(copy_content_to_target TARGET_NAME)
    if(EXISTS "${CMAKE_SOURCE_DIR}/docs/content")
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${CMAKE_SOURCE_DIR}/docs/content"
                    "$<TARGET_FILE_DIR:${TARGET_NAME}>/docs/content"
            COMMENT "Copying dialog content to output directory")
    endif()
endfunction()
if(EXISTS "${CMAKE_SOURCE_DIR}/docs/content")
    copy_content_to_target(${PROJECT_NAME})
endif()

# Blueprint Phase 4 §A — Doxygen architecture-audit target. Generates
# the class hierarchy + per-directory dependency graphs into
# build/doxygen/html/ (gitignored). Optional build-dep (graphviz +
# doxygen); the target is added unconditionally but the build only
# succeeds when both are installed. Run:
#     cmake --build build --target docs
# Then open build/doxygen/html/hierarchy.html or any dir_*.html.
find_program(DOXYGEN_EXECUTABLE doxygen)
if(DOXYGEN_EXECUTABLE)
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} "${CMAKE_SOURCE_DIR}/Doxyfile"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Generating Doxygen architecture audit (build/doxygen/html/)"
        VERBATIM)
endif()
