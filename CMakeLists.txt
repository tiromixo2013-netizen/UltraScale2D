cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(UltraScale2D VERSION 0.2.0)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Define GEODE_SDK to point to the Geode SDK.")
endif()

message(STATUS "Found Geode SDK: $ENV{GEODE_SDK}")
add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)
setup_geode_mod(${PROJECT_NAME})

if (ANDROID)
    # Geode/Cocos' Android precompiled header defines legacy GLES2 VAO macros.
    # Those macros collide with GLES 3.1 declarations in gpu_caps.hpp, even
    # when that header appears first in main.cpp, because PCH is force-included.
    # Disable PCH for this target so source include order is respected.
    set_property(TARGET ${PROJECT_NAME} PROPERTY DISABLE_PRECOMPILE_HEADERS ON)

    # OpenGL ES 3.1 / EGL are used only for safe runtime capability detection.
    target_link_libraries(${PROJECT_NAME} GLESv3 EGL)
endif()
