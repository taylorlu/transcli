#-------------------------------------------------
#
# Project created by QtCreator 2013-12-24T17:05:03
#
#-------------------------------------------------

QT       -= core gui

TARGET = common
TEMPLATE = lib
CONFIG += staticlib
#DEFINES += STRPRO_STATIC
INCLUDEPATH += ./ ../include ../strpro
SOURCES += \
    logger.cpp \
    MediaTools.cpp \
    MEvaluater.cpp \
    osdep.cpp \
    PlaylistGenerator.cpp \
    rootprefs.cpp \
    SplitterPrefs.cpp \
    streamprefs.cpp \
    util.cpp \
    xmlpref.cpp \
    yuvUtil.cpp \
    zml_gain_analysis.c \
    getopt.c \
    md5.c

HEADERS += \
    _getopt.h \
    zml_gain_analysis.h \
    logger.h \
    md5.h \
    MediaTools.h \
    MEvaluater.h \
    PlaylistGenerator.h \
    processwrapper.h \
    SplitterPrefs.h \
    tsdef.h \
    util.h \
    yuvUtil.h
unix {
    SOURCES += processwrapper_linux.cpp
    target.path = /usr/lib
    INSTALLS += target
    DESTDIR = $$_PRO_FILE_PWD_/../lib/x64_linux
    QMAKE_POST_LINK = cp -rf $$_PRO_FILE_PWD_/mcnt.xml $$_PRO_FILE_PWD_/../bin/x64_linux/mcnt.xml
}
win32 {
    SOURCES += processwrapper.cpp
    DESTDIR = $$_PRO_FILE_PWD_/../lib/win32
}
