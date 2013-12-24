# Makefile
include ../config.mk

SUBDIRNAME  :=watermark
INCLUDES    = cimg $(ROOTDIR)/common
DEFINES     = WATERMARK_STATIC cimg_display=0
ALTIVECFLAGS= -O3 -ffast-math  -fno-tree-pre
LDFLAGS     = -lm -lz

LIBNAME =libwatermark.a
CIMG_SRCS = \
	jcapimin.c  jcmarker.c  jdapimin.c  jdinput.c   jdtrans.c   jidctred.c \
	jcapistd.c  jcmaster.c  jdapistd.c  jdmainct.c  jerror.c    jmemansi.c \
	jccoefct.c  jcomapi.c   jdatadst.c  jdmarker.c  jfdctflt.c  jmemmgr.c \
	jccolor.c   jcparam.c   jdatasrc.c  jdmaster.c  jfdctfst.c  jquant1.c \
	jcdctmgr.c  jcphuff.c   jdcoefct.c  jdmerge.c   jfdctint.c  jquant2.c \
	jchuff.c    jcprepct.c  jdcolor.c   jdphuff.c   jidctflt.c  jutils.c \
	jcinit.c    jcsample.c  jddctmgr.c  jdpostct.c  jidctfst.c \
	jcmainct.c  jctrans.c   jdhuff.c    jdsample.c  jidctint.c \
	png.c       pngmem.c    pngrtran.c  pngtrans.c  pngwtran.c \
	pngerror.c  pngpread.c  pngrutil.c  pngvcrd.c   pngwutil.c \
	pnggccrd.c  pngread.c   pngset.c    pngwio.c \
	pngget.c    pngrio.c    pngwrite.c 

CIMG_SRCS_WITH_PATH = $(addprefix cimg/,$(CIMG_SRCS))

SRCS = WaterMarkFilter.cpp ThumbnailFilter.cpp
SRCS += $(CIMG_SRCS_WITH_PATH)

include ../rules.mk

default: $(DEPEND) $(LIBNAME)
