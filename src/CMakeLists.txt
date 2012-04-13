include_directories(${CMAKE_CURRENT_BINARY_DIR})

#
# first part: set every thing up to compile the lib (both shared and dynamic)
#
set(lib_SOURCES
    communication_statistic_widget.cpp
    composer.cpp
    data_line_input.cpp
    hex_input.cpp
    interface.cpp
    interface_container.cpp
    interface_traffic.cpp
    message.cpp
    message_traffic.cpp
    ndlcom.cpp
    ndlcom_container.cpp
    representation_mapper.cpp
    serialcom.cpp
    udpcom.cpp
    udpcom_connect_dialog.cpp
    udpcom_receive_thread.cpp
    )
set(lib_HEADERS_QT # need moc to be processed
    ../include/NDLCom/communication_statistic_widget.h
    ../include/NDLCom/composer.h
    ../include/NDLCom/interface.h
    ../include/NDLCom/interface_container.h
    ../include/NDLCom/interface_traffic.h
    ../include/NDLCom/message_traffic.h
    ../include/NDLCom/ndlcom_container.h
    ../include/NDLCom/representation_mapper.h
    ../include/NDLCom/serialcom.h
    ../include/NDLCom/udpcom.h
    data_line_input.h
    hex_input.h
    udpcom_connect_dialog.h
    udpcom_receive_thread.h
    )
set(lib_HEADERS # need no moc, plain cpp
    ../include/NDLCom/ndlcom.h
    ../include/NDLCom/message.h
    )
# here we only mention additionally which libs should get installed in the system
set(install_HEADERS
    ../include/NDLCom/interface.h
    ../include/NDLCom/interface_container.h
    ../include/NDLCom/interface_traffic.h
    ../include/NDLCom/message_traffic.h
    ../include/NDLCom/ndlcom.h
    ../include/NDLCom/ndlcom_container.h
    ../include/NDLCom/representation_mapper.h
    )
set(lib_FORMS
    ../forms/composer.ui
    ../forms/show_representations.ui
    ../forms/udpcom_connect_dialog.ui
    ../forms/interface_container.ui
    ../forms/interface.ui
    ../forms/communication_statistic_widget.ui
    ../forms/traffic.ui
    )
set(lib_RESSOURCES
    NDLCom.qrc
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
    set_property(SOURCE ndlcom_container.cpp APPEND PROPERTY COMPILE_FLAGS "-DSHARED_BUILD")
endif(${BUILD_SHARED_LIBS})

#
# installing
#
install(TARGETS ${PROJECT_NAME}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    )

install(FILES ${install_HEADERS} DESTINATION include/${PROJECT_NAME})
