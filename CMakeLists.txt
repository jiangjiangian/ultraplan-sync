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

# Ship the game binary as `umbrella` (per the assignment README naming) while
# keeping the CMake target name (${PROJECT_NAME}) and the _tests/_lib targets
# unchanged — so `./build/OOP_Raylib_Lab_tests` (the test gate) is unaffected
# and only the runnable game becomes `./build/umbrella`.
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME umbrella)
# Keep ./build/OOP_Raylib_Lab resolving (the harness playtest.sh + docs launch
# the game by that path) as a symlink to the umbrella binary, so the rename
# ADDS the assignment name without breaking the existing tooling.
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
            "$<TARGET_FILE_NAME:${PROJECT_NAME}>"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/OOP_Raylib_Lab"
    VERBATIM)

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
