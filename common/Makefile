# Makefile
include ../config.mk

SUBDIRNAME  :=common
INCLUDES    = $(ROOTDIR) \
                $(ROOTDIR)/httpd \
                $(ROOTDIR)/common \
                $(ROOTDIR)/strpro
INCLUDEFILES = tsdef.h

DEFINES     = STRPRO_STATIC
ALTIVECFLAGS=
LDFLAGS     =

ifeq ($(DEBUG),yes)
DEFINES 	+= DEBUG_EXTERNAL_CMD
endif

LIBNAME = libcommon.a
SRCS = MEvaluater.cpp rootprefs.cpp xmlpref.cpp \
	SysPerfInfo.cpp MediaTools.cpp \
	socket.cpp ftpclient.cpp httpclient.c httpd.cpp \
	discoverer.cpp logger.cpp \
	streamprefs.cpp processwrapper_linux.cpp \
	md5.c auth.c license.cpp \
	util.cpp yuvUtil.cpp \
	SplitterPrefs.cpp PlaylistGenerator.cpp \
	osdep.cpp httprequest.cpp sendmail.cpp

include ../rules.mk

default: $(DEPEND) $(LIBNAME)
