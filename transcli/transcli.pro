TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SVN_REVISION = $$system("svn info $$_PRO_FILE_PWD_/.. | grep 'Last Changed Rev:' | awk '{print $4}'")
SVN_DATE = $$system("svn info $$_PRO_FILE_PWD_/.. | grep 'Last Changed Date:' | awk '{print $4,$5}'")
SVN_NOW = $$system("date -u +'%F %T'")
DEFINES += __SVN_REVISION_H__
DEFINES += SVN_REVISION="\'\"$$SVN_REVISION\"\'"
DEFINES += SVN_DATE="\'\"$$SVN_DATE\"\'"
DEFINES += SVN_NOW="\'\"$$SVN_NOW\"\'"
message(transclidef:$$DEFINES)

DEFINES += STRPRO_STATIC WATERMARK_STATIC
INCLUDEPATH += ./ ../include ../common ../transnode ../strpro

LIBS += -ltransnode -lfaac -llame -lx264 \
   -lx265 -lxvidcore -leac3enc -lfdk-aac -lsamplerate -lwatermark -lcommon -lstrpro -lxml2 -liconv

DEPENDPATH += $$_PRO_FILE_PWD_/../lib

SOURCES += clihelper.cpp pplive.cpp transcli.cpp

HEADERS += clihelper.h

CONFIG(debug, debug|release) {
   QMAKE_CXXFLAGS += -D_DEBUG=1
}

win32 {
    DEPENDPATH += $$_PRO_FILE_PWD_/../lib/ia32
    LIBS += -L$$_PRO_FILE_PWD_/../lib/win32/intel -L$$_PRO_FILE_PWD_/../lib/win32 -lWSock32 -luser32 -lzlib1
    DESTDIR = $$_PRO_FILE_PWD_/../bin/win32
}

unix {
    DEPENDPATH += $$_PRO_FILE_PWD_/../lib/x64_linux
    LIBS += -L$$_PRO_FILE_PWD_/../lib/x64_linux -lc -lzstatic -lpthread -ldl -lirc
    DESTDIR = $$_PRO_FILE_PWD_/../bin/x64_linux
}
