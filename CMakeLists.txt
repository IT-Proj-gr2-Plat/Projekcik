cmake_minimum_required(VERSION 3.16)
project(Projekcik LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_image CONFIG REQUIRED)
find_package(SDL2_ttf CONFIG QUIET)        # prefer CONFIG but allow missing
find_package(ZLIB QUIET)
find_package(unofficial-minizip CONFIG QUIET)
find_package(SDL2_ttf CONFIG REQUIRED)

add_executable(SdlExample
        src/main.cpp
        src/Texture.cpp
        src/Player.cpp
        src/Level.cpp
        src/LevelEditor.cpp
        src/ZipUtil.cpp
        src/Menu.cpp
        include/Menu.h
)

target_include_directories(SdlExample PRIVATE include)

file(COPY "${CMAKE_SOURCE_DIR}/assets" DESTINATION "${CMAKE_BINARY_DIR}")

if(TARGET SDL2::SDL2_ttf)
    target_link_libraries(SdlExample PRIVATE SDL2::SDL2_ttf)
elseif(TARGET SDL2_ttf::SDL2_ttf)
    target_link_libraries(SdlExample PRIVATE SDL2_ttf::SDL2_ttf)
elseif(TARGET SDL2_ttf)
    target_link_libraries(SdlExample PRIVATE SDL2_ttf)
else()
    set(SDL2_TTF_INCLUDE_DIR "" CACHE PATH "Path to SDL2_ttf include directory (contains SDL_ttf.h)")
    set(SDL2_TTF_LIBRARY "" CACHE FILEPATH "Path to SDL2_ttf library file for your toolchain (.a, .dll.a, or .lib if appropriate)")

    if(SDL2_TTF_INCLUDE_DIR AND SDL2_TTF_LIBRARY)
        target_include_directories(SdlExample PRIVATE ${SDL2_TTF_INCLUDE_DIR})
        target_link_libraries(SdlExample PRIVATE ${SDL2_TTF_LIBRARY})
    else()
        message(WARNING "SDL2_ttf not found as imported target. Either:\n  * Install SDL2_ttf for your toolchain (e.g. MSYS2: `pacman -S mingw-w64-x86_64-SDL2_ttf`) and run CMake from the MinGW shell or set CLion toolchain to MinGW,\n  * Or set CMake cache variables `SDL2_TTF_INCLUDE_DIR` and `SDL2_TTF_LIBRARY` to the correct paths for your toolchain.")
    endif()
endif()
# Link SDL2 core
if(TARGET SDL2::SDL2)
    target_link_libraries(SdlExample PRIVATE SDL2::SDL2)
else()
    message(FATAL_ERROR "SDL2 CMake target not found.")
endif()

# SDL2_image: try several common imported target names or legacy variables
if(TARGET SDL2::SDL2_image)
    set(_SDL2_IMAGE_TARGET SDL2::SDL2_image)
elseif(TARGET SDL2_image::SDL2_image)
    set(_SDL2_IMAGE_TARGET SDL2_image::SDL2_image)
elseif(TARGET SDL2_image)
    set(_SDL2_IMAGE_TARGET SDL2_image)
endif()

if(DEFINED _SDL2_IMAGE_TARGET)
    target_link_libraries(SdlExample PRIVATE ${_SDL2_IMAGE_TARGET})
elseif(DEFINED SDL2_IMAGE_LIBRARIES)
    target_include_directories(SdlExample PRIVATE ${SDL2_IMAGE_INCLUDE_DIRS})
    target_link_libraries(SdlExample PRIVATE ${SDL2_IMAGE_LIBRARIES})
else()
    message(FATAL_ERROR "SDL2_image not found. Ensure `find_package(SDL2_image CONFIG)` succeeded.")
endif()

# SDL2_ttf: link if available; else continue but warn (Menu will handle missing font)
if(TARGET SDL2::SDL2_ttf)
    set(_SDL2_TTF_TARGET SDL2::SDL2_ttf)
elseif(TARGET SDL2_ttf::SDL2_ttf)
    set(_SDL2_TTF_TARGET SDL2_ttf::SDL2_ttf)
elseif(TARGET SDL2_ttf)
    set(_SDL2_TTF_TARGET SDL2_ttf)
endif()

if(DEFINED _SDL2_TTF_TARGET)
    target_link_libraries(SdlExample PRIVATE ${_SDL2_TTF_TARGET})
else()
    if(TARGET SDL2_ttf)
        target_link_libraries(SdlExample PRIVATE SDL2_ttf)
    else()
        message(WARNING "SDL2_ttf not found via CONFIG. Menu text rendering will be disabled unless TTF is available at runtime.")
    endif()
endif()

# SDL2main: prefer linking to the platform-provided startup glue; otherwise define SDL_MAIN_HANDLED
if(TARGET SDL2::SDL2main)
    set(_SDL2_MAIN_TARGET SDL2::SDL2main)
