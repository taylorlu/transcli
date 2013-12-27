TEMPLATE = subdirs

SUBDIRS += transcli \
    common \
    strpro \
    libxml2 \
    watermark \
    faac \
    lame \
    transnode \
    libsamplerate

transcli.depends = common strpro libxml2 watermark faac lame transnode
