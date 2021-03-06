# LT_API build script.
#

cmake_minimum_required(VERSION 3.9)
project(LT_API VERSION 1.0.0 DESCRIPTION "LiveTraffic API Example Plugin")

# By default, use optimized release configuration.
if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif ()

set(CMAKE_CXX_STANDARD 11)

# Set include directories used by our code and dependencies.
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/..")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
include_directories("${CMAKE_SOURCE_DIR}/Lib/SDK/CHeaders/Widgets")
include_directories("${CMAKE_SOURCE_DIR}/Lib/SDK/CHeaders/Wrappers")
include_directories("${CMAKE_SOURCE_DIR}/Lib/SDK/CHeaders/XPLM")

# Specify library search locations.
if (WIN32)
    list(APPEND CMAKE_LIBRARY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Lib/SDK/Libraries/Win")
elseif (APPLE)
    list(APPEND CMAKE_FRAMEWORK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Lib/SDK/Libraries/Mac")
elseif (UNIX)

endif ()

# message (STATUS "CMAKE_LIBRARY_PATH" = ${CMAKE_LIBRARY_PATH})

# # Enable all X-Plane SDK APIs up to the newest version.
add_definitions(-DXPLM200=1 -DXPLM210=1 -DXPLM300=1 -DXPLM301=1)
# Enable X-Plane 10 SDK APIs only:
# add_definitions(-DXPLM200=1 -DXPLM210=1)
# Define platform macros.
add_definitions(-DAPL=$<BOOL:${APPLE}> -DIBM=$<BOOL:${WIN32}> -DLIN=$<AND:$<BOOL:${UNIX}>,$<NOT:$<BOOL:${APPLE}>>>)

# Force-enable exception support. This is most likely redundant, although for C
# code the default is the opposite. Since we are mixing C++ and C libraries,
# safer to set it on?
add_compile_options(-fexceptions -fpermissive)
# On UNIX systems this makes symbols non-exported by default. On Windows this
# option is simply ignored, since symbol visibility works differently there.
add_compile_options(-fvisibility=hidden)

# Enable stricter warnings and then disable some we are not interested in.
add_compile_options(-Wall -Wshadow -Wfloat-equal -Wextra)
add_compile_options(-Wno-unused)

# Always use position-independent code and highest optimization level (FPS!).
add_compile_options(-O3 -fPIC)

# X-Plane plugin


################################################################################
# Source groups
################################################################################
set(Header_Files
    ../LTAPI.h
)
source_group("Header Files" FILES ${Header_Files})

set(Source_Files
    ../LTAPI.cpp
    LTAPIExample.cpp
)
source_group("Source Files" FILES ${Source_Files})

set(ALL_FILES  ${Header_Files} ${Source_Files})

add_library(LT_API SHARED ${ALL_FILES})

target_compile_features(LT_API PUBLIC cxx_std_11)

if (APPLE)
    # X-Plane supports OS X 10.10+, so this should ensure FlyWithLua can run on
    # all supported versions.
    target_compile_options(LT_API PUBLIC -mmacosx-version-min=10.10)
    target_link_libraries(LT_API -mmacosx-version-min=10.10)
endif ()

# Link OpenGL and OpenAL related libraries.
set (OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)  # apt install freeglut3-dev
if ( OpenGL_FOUND )
    include_directories( ${OpenGL_INCLUDE_DIRS} )
    target_link_libraries( LT_API ${OpenGL_LIBRARIES} )
endif( OpenGL_FOUND )


# Link X-Plane plugin system libraries. They are only provided for OS X and Windows.
if (WIN32 OR APPLE)
# if (WIN32)
    find_library(XPLM_LIBRARY NAMES XPLM XPLM_64.lib)
	find_library(XPWIDGETS_LIBRARY NAMES XPWidgets XPWidgets_64.lib)
    target_link_libraries(LT_API ${XPLM_LIBRARY} ${XPWIDGETS_LIBRARY})
endif ()


# Link library for dynamic loading of shared objects on UNIX systems.
if (UNIX)
    find_library(DL_LIBRARY dl)
    target_link_libraries(LT_API ${DL_LIBRARY})
endif ()

# Link OS X core system libraries.
if (APPLE)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(CORE_FOUNDATION_LIBRARY CoreFoundation)
    find_library(GLUT_LIBRARY GLUT)
    find_library(OpenGL_LIBRARY OpenGL)
    find_library(Cocoa_LIBRARY Cocoa)

    target_link_libraries(LT_API ${IOKIT_LIBRARY} ${CORE_FOUNDATION_LIBRARY} ${GLUT_LIBRARY})
    target_link_libraries(LT_API ${OpenGL_LIBRARY} ${Cocoa_LIBRARY})
endif ()

if (WIN32)
    # Unlike OS X and Linux we build standard libraries statically since X-Plane does not provide them.
    target_link_libraries(LT_API -static-libgcc -static-libstdc++ opengl32)
    # MSYS2 for some reason always links against winpthread, so we have to work around it by statically linking the
    # entire plugin. This allows building the plugin nativaly on Windows.
    target_link_libraries(LT_API -static)
elseif (APPLE)
    # Restrict set of symbols exported from the plugin. This reduces changes of
    # conflict with other plugins, in particular ones with Lua interpreter
    # embedded.
    target_link_libraries(LT_API "-exported_symbols_list ${CMAKE_SOURCE_DIR}/LTAPIExample.sym_mac")
	target_link_libraries(LT_API "-framework XPLM -framework XPWidgets -ldl")
elseif (UNIX)
    # Specify additional runtime search laths for dynamically-linked libraries.
    # Restrict set of symbols exported from the plugin. This reduces changes of
    # conflict with other plugins, in particular ones with Lua interpreter
    # embedded.
    message(${CMAKE_CURRENT_SOURCE_DIR})
    list(APPEND CMAKE_LIBRARY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Lib/xplanemp")

#    find_library(XPLANEMP_LIBRARY NAMES libxplanemp.a)
#    target_link_libraries(LT_API ${XPLANEMP_LIBRARY})

    target_link_libraries(LT_API -Wl,--version-script -Wl,${CMAKE_SOURCE_DIR}/LTAPIExample.sym)
endif ()

set_target_properties(LT_API PROPERTIES PREFIX "")
if (WIN32)
   set_target_properties(LT_API PROPERTIES OUTPUT_NAME "LTAPIExample-win")
elseif (APPLE)
   set_target_properties(LT_API PROPERTIES OUTPUT_NAME "LTAPIExample-mac")
elseif (UNIX)
   set_target_properties(LT_API PROPERTIES OUTPUT_NAME "LTAPIExample-lin")
endif ()
set_target_properties(LT_API PROPERTIES SUFFIX ".xpl")

# set_target_properties(LT_API PROPERTIES PREFIX "")
# set_target_properties(LT_API PROPERTIES OUTPUT_NAME "LT_API")
# set_target_properties(LT_API PROPERTIES SUFFIX ".xpl")
