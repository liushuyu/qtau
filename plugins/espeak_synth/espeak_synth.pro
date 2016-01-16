#-------------------------------------------------
#-------------------------------------------------

TEMPLATE = lib
CONFIG  += plugin
TARGET   = $$qtLibraryTarget(espeaksynth)

#ecantorix is at https://github.com/divVerent/ecantorix
INCLUDEPATH += ../../editor ../ ../../tools
LIBS += -lespeak -lsndfile -Wl,--no-undefined

PKGCONFIG += glib-2.0

LIBS += -lsmf

CONFIG      += link_pkgconfig


HEADERS += \
    espeak_synth.h \
    ../../tools/utauloid/oto.h \
    ../../editor/Utils.h

SOURCES += \
    espeak_synth.cpp \
    ../../editor/Utils.cpp

QMAKE_CXXFLAGS += -Wall -std=c++11

#--------------------------------------------
CONFIG(debug, debug|release) {
    COMPILEDIR = $${OUT_PWD}/../../debug
} else {
    COMPILEDIR = $${OUT_PWD}/../../release
}

DESTDIR         = $${COMPILEDIR}/plugins
OBJECTS_DIR     = $${COMPILEDIR}/espeaksynth/.obj
MOC_DIR         = $${COMPILEDIR}/espeaksynth/.moc
RCC_DIR         = $${COMPILEDIR}/espeaksynth/.rcc
UI_DIR          = $${COMPILEDIR}/espeaksynth/.ui
#--------------------------------------------
