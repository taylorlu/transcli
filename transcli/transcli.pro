TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

DEFINES += STRPRO_STATIC WATERMARK_STATIC
INCLUDEPATH += ./ ../include ../common ../transnode ../strpro

LIBS += -L$$_PRO_FILE_PWD_/../lib -L$$_PRO_FILE_PWD_/../lib/ia32
LIBS += -ltransnode -lfaac -llame -lx264 \
   -lx265 -lxvidcore -leac3enc -lsamplerate -lwatermark -lcommon -lstrpro -lxml2 -liconv -lzlib1

DEPENDPATH += $$_PRO_FILE_PWD_/../lib $$_PRO_FILE_PWD_/../lib/ia32

SOURCES += clihelper.cpp pplive.cpp transcli.cpp

HEADERS += clihelper.h

win32 {
    DESTDIR = $$_PRO_FILE_PWD_/../bin
    LIBS += -lWSock32 -luser32
}
