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

# needed by qt's generated header files (from the ui files)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

#
# first part: set every thing up to compile the lib (both shared and dynamic)
#
set(lib_SOURCES
    src/StaticTools.cpp
    src/communication_statistic_widget.cpp
    src/composer.cpp
    src/data_line_input.cpp
    src/hex_input.cpp
    src/interface.cpp
    src/InterfaceWidget.cpp
    src/interface_container_widget.cpp
    src/interface_container.cpp
    src/interface_traffic.cpp
    src/message.cpp
    src/message_traffic.cpp
    src/ndlcom.cpp
    src/ndlcom_container.cpp
    src/representation_mapper.cpp
    src/AvailableRepresentations.cpp
    src/serialcom.cpp
    src/udpcom.cpp
    src/udpcom_connect_dialog.cpp
    src/udpcom_receive_thread.cpp
    )
set(lib_HEADERS_QT # need moc to be processed
    include/NDLCom/communication_statistic_widget.h
    include/NDLCom/composer.h
    include/NDLCom/interface.h
    include/NDLCom/InterfaceWidget.h
    include/NDLCom/interface_container_widget.h
    include/NDLCom/interface_container.h
    include/NDLCom/interface_traffic.h
    include/NDLCom/message_traffic.h
    include/NDLCom/ndlcom_container.h
    include/NDLCom/representation_mapper.h
    include/NDLCom/AvailableRepresentations.h
    include/NDLCom/serialcom.h
    include/NDLCom/udpcom.h
    src/data_line_input.h
    src/hex_input.h
    src/udpcom_connect_dialog.h
    src/udpcom_receive_thread.h
    )
set(lib_HEADERS # need no moc, plain cpp
    include/NDLCom/StaticTools.h
    include/NDLCom/CommandLineArgument.h
    include/NDLCom/ndlcom.h
    include/NDLCom/message.h
    )
# here we only mention additionally which libs should get installed in the system
set(install_HEADERS
    include/NDLCom/StaticTools.h
    include/NDLCom/ndlcom.h
    include/NDLCom/CommandLineArgument.h
    include/NDLCom/interface_container.h
    )
set(lib_FORMS
    forms/composer.ui
    forms/AvailableRepresentations.ui
    forms/udpcom_connect_dialog.ui
    forms/interface_container_widget.ui
    forms/interface_widget.ui
    forms/communication_statistic_widget.ui
    forms/traffic.ui
    )
set(lib_RESSOURCES
    src/NDLCom.qrc
    )

# some qt-magic...
qt4_wrap_cpp(lib_HEADERS_MOC ${lib_HEADERS_QT})
qt4_wrap_ui(lib_FORMS_HEADERS ${lib_FORMS})
qt4_add_resources(lib_RESSOURCES_RCC ${lib_RESSOURCES})

#
# after setting everything up, we define two tragets: shared and dynamic, and link both with the respective libs
#
add_library(${PROJECT_NAME}
    ${lib_SOURCES}
    ${lib_HEADERS}
    ${lib_HEADERS_QT}
    ${lib_HEADERS_MOC}
    ${lib_FORMS_HEADERS}
    ${lib_RESSOURCES_RCC}
    )
target_link_libraries(${PROJECT_NAME}
    ${QT_LIBRARIES}
    ${NDLCom_PKGCONFIG_LIBRARIES}
    )

if(${BUILD_SHARED_LIBS})
    # this switch is used to deactivate "Q_INIT_RESSOURCE" macro on shared builds
    set_property(SOURCE src/ndlcom_container.cpp APPEND PROPERTY COMPILE_FLAGS "-DSHARED_BUILD")
endif(${BUILD_SHARED_LIBS})

#
# installing
#
install(TARGETS ${PROJECT_NAME}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    )

install(FILES ${install_HEADERS} DESTINATION include/${PROJECT_NAME})

# pkg-config, to be installed:
configure_file(${PROJECT_NAME}.pc.in ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc DESTINATION lib/pkgconfig)
# pkg-config for use inside our buildtree
configure_file(${PROJECT_NAME}-uninstalled.pc.in ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-uninstalled.pc @ONLY)

# still an extra subdir
add_subdirectory(gui EXCLUDE_FROM_ALL)
