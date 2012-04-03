cmake_minimum_required(VERSION 2.6)

# build static and shared NDLCom-lib, aswell as static and shared demoapp
# uses some sibling-directorys of this repository, so keep them at the right place
project(NDLCom)

# this allows installing out of project tree with dynamic linkage and relocatable files
# see http://www.cmake.org/Wiki/CMake_RPATH_handling
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

find_package(Qt4 REQUIRED)
set(QT_USE_QTNETWORK true)

# by saying this here, all subdirs provided thereafter are built with "-Wall"...
add_definitions(-Wall -g)

# we use these "mini-libs" to link against. they are rebuilt inside our build-tree (called out of source build)
# each subdir is a "project", so some standard variables are defined therein (like protocol_SOURCE_DIR for example)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../udpcom udpcom)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../serialcom serialcom)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../protocol protocol)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../representations representations)

include(${QT_USE_FILE})

include_directories(
    include
    ${CMAKE_CURRENT_BINARY_DIR}
    # all dependencys are subdirs, needed includes get stored in each build-dir (which are subdirs to our build dir)
    ${protocol_SOURCE_DIR}/include
    ${representations_SOURCE_DIR}/include
    ${representations_BINARY_DIR}/include
    ${serialcom_SOURCE_DIR}/include
    ${udpcom_SOURCE_DIR}/include
    )

link_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    # all dependencys are subdirs, compiled libs get stored in each build-dir (which are subdirs to our build dir)
    ${protocol_BINARY_DIR}
    ${representations_BINARY_DIR}
    ${serialcom_BINARY_DIR}
    ${udpcom_BINARY_DIR}
    )

#
# first part: set every thing up to compile the lib (both shared and dynamic)
#
set(lib_SOURCES
    src/communication_statistic_widget.cpp
    src/composer.cpp
    src/data_line_input.cpp
    src/hex_input.cpp
    src/interface.cpp
    src/interface_container.cpp
    src/interface_traffic.cpp
    src/message.cpp
    src/message_traffic.cpp
    src/ndlcom.cpp
    src/ndlcom_container.cpp
    src/representation_mapper.cpp
    src/serialcom.cpp
    src/udpcom.cpp
    src/udpcom_connect_dialog.cpp
    src/udpcom_receive_thread.cpp
    )

set(lib_HEADERS
    include/NDLCom/communication_statistic_widget.h
    include/NDLCom/composer.h
    include/NDLCom/interface.h
    include/NDLCom/interface_container.h
    include/NDLCom/interface_traffic.h
    include/NDLCom/message_traffic.h
    include/NDLCom/ndlcom.h
    include/NDLCom/ndlcom_container.h
    include/NDLCom/representation_mapper.h
    include/NDLCom/serialcom.h
    include/NDLCom/udpcom.h
    src/data_line_input.h
    src/hex_input.h
    src/udpcom_connect_dialog.h
    src/udpcom_receive_thread.h
    )
# here we only mention additionally which libs should get installed in the system
set(install_HEADERS
    include/NDLCom/interface.h
    include/NDLCom/interface_container.h
    include/NDLCom/interface_traffic.h
    include/NDLCom/message_traffic.h
    include/NDLCom/ndlcom.h
    include/NDLCom/ndlcom_container.h
    include/NDLCom/representation_mapper.h
    )

set(lib_FORMS
    forms/composer.ui
    forms/show_representations.ui
    forms/udpcom_connect_dialog.ui
    forms/interface_container.ui
    forms/interface.ui
    forms/communication_statistic_widget.ui
    forms/traffic.ui
    )

set(lib_RESSOURCES
    src/NDLCom.qrc
)

# some qt-magic...
qt4_wrap_cpp(lib_HEADERS_MOC ${lib_HEADERS})
qt4_wrap_ui(lib_FORMS_HEADERS ${lib_FORMS})
qt4_add_resources(lib_RESSOURCES_RCC ${lib_RESSOURCES})

#
# after setting everything up, we define two tragets: shared and dynamic, and link both with the respective libs
#
add_library(${PROJECT_NAME}_static STATIC
    ${lib_SOURCES}
    ${lib_HEADERS}
    ${lib_HEADERS_MOC}
    ${lib_FORMS_HEADERS}
    ${lib_RESSOURCES_RCC}
    include/NDLCom/message.h
    )
target_link_libraries(${PROJECT_NAME}_static
    ${QT_LIBRARIES}
    protocol_static
    representations_static
    serialcom_static
    udpcom_static
    )
set_target_properties(${PROJECT_NAME}_static PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}
    )
add_dependencies(${PROJECT_NAME}_static
    protocol_static
    representations_static
    udpcom_static
    serialcom_static
    )

#
# like i said, same with shared:
#
add_library(${PROJECT_NAME}_shared SHARED
    ${lib_SOURCES}
    ${lib_HEADERS}
    ${lib_HEADERS_MOC}
    ${lib_FORMS_HEADERS}
    ${lib_RESSOURCES_RCC}
    include/NDLCom/message.h
    )
target_link_libraries(${PROJECT_NAME}_shared
    ${QT_LIBRARIES}
    protocol_shared
    representations_shared
    serialcom_shared
    udpcom_shared
    )
set_target_properties(${PROJECT_NAME}_shared PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}
    # this switch is used to deactivate "Q_INIT_RESSOURCE" macro on dynamic builds
    COMPILE_FLAGS -DSHARED_BUILD
    )
add_dependencies(${PROJECT_NAME}_shared
    protocol_shared
    representations_shared
    udpcom_shared
    serialcom_shared
    )

#
# setting up the demo app
#
set(gui_SOURCES
    gui/testGUI.cpp
    gui/main.cpp
    )

set(gui_HEADERS
    gui/testGUI.h
    )

set(gui_FORMS
    gui/testGUI.ui
    )

qt4_wrap_cpp(gui_HEADERS_MOC ${gui_HEADERS})
qt4_wrap_ui(gui_FORMS_HEADERS ${gui_FORMS})

#
# defining shared one
#
add_executable(${PROJECT_NAME}_sharedgui
    ${gui_SOURCES}
    ${gui_HEADERS}
    ${gui_HEADERS_MOC}
    ${gui_FORMS_HEADERS}
    )
target_link_libraries(${PROJECT_NAME}_sharedgui
    ${QT_LIBRARIES}
    ${PROJECT_NAME}_shared
    )
set_target_properties(${PROJECT_NAME}_sharedgui PROPERTIES
    # this switch is used to deactivate "Q_INIT_RESSOURCE" macro on dynamic builds
    COMPILE_FLAGS -DSHARED_BUILD
    )

#
# same demo app as static build
#
add_executable(${PROJECT_NAME}_staticgui
    ${gui_SOURCES}
    ${gui_HEADERS}
    ${gui_HEADERS_MOC}
    ${gui_FORMS_HEADERS}
    )
target_link_libraries(${PROJECT_NAME}_staticgui
    ${QT_LIBRARIES}
    ${PROJECT_NAME}_static
    )

#
# installing
#
install(TARGETS ${PROJECT_NAME}_shared ${PROJECT_NAME}_static ${PROJECT_NAME}_sharedgui ${PROJECT_NAME}_staticgui
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    )

install(FILES ${install_HEADERS} DESTINATION include/${PROJECT_NAME})

configure_file(${PROJECT_SOURCE_DIR}/${PROJECT_NAME}.pc.in ${PROJECT_NAME}.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc DESTINATION lib/pkgconfig)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Find${PROJECT_NAME}.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/Find${PROJECT_NAME}.cmake @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Find${PROJECT_NAME}.cmake DESTINATION CMake-Modules)
