#-------------------------------------------------
# http://github.com/qtau-devgroup/editor
#-------------------------------------------------

QT += core widgets network

TARGET = QTau
TEMPLATE = app

INCLUDEPATH += ../tools
PKGCONFIG += glib-2.0

LIBS += -lsmf

CONFIG      += link_pkgconfig

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    Session.cpp \
    Controller.cpp \
    ui/piano.cpp \
    ui/noteEditor.cpp \
    ui/dynDrawer.cpp \
    ui/meter.cpp \
    Utils.cpp \
    ui/noteEditorHandlers.cpp \
    ../audio/jackconnector.cpp \
    ui/tempodialog.cpp



HEADERS  += \
    mainwindow.h \
    PluginInterfaces.h \
    Events.h \
    NoteEvents.h \
    Controller.h \
    Session.h \
    ui/piano.h \
    ui/noteEditor.h \
    ui/dynDrawer.h \
    ui/meter.h \
    ui/Config.h \
    Utils.h \
    ui/noteEditorHandlers.h \
    ../audio/jackconnector.h \
    ui/tempodialog.h

FORMS += ui/mainwindow.ui \
    ui/tempodialog.ui

RESOURCES += res/qtau.qrc

windows:RC_FILE = res/qtau_win.rc

QMAKE_CXXFLAGS += -Wall -std=c++11

#--------------------------------------------
CONFIG(debug, debug|release) {
    DESTDIR = $${OUT_PWD}/../debug
} else {
    DESTDIR = $${OUT_PWD}/../release
}

OBJECTS_DIR     = $${DESTDIR}/editor/.obj
MOC_DIR         = $${DESTDIR}/editor/.moc
RCC_DIR         = $${DESTDIR}/editor/.rcc
UI_DIR          = $${DESTDIR}/editor/.ui
#--------------------------------------------

INCLUDEPATH += ../tools/libogg-1.3.1/include ../tools/flac-1.3.0/include ../tools/flac-1.3.0/src/libFLAC/include

DEFINES += HAVE_CONFIG_H

QMAKE_CFLAGS += -std=c99

LIBS += -ljack
