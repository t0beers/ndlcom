cmake_minimum_required(VERSION 2.8)

project(NDLCom_testGui)

if(NOT TARGET NDLCom)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../ NDLCom)
endif(NOT TARGET NDLCom)

find_package(Qt4 REQUIRED)
set(QT_USE_QTNETWORK true)
include(${QT_USE_FILE})

SET(ENV{PKG_CONFIG_PATH} ${CMAKE_BINARY_DIR}:$ENV{PKG_CONFIG_PATH})
find_package(PkgConfig)
pkg_check_modules(${PROJECT_NAME}_PKGCONFIG REQUIRED NDLCom)
include_directories(${${PROJECT_NAME}_PKGCONFIG_INCLUDE_DIRS})
link_directories(${${PROJECT_NAME}_PKGCONFIG_LIBRARY_DIRS})
add_definitions(${${PROJECT_NAME}_PKGCONFIG_CFLAGS_OTHER})

include_directories(${CMAKE_CURRENT_BINARY_DIR})

# a minimal test-application
set(gui_SOURCES
    testGUI.cpp
    main.cpp
    )
set(gui_HEADERS
    testGUI.h
    )
set(gui_FORMS
    testGUI.ui
    )

qt4_wrap_cpp(gui_HEADERS_MOC ${gui_HEADERS})
qt4_wrap_ui(gui_FORMS_HEADERS ${gui_FORMS})

add_executable(${PROJECT_NAME}
    ${gui_SOURCES}
    ${gui_HEADERS}
    ${gui_HEADERS_MOC}
    ${gui_FORMS_HEADERS}
    )
target_link_libraries(${PROJECT_NAME}
    ${QT_LIBRARIES}
    ${${PROJECT_NAME}_PKGCONFIG_LIBRARIES}
    )
# creating a convenience target, so a user can easily execute the binary, without cd'ing into the builddir
add_custom_target(${PROJECT_NAME}-lazy-symlink ALL ln --force -s ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/${PROJECT_NAME} DEPENDS ${PROJECT_NAME})
# and clean this up if someone asks us to do so
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_SOURCE_DIR}/${PROJECT_NAME})
