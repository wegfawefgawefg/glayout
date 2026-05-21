cmake_minimum_required(VERSION 3.20)

project(glayout VERSION 0.1.0 LANGUAGES CXX)

option(GLAYOUT_STRICT "Enable strict compiler warnings" ON)
option(GLAYOUT_WARN_AS_ERROR "Treat warnings as errors" OFF)
option(GLAYOUT_BUILD_EXAMPLES "Build example programs" ON)
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
    src/layout.cpp
)

target_include_directories(glayout_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(glayout_core PUBLIC cxx_std_20)
target_link_libraries(glayout_core PUBLIC gsexp::gsexp)

if(GLAYOUT_STRICT)
    if(MSVC)
        target_compile_options(glayout_core PRIVATE /W4)
        if(GLAYOUT_WARN_AS_ERROR)
            target_compile_options(glayout_core PRIVATE /WX)
        endif()
    else()
        target_compile_options(glayout_core PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wshadow
        )
        if(GLAYOUT_WARN_AS_ERROR)
            target_compile_options(glayout_core PRIVATE -Werror)
        endif()
    endif()
endif()

if(GLAYOUT_BUILD_EXAMPLES)
    add_executable(glayout_demo examples/glayout_demo.cpp)
    target_link_libraries(glayout_demo PRIVATE glayout_core)
    target_compile_features(glayout_demo PRIVATE cxx_std_20)

    if(GLAYOUT_STRICT)
        if(MSVC)
            target_compile_options(glayout_demo PRIVATE /W4)
            if(GLAYOUT_WARN_AS_ERROR)
                target_compile_options(glayout_demo PRIVATE /WX)
            endif()
        else()
            target_compile_options(glayout_demo PRIVATE
                -Wall
                -Wextra
                -Wpedantic
                -Wconversion
                -Wshadow
            )
            if(GLAYOUT_WARN_AS_ERROR)
                target_compile_options(glayout_demo PRIVATE -Werror)
            endif()
        endif()
    endif()
endif()
