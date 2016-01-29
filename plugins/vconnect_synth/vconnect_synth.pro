#-------------------------------------------------
#-------------------------------------------------

TEMPLATE = lib
CONFIG  += plugin
TARGET   = $$qtLibraryTarget(vconnectsynth)

INCLUDEPATH += ../../editor ../ ../../tools /usr/local/libvsq/include
LIBS += -lespeak -lsndfile -Wl,--no-undefined -lvsq

PKGCONFIG += glib-2.0

LIBS += -lsmf

CONFIG      += link_pkgconfig


HEADERS += \
   vconnect_synth.h \
    ../../tools/utauloid/oto.h \
    ../../editor/Utils.h

SOURCES += \
   vconnect_synth.cpp \
    ../../editor/Utils.cpp

QMAKE_CXXFLAGS += -Wall -std=c++11

#--------------------------------------------
CONFIG(debug, debug|release) {
    COMPILEDIR = $${OUT_PWD}/../../debug
} else {
    COMPILEDIR = $${OUT_PWD}/../../release
}

DESTDIR         = $${COMPILEDIR}/plugins
OBJECTS_DIR     = $${COMPILEDIR}/vconnectsynth/.obj
MOC_DIR         = $${COMPILEDIR}/vconnectsynth/.moc
RCC_DIR         = $${COMPILEDIR}/vconnectsynth/.rcc
UI_DIR          = $${COMPILEDIR}/vconnectsynth/.ui
#--------------------------------------------
