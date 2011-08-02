TEMPLATE = app
TARGET = TestGUINDLCom

MACHINE = $$system($$QMAKE_CC -dumpmachine)

OBJECTS_DIR = ../build/$${MACHINE}/
MOC_DIR = ../build/$${MACHINE}/
UI_DIR = ../build/$${MACHINE}/
DESTDIR = ../

# linking depends on these library-files
POST_TARGETDEPS += \
        ../lib/$$MACHINE/libNDLCom.a \
        ../../serialcom/lib/$$MACHINE/libserialcom.a \
        ../../udpcom/lib/$$MACHINE/libudpcom.a \
        ../../representations/lib/$$MACHINE/librepresentations.a \
        ../../protocol/lib/$$MACHINE/libprotocol.a \

DEPENDPATH += ../src
INCLUDEPATH += ../include ../../protocol/include

LIBS += \
        -L../lib/$${MACHINE} -lNDLCom \
        -L../../serialcom/lib/$$MACHINE -lserialcom \
        -L../../udpcom/lib/$$MACHINE -ludpcom \
        -L../../representations/lib/$$MACHINE -lrepresentations \
        -L../../protocol/lib/$$MACHINE -lprotocol \

CONFIG += debug gui

# Inputfiles:
HEADERS += \
    testGUI.h

FORMS += \
    testGUI.ui

SOURCES += \
    testGUI.cpp \
    main.cpp \
