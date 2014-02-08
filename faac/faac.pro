#-------------------------------------------------
#
# Project created by QtCreator 2013-12-25T16:18:52
#
#-------------------------------------------------

QT       -= core gui

TARGET = faac
TEMPLATE = lib
CONFIG += staticlib
INCLUDEPATH += ./ ../include
SOURCES += \
    libfaac/aacquant.c \
    libfaac/backpred.c \
    libfaac/bitstream.c \
    libfaac/channels.c \
    libfaac/fft.c \
    libfaac/filtbank.c \
    libfaac/frame.c \
    libfaac/huffman.c \
    libfaac/ltp.c \
    libfaac/midside.c \
    libfaac/psychkni.c \
    libfaac/tns.c \
    libfaac/util.c

HEADERS += \
    libfaac/aacquant.h \
    libfaac/backpred.h \
    libfaac/bitstream.h \
    libfaac/channels.h \
    libfaac/coder.h \
    libfaac/fft.h \
    libfaac/filtbank.h \
    libfaac/frame.h \
    libfaac/huffman.h \
    libfaac/hufftab.h \
    libfaac/ltp.h \
    libfaac/midside.h \
    libfaac/psych.h \
    libfaac/tns.h \
    libfaac/util.h \
    libfaac/version.h
unix {
    target.path = /usr/lib
    INSTALLS += target
}
DESTDIR = $$_PRO_FILE_PWD_/../lib
