# Makefile
include ../config.mk

SUBDIRNAME      :=transcli
INCLUDES        = $(ROOTDIR) \
                  $(ROOTDIR)/transnode \
                  $(ROOTDIR)/strpro $(ROOTDIR)/httpd \
                  $(ROOTDIR)/x264 $(ROOTDIR)/faac/include \
                  $(ROOTDIR)/libxmlrpc/src $(ROOTDIR)/common \
		  $(ROOTDIR)/ffmpeg-build \
		  $(ROOTDIR)/watermark \

DEFINES         := HAVE_MMX STRPRO_STATIC DEBUG_EXTERNAL_CMD
ALTIVECFLAGS    =
LDFLAGS         = -lm -lpthread -ldl -liconv -lstdc++ -lexpat -lstrpro -lxml2 -lsamplerate
FFMPEG_LDFLAGS  = -lavcodec -lavformat -lavutil \
				  -lz -pthread -lm -lfaac -lmp3lame -lm -ltheora -logg -lvorbisenc -lvorbis -logg -lx264 -lm -lxvidcore
CIMG_LDFLAGS    = -lz

LDFLAGS		+= $(FFMPEG_LDFLAGS)

ifeq ($(ENABLE_WATERMARK),yes)
LDFLAGS 	+= $(CIMG_LDFLAGS)
DEFINES		+= WATERMARK_STATIC ENABLE_WATERMARK
else
DEFINES		+= DISABLE_WATERMARK
endif

ifneq ($(SHARED),no)
LDFLAGS		+= -lmp3lame -lfaac -lxvidcore -lx264
endif

APPNAME = transcli
APPSRCS = transcli.cpp clihelper.cpp cisco.cpp pplive.cpp 
DEFINES+=ENABLE_TRANSERVER

APPDEPS = $(LIBDIR)/libtransnode.a \
			$(LIBDIR)/libhttpd.a \
			$(LIBDIR)/libcommon.a \
			$(LIBDIR)/libxmlrpc.a 

ifeq ($(ENABLE_WATERMARK),yes)
APPDEPS += $(LIBDIR)/libwatermark.a
endif

ifeq ($(SHARED),no)
APPDEPS += $(LIBDIR)/libx264.a \
			$(LIBDIR)/libmp3lame.a \
			$(LIBDIR)/libxvidcore.a \
			$(LIBDIR)/libfaac.a
endif

EXTRA_INSTALL=install_scripts

include ../rules.mk

install_scripts:
	-$(INSTALL) -c -m 755 transcli.sh $(BINDIR)

default: $(DEPEND) $(APPNAME)
