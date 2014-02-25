#-------------------------------------------------
#
# Project created by QtCreator 2013-12-24T17:10:26
#
#-------------------------------------------------

QT       -= core gui

TARGET = strpro
TEMPLATE = lib
CONFIG += staticlib

#DEFINES += STRPRO_STATIC
INCLUDEPATH += ./ ../include

SOURCES += charset.cpp StrHelper.cpp xmlaccessor.cpp xmlaccessor2.cpp

HEADERS += charset.h StrHelper.h StrProInc.h StrProMacro.h xmlaccessor.h xmlaccessor2.h
unix {
    target.path = /usr/lib
    INSTALLS += target
    DESTDIR = $$_PRO_FILE_PWD_/../lib/x64_linux
}

win32 {
    DESTDIR = $$_PRO_FILE_PWD_/../lib/win32
}
