#-------------------------------------------------
#
# Project created by QtCreator 2013-12-26T13:20:38
#
#-------------------------------------------------

QT       -= core gui

TARGET = samplerate
TEMPLATE = lib
CONFIG += staticlib

INCLUDEPATH += ./ ../include

SOURCES += \
    src/samplerate.c \
    src/src_linear.c \
    src/src_sinc.c \
    src/src_zoh.c

HEADERS += \
    src/common.h \
    src/fastest_coeffs.h \
    src/float_cast.h \
    src/high_qual_coeffs.h \
    src/mid_qual_coeffs.h \
    src/samplerate.h
unix {
    target.path = /usr/lib
    INSTALLS += target
    DESTDIR = $$_PRO_FILE_PWD_/../lib/x64_linux
}
win32 {
    DESTDIR = $$_PRO_FILE_PWD_/../lib/win32
}
