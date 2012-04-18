cmake_minimum_required(VERSION 2.8)

# build static and shared NDLCom-lib, aswell as static and shared demoapp
# uses some sibling-directorys of this repository, so keep them at the right place
project(NDLCom)

# this allows installing out of project tree with dynamic linkage and relocatable files
# see http://www.cmake.org/Wiki/CMake_RPATH_handling
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

find_package(Qt4 REQUIRED)
set(QT_USE_QTNETWORK true)
include(${QT_USE_FILE})

# we use these "mini-libs" to link against. they are rebuilt inside our build-tree (called out of source build)
# each subdir is a "project", so some standard variables are defined therein (like protocol_SOURCE_DIR for example)
# but we ourself may be add_subdirectory'ed, with some of the targets already present. so check this!
if(NOT TARGET udpcom)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../udpcom udpcom)
endif(NOT TARGET udpcom)
if(NOT TARGET serialcom)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../serialcom serialcom)
endif(NOT TARGET serialcom)
if(NOT TARGET protocol)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../protocol protocol)
endif(NOT TARGET protocol)
if(NOT TARGET representations)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../representations representations)
endif(NOT TARGET representations)

SET(ENV{PKG_CONFIG_PATH} ${CMAKE_BINARY_DIR}:$ENV{PKG_CONFIG_PATH})

find_package(PkgConfig)
pkg_check_modules(NDLCom_PKGCONFIG REQUIRED
    serialcom udpcom representations protocol
    )

include_directories(${NDLCom_PKGCONFIG_INCLUDE_DIRS})
link_directories(${NDLCom_PKGCONFIG_LIBRARY_DIRS})
add_definitions(${NDLCom_PKGCONFIG_CFLAGS_OTHER})

include_directories(include)

add_subdirectory(src)
add_subdirectory(gui EXCLUDE_FROM_ALL)

configure_file(Find${PROJECT_NAME}.cmake.in Find${PROJECT_NAME}.cmake @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Find${PROJECT_NAME}.cmake DESTINATION CMake-Modules)

# pkg-config, to be installed:
configure_file(${PROJECT_NAME}.pc.in ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc DESTINATION lib/pkgconfig)
# pkg-config for use inside our buildtree
configure_file(${PROJECT_NAME}-uninstalled.pc.in ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-uninstalled.pc @ONLY)

