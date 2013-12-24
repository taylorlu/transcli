# Makefile
include ../config.mk

SUBDIRNAME      :=transnode
INCLUDES        = $(ROOTDIR) $(ROOTDIR)/strpro $(ROOTDIR)/httpd \
	          $(ROOTDIR)/faac/include \
                  $(ROOTDIR)/libxmlrpc/src $(ROOTDIR)/common \
		  $(ROOTDIR)/watermark
INCLUDEFILES	= NodeApi.h
DEFINES         := HAVE_MMX STRPRO_STATIC __STDC_CONSTANT_MACROS # define __STDC_CONSTANT_MACROS to avoid build error (VideoEncodeFFMpeg.cpp)
ALTIVECFLAGS    =
LDFLAGS         = -lm -lpthread -ldl -liconv -lstdc++ -lexpat -lstrpro
FFMPEG_LDFLAGS  = -lavcodec -lavformat -lavutil \
				  -lz -pthread -lm -lfaac -lmp3lame -lm -ltheora -logg -lvorbisenc -lvorbis -logg -lx264 -lm -lxvidcore
CIMG_LDFLAGS    = -lz

LDFLAGS		+= $(FFMPEG_LDFLAGS)

ifeq ($(DEBUG),yes)
DEFINES 	+= DEBUG_EXTERNAL_CMD
endif

ifeq ($(ENABLE_WATERMARK),yes)
LDFLAGS 	+= $(CIMG_LDFLAGS)
DEFINES		+= WATERMARK_STATIC ENABLE_WATERMARK
else
DEFINES		+= DISABLE_WATERMARK
endif

ifneq ($(SHARED),no)
LDFLAGS		+= -lmp3lame -lfaac -lxvidcore -lx264
endif


LIBNAME = libtransnode.a

SRCS =  adapter.cpp \
	WorkManager.cpp TransnodeUtils.cpp TransWorker.cpp TransWorkerSeperate.cpp TransWorkerUnify.cpp TransWorkerRemote.cpp \
	Decoder.cpp DecoderMplayer.cpp DecoderMencoder.cpp DecoderAVS.cpp DecoderVLC.cpp DecoderCopy.cpp \
	encoderCommon.cpp VideoEncode.cpp CX264Encode.cpp XvidEncode.cpp VideoEncodeFFMpeg.cpp CLIEncoder.cpp AVEncoder.cpp FFMpegAVEncoder.cpp \
	AudioEncode.cpp Mp3Encode.cpp AudioEncodeCmd.cpp FaacEncode.cpp AudioEncodeFFMpeg.cpp AudioResampler.cpp \
	VideoCopier.cpp AudioCopier.cpp \
	StreamOutput.cpp muxers.cpp \
	FileQueue.cpp \
	MediaSplitter.cpp \
	NodeApi.cpp

ifeq ($(ENABLE_TRANSERVER),no)
  DEFINES += ENABLE_SINGLE_MODE
else
  SRCS    += httphandler.cpp
endif

include ../rules.mk

default: $(DEPEND) $(LIBNAME)
