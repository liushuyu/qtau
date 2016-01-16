#-------------------------------------------------
#-------------------------------------------------

TEMPLATE = lib
CONFIG  += plugin
TARGET   = $$qtLibraryTarget(vconnectsynth)


#you need to change the path to libvsq if you want to use vconnect-stand
#libvsq is at https://github.com/kbinani/libvsq
INCLUDEPATH += ../../editor ../ ../../tools /home/tobiasplaten/Projects/Music/libvsq/include
LIBS += -lespeak -lsndfile -Wl,--no-undefined -L/home/tobiasplaten/Projects/Music/libvsq/build -lvsq
#get vconnect-stand from https://mentors.debian.net/package/vconnectstand

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
