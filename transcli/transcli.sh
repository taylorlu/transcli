#!/bin/bash

#set -x

APPNAME=`echo $0 | sed -e 's|.*/\(.*\)|\1|' -e 's|\(.*\)\.sh|\1|'`

HOST=`gcc -dumpmachine | grep x86_64`

TRANSCODER_BINDIR=`dirname "$0"`

#absolute path
TMP=`echo "X${TRANSCODER_BINDIR}" | grep "^X/"`
if [ "X${TMP}" == "X" ]; then
    TRANSCODER_BINDIR="${PWD}/${TRANSCODER_BINDIR}"
fi

TRANSCODER_BINDIR=`readlink -f ${TRANSCODER_BINDIR}`

EXTRA_LIB_PATH=`readlink -f "${TRANSCODER_BINDIR}/../lib"`
EXTRA_LIB_PATH_X64=`readlink -f "${TRANSCODER_BINDIR}/../lib64"`

if [ "X${LD_LIBRARY_PATH}" == "X" ]; then
	LD_LIBRARY_PATH=${EXTRA_LIB_PATH}
else
	if [ "X`echo ${LD_LIBRARY_PATH}|grep ${EXTRA_LIB_PATH}`" == "X" ]; then
		LD_LIBRARY_PATH=${EXTRA_LIB_PATH}:${LD_LIBRARY_PATH}
	fi
fi

if [ "X${HOST}" != "X" ]; then
#x84_64
	if [ "X`echo ${LD_LIBRARY_PATH}|grep ${EXTRA_LIB_PATH_X64}`" == "X" ]; then
		LD_LIBRARY_PATH=${EXTRA_LIB_PATH_X64}:${LD_LIBRARY_PATH}
	fi
fi

export TRANSCODER_BINDIR=${TRANSCODER_BINDIR}
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}

#set +x

#exec
exec ${TRANSCODER_BINDIR}/${APPNAME} "$@"

