#-------------------------------------------------
#
# Project created by QtCreator 2013-12-25T16:31:06
#
#-------------------------------------------------

QT       -= core gui

TARGET = lame
TEMPLATE = lib
CONFIG += staticlib
DEFINES += HAVE_CONFIG_H
INCLUDEPATH += ../include ./libmp3lame ./mpglib

SOURCES += \
    libmp3lame/bitstream.c \
    libmp3lame/encoder.c \
    libmp3lame/fft.c \
    libmp3lame/gain_analysis.c \
    libmp3lame/id3tag.c \
    libmp3lame/lame.c \
    libmp3lame/mpglib_interface.c \
    libmp3lame/newmdct.c \
    libmp3lame/presets.c \
    libmp3lame/psymodel.c \
    libmp3lame/quantize.c \
    libmp3lame/quantize_pvt.c \
    libmp3lame/reservoir.c \
    libmp3lame/set_get.c \
    libmp3lame/tables.c \
    libmp3lame/takehiro.c \
    libmp3lame/util.c \
    libmp3lame/vbrquantize.c \
    libmp3lame/VbrTag.c \
    libmp3lame/version.c \
    mpglib/common.c \
    mpglib/dct64_i386.c \
    mpglib/decode_i386.c \
    mpglib/interface.c \
    mpglib/layer1.c \
    mpglib/layer2.c \
    mpglib/layer3.c \
    mpglib/tabinit.c

HEADERS +=

unix {
    target.path = /usr/lib
    INSTALLS += target
    DESTDIR = $$_PRO_FILE_PWD_/../lib/x64_linux
}

win32 {
    DESTDIR = $$_PRO_FILE_PWD_/../lib/win32
}
