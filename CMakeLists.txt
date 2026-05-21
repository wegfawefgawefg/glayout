cmake_minimum_required(VERSION 3.20)

project(glayout VERSION 0.1.0 LANGUAGES CXX)

option(GLAYOUT_STRICT "Enable strict compiler warnings" ON)
option(GLAYOUT_WARN_AS_ERROR "Treat warnings as errors" OFF)
option(GLAYOUT_BUILD_EXAMPLES "Build example programs" ON)
option(GLAYOUT_BUILD_TESTS "Build test programs" ON)
option(GLAYOUT_BUILD_SDL_DEMO "Build SDL3 demo program when SDL3 is available" ON)
option(GLAYOUT_WITH_IMGUI "Build optional Dear ImGui helpers" OFF)
set(GLAYOUT_GSEXP_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../gsexp"
    CACHE PATH "Path to a gsexp source tree when gsexp::gsexp is not already available")

if(NOT TARGET gsexp::gsexp)
    if(EXISTS "${GLAYOUT_GSEXP_SOURCE_DIR}/CMakeLists.txt")
        set(GSEXP_BUILD_EXAMPLES OFF CACHE BOOL "Build gsexp examples" FORCE)
        add_subdirectory("${GLAYOUT_GSEXP_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/_deps/gsexp")
    else()
        message(FATAL_ERROR
            "gsexp::gsexp target was not found and GLAYOUT_GSEXP_SOURCE_DIR does not point "
            "to a gsexp source tree: ${GLAYOUT_GSEXP_SOURCE_DIR}")
    endif()
endif()

add_library(glayout_core
    src/editor.cpp
    src/layout.cpp
)

target_include_directories(glayout_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(glayout_core PUBLIC cxx_std_20)
target_link_libraries(glayout_core PUBLIC gsexp::gsexp)

function(glayout_apply_warnings target_name)
    if(NOT GLAYOUT_STRICT)
        return()
    endif()

    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4)
        if(GLAYOUT_WARN_AS_ERROR)
            target_compile_options(${target_name} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target_name} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wshadow
        )
        if(GLAYOUT_WARN_AS_ERROR)
            target_compile_options(${target_name} PRIVATE -Werror)
        endif()
    endif()
endfunction()

glayout_apply_warnings(glayout_core)

if(GLAYOUT_BUILD_EXAMPLES)
    add_executable(glayout_demo examples/glayout_demo.cpp)
    target_link_libraries(glayout_demo PRIVATE glayout_core)
    target_compile_features(glayout_demo PRIVATE cxx_std_20)
    glayout_apply_warnings(glayout_demo)
endif()

if(GLAYOUT_BUILD_SDL_DEMO)
    find_package(SDL3 CONFIG QUIET)
    if(TARGET SDL3::SDL3)
        set(GLAYOUT_SDL3_TARGET SDL3::SDL3)
    else()
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(SDL3_PC QUIET IMPORTED_TARGET sdl3)
            if(TARGET PkgConfig::SDL3_PC)
                set(GLAYOUT_SDL3_TARGET PkgConfig::SDL3_PC)
            endif()
        endif()
    endif()

    if(GLAYOUT_SDL3_TARGET)
        add_executable(glayout_sdl_demo examples/sdl_demo/main.cpp)
        target_link_libraries(glayout_sdl_demo PRIVATE glayout_core ${GLAYOUT_SDL3_TARGET})
        target_compile_features(glayout_sdl_demo PRIVATE cxx_std_20)
        target_compile_definitions(glayout_sdl_demo PRIVATE
            GLAYOUT_SDL_DEMO_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/examples/sdl_demo/data")
        glayout_apply_warnings(glayout_sdl_demo)
    else()
        message(STATUS "SDL3 not found; skipping glayout_sdl_demo")
    endif()
endif()

if(GLAYOUT_BUILD_TESTS)
    enable_testing()

    add_executable(glayout_core_tests tests/core_tests.cpp)
    target_link_libraries(glayout_core_tests PRIVATE glayout_core)
    target_compile_features(glayout_core_tests PRIVATE cxx_std_20)
    glayout_apply_warnings(glayout_core_tests)

    add_test(NAME glayout_core_tests COMMAND glayout_core_tests)
endif()
