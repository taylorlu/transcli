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
    mplayerwrapper.cpp \
    osdep.cpp \
    PlaylistGenerator.cpp \
    processwrapper.cpp \
    rootprefs.cpp \
    SplitterPrefs.cpp \
    streamprefs.cpp \
    util.cpp \
    xmlpref.cpp \
    yuvUtil.cpp \
    gain_analysis.c \
    getopt.c \
    md5.c

HEADERS += \
    _getopt.h \
    gain_analysis.h \
    logger.h \
    md5.h \
    MediaTools.h \
    MEvaluater.h \
    mplayerwrapper.h \
    PlaylistGenerator.h \
    processwrapper.h \
    SplitterPrefs.h \
    tsdef.h \
    util.h \
    yuvUtil.h
unix {
    target.path = /usr/lib
    INSTALLS += target
}
win32 {
    DESTDIR = $$_PRO_FILE_PWD_/../lib
}