elseif(TARGET SDL2main::SDL2main)
    set(_SDL2_MAIN_TARGET SDL2main::SDL2main)
elseif(TARGET SDL2main)
    set(_SDL2_MAIN_TARGET SDL2main)
endif()

if(DEFINED _SDL2_MAIN_TARGET)
    target_link_libraries(SdlExample PRIVATE ${_SDL2_MAIN_TARGET})
else()
    # Define SDL_MAIN_HANDLED so user's main is used directly if SDL2main is missing
    target_compile_definitions(SdlExample PRIVATE SDL_MAIN_HANDLED)
    message(WARNING "SDL2main target not found. Defining SDL_MAIN_HANDLED to use application main().")
endif()

# minizip (unofficial-minizip) and zlib linking with fallbacks (vcpkg awareness)
if(NOT DEFINED VCPKG_TARGET_TRIPLET)
    if(MSVC)
        set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "vcpkg triplet")
    else()
        set(VCPKG_TARGET_TRIPLET "x64-mingw" CACHE STRING "vcpkg triplet")
    endif()
endif()

# Allow explicit manual override of vcpkg root path
if(NOT DEFINED _vcpkg_root)
    if(DEFINED ENV{VCPKG_ROOT})
        set(_vcpkg_root $ENV{VCPKG_ROOT})
    else()
        set(_vcpkg_root "C:/tools/vcpkg" CACHE PATH "Path to vcpkg root directory")
    endif()
endif()

# Prefer imported target if present
if(TARGET minizip::minizip)
    target_link_libraries(SdlExample PRIVATE minizip::minizip)
elseif(TARGET minizip)
    target_link_libraries(SdlExample PRIVATE minizip)
elseif(TARGET minizip-ng::minizip-ng)
    target_link_libraries(SdlExample PRIVATE minizip-ng::minizip-ng)
elseif(TARGET minizip-ng)
    target_link_libraries(SdlExample PRIVATE minizip-ng)
elseif(TARGET unofficial::minizip)
    target_link_libraries(SdlExample PRIVATE unofficial::minizip)
elseif(TARGET unofficial-minizip::unofficial-minizip)
    target_link_libraries(SdlExample PRIVATE unofficial-minizip::unofficial-minizip)
elseif(TARGET unofficial-minizip)
    target_link_libraries(SdlExample PRIVATE unofficial-minizip)
else()
    # fallback: look in vcpkg installed layout if _vcpkg_root/VCPKG_TARGET_TRIPLET are set
    if(DEFINED _vcpkg_root AND _vcpkg_root)
        set(_vcpkg_inc "${_vcpkg_root}/installed/${VCPKG_TARGET_TRIPLET}/include")
        set(_vcpkg_lib "${_vcpkg_root}/installed/${VCPKG_TARGET_TRIPLET}/lib")
        find_path(MINIZIP_INCLUDE_DIR zip.h HINTS "${_vcpkg_inc}" PATH_SUFFIXES minizip)
        find_library(MINIZIP_LIBRARY NAMES minizip minizip-static minizip64 unofficial-minizip HINTS "${_vcpkg_lib}")
        find_library(ZLIB_LIBRARY NAMES zlib zlibstatic HINTS "${_vcpkg_lib}")
        if(MINIZIP_INCLUDE_DIR AND MINIZIP_LIBRARY)
            target_include_directories(SdlExample PRIVATE ${MINIZIP_INCLUDE_DIR})
            if(ZLIB_LIBRARY)
                target_link_libraries(SdlExample PRIVATE ${MINIZIP_LIBRARY} ${ZLIB_LIBRARY})
            else()
                target_link_libraries(SdlExample PRIVATE ${MINIZIP_LIBRARY})
            endif()
        else()
            message(WARNING "minizip not found under vcpkg root ${_vcpkg_root}. Run:\n  cd ${_vcpkg_root} && ./vcpkg install minizip:${VCPKG_TARGET_TRIPLET}\nOr set MINIZIP_INCLUDE_DIR and MINIZIP_LIBRARY manually.")
        endif()
    else()
        message(WARNING "minizip not found and vcpkg root unknown. Set CMake cache variable VCPKG_ROOT or install minizip into the vcpkg instance CMake is using.")
    endif()
endif()

# zlib
if(TARGET ZLIB::ZLIB)
    target_link_libraries(SdlExample PRIVATE ZLIB::ZLIB)
elseif(DEFINED ZLIB_LIBRARIES)
    target_link_libraries(SdlExample PRIVATE ${ZLIB_LIBRARIES})
endif()

# Recommended: enable warnings
if(MSVC)
    target_compile_options(SdlExample PRIVATE /W4 /permissive-)
else()
    target_compile_options(SdlExample PRIVATE -Wall -Wextra -Wpedantic)
endif()
