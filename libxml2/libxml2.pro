#-------------------------------------------------
#
# Project created by QtCreator 2013-12-25T15:23:38
#
#-------------------------------------------------

QT       -= core gui

TARGET = xml2
TEMPLATE = lib
CONFIG += staticlib
INCLUDEPATH += ../include

SOURCES += \
    libxml2-2.7.8/catalog.c \
    libxml2-2.7.8/chvalid.c \
    libxml2-2.7.8/dict.c \
    libxml2-2.7.8/DOCBparser.c \
    libxml2-2.7.8/encoding.c \
    libxml2-2.7.8/entities.c \
    libxml2-2.7.8/error.c \
    libxml2-2.7.8/globals.c \
    libxml2-2.7.8/hash.c \
    libxml2-2.7.8/legacy.c \
    libxml2-2.7.8/list.c \
    libxml2-2.7.8/parser.c \
    libxml2-2.7.8/parserInternals.c \
    libxml2-2.7.8/pattern.c \
    libxml2-2.7.8/relaxng.c \
    libxml2-2.7.8/SAX.c \
    libxml2-2.7.8/SAX2.c \
    libxml2-2.7.8/schematron.c \
    libxml2-2.7.8/threads.c \
    libxml2-2.7.8/tree.c \
    libxml2-2.7.8/trio.c \
    libxml2-2.7.8/trionan.c \
    libxml2-2.7.8/triostr.c \
    libxml2-2.7.8/uri.c \
    libxml2-2.7.8/valid.c \
    libxml2-2.7.8/xinclude.c \
    libxml2-2.7.8/xlink.c \
    libxml2-2.7.8/xmlcatalog.c \
    libxml2-2.7.8/xmlIO.c \
    libxml2-2.7.8/xmlmemory.c \
    libxml2-2.7.8/xmlmodule.c \
    libxml2-2.7.8/xmlreader.c \
    libxml2-2.7.8/xmlsave.c \
    libxml2-2.7.8/xmlschemas.c \
    libxml2-2.7.8/xmlschemastypes.c \
    libxml2-2.7.8/xmlstring.c \
    libxml2-2.7.8/xmlwriter.c \
    libxml2-2.7.8/xpath.c \
    libxml2-2.7.8/xpointer.c

unix {
    target.path = /usr/lib
    INSTALLS += target
    $${QMAKE_COPY} $$_PRO_FILE_PWD_/libxml2-2.7.8/include/config.h $$_PRO_FILE_PWD_/libxml2-2.7.8/config.h
}

win32 {
    DESTDIR = $$_PRO_FILE_PWD_/../lib
    $${QMAKE_COPY} $$_PRO_FILE_PWD_/libxml2-2.7.8/include/win32config.h $$_PRO_FILE_PWD_/libxml2-2.7.8/config.h
}
