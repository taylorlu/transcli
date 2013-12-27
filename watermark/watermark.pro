#-------------------------------------------------
#
# Project created by QtCreator 2013-12-25T16:00:39
#
#-------------------------------------------------

QT       -= core gui

TARGET = watermark
TEMPLATE = lib
CONFIG += staticlib
#DEFINES += WATERMARK_STATIC
INCLUDEPATH += ./ ../common ../include
SOURCES += \
    ImageSrc.cpp \
    ThumbnailFilter.cpp \
    VideoPreview.cpp \
    WaterMarkFilter.cpp \
    cimg/jcapimin.c \
    cimg/jcapistd.c \
    cimg/jccoefct.c \
    cimg/jccolor.c \
    cimg/jcdctmgr.c \
    cimg/jchuff.c \
    cimg/jcinit.c \
    cimg/jcmainct.c \
    cimg/jcmarker.c \
    cimg/jcmaster.c \
    cimg/jcomapi.c \
    cimg/jcparam.c \
    cimg/jcphuff.c \
    cimg/jcprepct.c \
    cimg/jcsample.c \
    cimg/jctrans.c \
    cimg/jdapimin.c \
    cimg/jdapistd.c \
    cimg/jdatadst.c \
    cimg/jdatasrc.c \
    cimg/jdcoefct.c \
    cimg/jdcolor.c \
    cimg/jddctmgr.c \
    cimg/jdhuff.c \
    cimg/jdinput.c \
    cimg/jdmainct.c \
    cimg/jdmarker.c \
    cimg/jdmaster.c \
    cimg/jdmerge.c \
    cimg/jdphuff.c \
    cimg/jdpostct.c \
    cimg/jdsample.c \
    cimg/jdtrans.c \
    cimg/jerror.c \
    cimg/jfdctflt.c \
    cimg/jfdctfst.c \
    cimg/jfdctint.c \
    cimg/jidctflt.c \
    cimg/jidctfst.c \
    cimg/jidctint.c \
    cimg/jidctred.c \
    cimg/jmemansi.c \
    cimg/jmemmgr.c \
    cimg/jquant1.c \
    cimg/jquant2.c \
    cimg/jutils.c \
    cimg/png.c \
    cimg/pngerror.c \
    cimg/pnggccrd.c \
    cimg/pngget.c \
    cimg/pngmem.c \
    cimg/pngpread.c \
    cimg/pngread.c \
    cimg/pngrio.c \
    cimg/pngrtran.c \
    cimg/pngrutil.c \
    cimg/pngset.c \
    cimg/pngtrans.c \
    cimg/pngvcrd.c \
    cimg/pngwio.c \
    cimg/pngwrite.c \
    cimg/pngwtran.c \
    cimg/pngwutil.c

HEADERS += \
    ImageSrc.h \
    ThumbnailFilter.h \
    VideoPreview.h \
    watermakermacro.h \
    WaterMarkFilter.h \
    cimg/CImg.h \
    cimg/jchuff.h \
    cimg/jconfig.h \
    cimg/jdct.h \
    cimg/jdhuff.h \
    cimg/jerror.h \
    cimg/jinclude.h \
    cimg/jmemsys.h \
    cimg/jmorecfg.h \
    cimg/jpegint.h \
    cimg/jpeglib.h \
    cimg/jversion.h \
    cimg/png.h \
    cimg/pngconf.h \
    cimg/zconf.h \
    cimg/zlib.h
unix {
    target.path = /usr/lib
    INSTALLS += target
}
win32 {
    DESTDIR = $$_PRO_FILE_PWD_/../lib
}
