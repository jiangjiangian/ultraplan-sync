cmake_minimum_required(VERSION 3.14)
project(OOP_Raylib_Lab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
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

add_executable(${PROJECT_NAME} "${SRC_DIR}/main.cpp")
target_include_directories(${PROJECT_NAME} PRIVATE "${INCLUDE_DIR}")

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

file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/tests/*.cpp")
if(TEST_SOURCES AND TARGET ${PROJECT_NAME}_lib)
    add_executable(${PROJECT_NAME}_tests ${TEST_SOURCES})
    target_link_libraries(${PROJECT_NAME}_tests PRIVATE ${PROJECT_NAME}_lib doctest::doctest)
    target_compile_definitions(${PROJECT_NAME}_tests PRIVATE
        TEST_FIXTURES_DIR="${CMAKE_SOURCE_DIR}/tests/fixtures"
        TEST_CONTENT_DIR="${CMAKE_SOURCE_DIR}/docs/content")
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
