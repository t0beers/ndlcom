include_directories(${CMAKE_CURRENT_BINARY_DIR})

#
# setting up the demo app
#
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
# installing both guis
#
install(TARGETS ${PROJECT_NAME}_sharedgui ${PROJECT_NAME}_staticgui RUNTIME DESTINATION bin)
