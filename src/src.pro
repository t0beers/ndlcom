TEMPLATE = lib
TARGET = NDLCom

OBJECTS_DIR = ../build/$$system($$QMAKE_CC -dumpmachine)/
MOC_DIR = ../build/$$system($$QMAKE_CC -dumpmachine)/
UI_DIR = ../build/$$system($$QMAKE_CC -dumpmachine)/
RCC_DIR = ../build/$$system($$QMAKE_CC -dumpmachine)/
DESTDIR = ../lib/$$system($$QMAKE_CC -dumpmachine)/

SRCDIR = .
FORMSDIR = ../forms
HEADERSDIR = ../include/NDLCom

QT += network

CONFIG += staticlib
CONFIG += debug

DEPENDPATH += . ../include

INCLUDEPATH += ../build ../include $$UI_DIR ../../serialcom/include ../../protocol/include ../../representations/include ../../udpcom/include

# Input
FORMS   += \
    $${FORMSDIR}/communication_statistic_widget.ui \
    $${FORMSDIR}/composer.ui \
    $${FORMSDIR}/interface.ui \
    $${FORMSDIR}/interface_container.ui \
    $${FORMSDIR}/show_representations.ui \
    $${FORMSDIR}/traffic.ui \
    $${FORMSDIR}/udpcom_connect_dialog.ui \

RESOURCES += NDLCom.qrc

HEADERS += \
    $${HEADERSDIR}/communication_statistic_widget.h \
    $${HEADERSDIR}/composer.h \
    $${HEADERSDIR}/interface.h \
    $${HEADERSDIR}/interface_container.h \
    $${HEADERSDIR}/interface_traffic.h \
    $${HEADERSDIR}/message.h \
    $${HEADERSDIR}/message_traffic.h \
    $${HEADERSDIR}/ndlcom.h \
    $${HEADERSDIR}/ndlcom_container.h \
    $${HEADERSDIR}/representation_mapper.h \
    $${HEADERSDIR}/serialcom.h \
    $${HEADERSDIR}/udpcom.h \
    $${SRCDIR}/data_line_input.h \
    $${SRCDIR}/hex_input.h \
    $${SRCDIR}/udpcom_connect_dialog.h \
    $${SRCDIR}/udpcom_receive_thread.h \

SOURCES += \
    $${SRCDIR}/communication_statistic_widget.cpp \
    $${SRCDIR}/composer.cpp \
    $${SRCDIR}/data_line_input.cpp \
    $${SRCDIR}/hex_input.cpp \
    $${SRCDIR}/interface.cpp \
    $${SRCDIR}/interface_container.cpp \
    $${SRCDIR}/interface_traffic.cpp \
    $${SRCDIR}/message.cpp \
    $${SRCDIR}/message_traffic.cpp \
    $${SRCDIR}/ndlcom.cpp \
    $${SRCDIR}/ndlcom_container.cpp \
    $${SRCDIR}/representation_mapper.cpp \
    $${SRCDIR}/serialcom.cpp \
    $${SRCDIR}/udpcom.cpp \
    $${SRCDIR}/udpcom_connect_dialog.cpp \
    $${SRCDIR}/udpcom_receive_thread.cpp \

#Doxygen
doxydoc.target = doc
doxydoc.commands = doxygen ../doc/Doxyfile
doxydoc.depends = FORCE
QMAKE_EXTRA_TARGETS += doxydoc
