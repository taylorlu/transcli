TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

DEFINES += STRPRO_STATIC WATERMARK_STATIC
INCLUDEPATH += ./ ../include ../common ../transnode ../strpro

LIBS += -L$$_PRO_FILE_PWD_/../lib
LIBS += -ltransnode -lfaac -llame -lx264 \
   -lx265 -lxvidcore -leac3enc -lsamplerate -lwatermark -lcommon -lstrpro -lxml2 -liconv

DEPENDPATH += $$_PRO_FILE_PWD_/../lib

SOURCES += clihelper.cpp pplive.cpp transcli.cpp

HEADERS += clihelper.h
DESTDIR = $$_PRO_FILE_PWD_/../bin

win32 {
    DEPENDPATH += $$_PRO_FILE_PWD_/../lib/ia32
    LIBS += -L$$_PRO_FILE_PWD_/../lib/ia32 -lWSock32 -luser32 -lzlib1
}

unix {
    DEPENDPATH += $$_PRO_FILE_PWD_/../lib/x64_linux
    LIBS += -L$$_PRO_FILE_PWD_/../lib/x64_linux -lc -lzstatic -lpthread -ldl -lirc
}
