cmake_minimum_required(VERSION 3.14)

## Configure Compiler
set(CMAKE_CXX_STANDARD 17
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(BUILD_SHARED_LIBS FALSE)
set(CMAKE_POSITION_INDEPENDENT_CODE OFF)

## CMake Extensions
include(./cmake/CMakeRC.cmake)
include(./cmake/CPM.cmake)

## Create project
project(Eksamen LANGUAGES CXX)

## Dependencies
CPMAddPackage(
  NAME glfw
  GITHUB_REPOSITORY glfw/glfw
  GIT_TAG 3.3.3
  OPTIONS "GLFW_BUILD_DOCS OFF" "GLFW_BUILD_TESTS OFF" "GLFW_BUILD_EXAMPLES OFF"
)
CPMAddPackage(
  NAME bgfx
  GITHUB_REPOSITORY widberg/bgfx.cmake
  GIT_TAG 66f18daf5c380b34f4745ac0922b8a9853b51ad9
  OPTIONS "BGFX_LIBRARY_TYPE STATIC"
)
CPMAddPackage(
  NAME imgui_src
  GITHUB_REPOSITORY ocornut/imgui
  VERSION 1.82
)
CPMAddPackage(
  NAME fmt
  GITHUB_REPOSITORY fmtlib/fmt
  GIT_TAG 7.1.3
)
CPMAddPackage(
  NAME cxxopts
  GITHUB_REPOSITORY jarro2783/cxxopts
  VERSION 2.2.1
)

## Libraries
add_library(imgui
    "${bgfx_SOURCE_DIR}/bgfx/examples/common/imgui/imgui.cpp"
    "${imgui_src_SOURCE_DIR}/backends/imgui_impl_glfw.cpp")
target_include_directories(imgui PUBLIC
    "${bgfx_SOURCE_DIR}/bgfx/examples/common/imgui"
    "${imgui_src_SOURCE_DIR}/backends")
target_link_libraries(imgui dear-imgui bgfx glfw) # We link the main part of imgui source code from bgfx

add_library(nanovg
    "${bgfx_SOURCE_DIR}/bgfx/examples/common/nanovg/nanovg.cpp"
    "${bgfx_SOURCE_DIR}/bgfx/examples/common/nanovg/nanovg_bgfx.cpp")
target_include_directories(nanovg PUBLIC
    "${bgfx_SOURCE_DIR}/bgfx/examples/common/nanovg"
    "${bgfx_SOURCE_DIR}/bgfx/3rdparty")
target_link_libraries(nanovg bgfx)

## Assets
set(ASSET_DIR "${CMAKE_SOURCE_DIR}/assets")
cmrc_add_resource_library(fonts WHENCE "${ASSET_DIR}/fonts" PREFIX fonts
    "${ASSET_DIR}/fonts/Roboto-Regular.ttf"
    "${ASSET_DIR}/fonts/FreeSerif.ttf"
    "${ASSET_DIR}/fonts/NotoEmoji-Regular.ttf")

## Targets
add_executable(demo "src/main.cpp"
    "src/App.cpp" "include/App.h"
    "src/Util.cpp" "include/Util.h")
target_link_libraries(demo glfw bgfx imgui nanovg fmt cxxopts fonts)
target_precompile_headers(demo PUBLIC
    "<fmt/core.h>" "<cxxopts.hpp>" "<bgfx/bgfx.h>" "<GLFW/glfw3.h>"
     "<bx/bx.h>" "<bgfx/platform.h>" "<bx/math.h>" "<bx/math.h>"
     "<imgui.h>" "<imgui_impl_glfw.h>" "<nanovg.h>") # Optimize compile times with precompiled headers

## Bundle executable
if(APPLE)
    set_target_properties(demo PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${ASSET_DIR}/Info.plist
    )

    install(TARGETS demo
        BUNDLE DESTINATION . COMPONENT Runtime
        RUNTIME DESTINATION bin COMPONENT Runtime
    )

    # Note Mac specific extension .app
    set(APPS "\${CMAKE_INSTALL_PREFIX}/Demo.app")

    # Directories to look for dependencies
    set(DIRS ${CMAKE_BINARY_DIR})

    install(CODE "include(BundleUtilities)
        fixup_bundle(\"${APPS}\" \"\" \"${DIRS}\")")

    set(CPACK_GENERATOR "DRAGNDROP")
    include(CPack)
endif()


