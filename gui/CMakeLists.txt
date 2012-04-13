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

add_executable(${PROJECT_NAME}_gui
    ${gui_SOURCES}
    ${gui_HEADERS}
    ${gui_HEADERS_MOC}
    ${gui_FORMS_HEADERS}
    )
target_link_libraries(${PROJECT_NAME}_gui
    ${QT_LIBRARIES}
    ${PROJECT_NAME}
    )

#
# installing
#
install(TARGETS ${PROJECT_NAME}_gui RUNTIME DESTINATION bin)
