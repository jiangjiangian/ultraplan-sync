cmake_minimum_required(VERSION 3.14)
project(OOP_Raylib_Lab LANGUAGES CXX)

# If compiler not found, uncomment and set your compiler path 
# set(CMAKE_CXX_COMPILER "directory to g++") 
# set(CMAKE_C_COMPILER "directory to gcc")
#
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")

include(FetchContent)

file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
    "${SRC_DIR}/*.cpp"
    "${INCLUDE_DIR}/*.h"
)

list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")

if(NOT GAME_SOURCES)
    message(FATAL_ERROR "No source files were found in ${SRC_DIR}")
endif()

find_package(raylib 5 QUIET)

if(NOT raylib_FOUND)
    message(STATUS "raylib not found locally. Fetching raylib from GitHub...")
    set(FETCHCONTENT_QUIET OFF)

    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5
        GIT_SHALLOW TRUE
    )

    FetchContent_MakeAvailable(raylib)
endif()

add_library(${PROJECT_NAME}_lib ${GAME_SOURCES})
target_include_directories(${PROJECT_NAME}_lib PUBLIC "${INCLUDE_DIR}")
target_link_libraries(${PROJECT_NAME}_lib PUBLIC raylib)

add_executable(${PROJECT_NAME} "${SRC_DIR}/main.cpp")

target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_lib)

if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework Cocoa")
endif()

function(copy_resources_to_target TARGET_NAME)
    if(EXISTS "${CMAKE_SOURCE_DIR}/resources")
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${CMAKE_SOURCE_DIR}/resources"
                    "$<TARGET_FILE_DIR:${TARGET_NAME}>"
            COMMENT "Copying assets to output directory"
        )
    endif()
endfunction()

if(EXISTS "${CMAKE_SOURCE_DIR}/resources")
    copy_resources_to_target(${PROJECT_NAME})
endif()
