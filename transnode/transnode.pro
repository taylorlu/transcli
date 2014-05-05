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
    CEac3Encode.cpp \
    flvmuxer/AACParse.cpp \
    flvmuxer/AudioSourceData.cpp \
    flvmuxer/Box.cpp \
    flvmuxer/DecodeConfig.cpp \
    flvmuxer/FileMixer.cpp \
    flvmuxer/FileWrite.cpp \
    flvmuxer/GlobalDefine.cpp \
    flvmuxer/H264Parse.cpp \
    flvmuxer/MetaUnit.cpp \
    flvmuxer/PPS.cpp \
    flvmuxer/SourceData.cpp \
    flvmuxer/SPS.cpp \
    flvmuxer/VideoSourceData.cpp \
    fdkAacEncode.cpp

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
    CEac3Encode.h \
    flvmuxer/AACParse.h \
    flvmuxer/AudioSourceData.h \
    flvmuxer/Box.h \
    flvmuxer/CommonDef.h \
    flvmuxer/DecodeConfig.h \
    flvmuxer/FileMixer.h \
    flvmuxer/FileWriter.h \
    flvmuxer/GlobalDefine.h \
    flvmuxer/H264Parse.h \
    flvmuxer/MetaUnit.h \
    flvmuxer/PPS.h \
    flvmuxer/SourceData.h \
    flvmuxer/SPS.h \
    flvmuxer/TypeDef.h \
    flvmuxer/VideoSourceData.h \
    fdkAacEncode.h
unix {
    target.path = /usr/lib
    INSTALLS += target
    DESTDIR = $$_PRO_FILE_PWD_/../lib/x64_linux
}

win32 {
    DESTDIR = $$_PRO_FILE_PWD_/../lib/win32
}
