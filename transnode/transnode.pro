#-------------------------------------------------
#
# Project created by QtCreator 2013-12-25T19:26:04
#
#-------------------------------------------------

QT       -= core gui

TARGET = transnode
TEMPLATE = lib
CONFIG += staticlib
INCLUDEPATH += ./ ../common ../include ../strpro ../watermark

SOURCES += \
    AudioEncode.cpp \
    AudioResampler.cpp \
    CLIEncoder.cpp \
    CX264Encode.cpp \
    CX265Encode.cpp \
    Decoder.cpp \
    DecoderCopy.cpp \
    DecoderFFMpeg.cpp \
    encoderCommon.cpp \
    FaacEncode.cpp \
    FileQueue.cpp \
    MediaSplitter.cpp \
    Mp3Encode.cpp \
    muxers.cpp \
    StreamOutput.cpp \
    TransnodeUtils.cpp \
    TransWorker.cpp \
    TransWorkerSeperate.cpp \
    VideoEncode.cpp \
    videoEnhancer.cpp \
    WorkManager.cpp \
    XvidEncode.cpp \
    CEac3Encode.cpp

HEADERS += \
    AudioEncode.h \
    AudioResampler.h \
    CEac3Encode.h \
    CLIEncoder.h \
    CX264Encode.h \
    CX265Encode.h \
    Decoder.h \
    DecoderCopy.h \
    DecoderFFMpeg.h \
    encoderCommon.h \
    FaacEncode.h \
    FileQueue.h \
    MediaSplitter.h \
    Mp3Encode.h \
    muxers.h \
    StreamOutput.h \
    TransnodeUtils.h \
    TransWorker.h \
    TransWorkerSeperate.h \
    VideoEncode.h \
    videoEnhancer.h \
    WorkManager.h \
    XvidEncode.h \
    CEac3Encode.h
unix {
    target.path = /usr/lib
    INSTALLS += target
}

DESTDIR = $$_PRO_FILE_PWD_/../lib
