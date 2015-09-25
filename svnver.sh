#!/bin/bash
#Descripion: Auto print SVN revisions. For windows mainly. 
#   On linux, svn would updated automatic by 'transcli/transcli.pro'
#Author: jfzheng <jianfengzheng@pptv.com>

UPDATE_FILE=0
VERSION_HDR='include/version.h'

#reset the args
ARGS=`getopt -o hu --long help,update -n svnver -- "$@"`
if [ $? -ne 0 ]; then
    echo "bad arguments"
    exit 1
fi
eval set -- $ARGS

print_help() {
	echo "
    -h | --help )       show help
    -u | --update )     update \"${VERSION_HDR}\"
	"
}

#parse args
while true; do
    case $1 in
    -h | --help )       print_help;     shift;  exit 1;;
    -u | --update )     UPDATE_FILE=1;  shift;;
    -- )                                shift;  break;;
    * ) echo "unrecognized option '$1'";        exit 1;;
    esac
done

get_transcli_ver () {
    SVN_REVISION=`svn info | grep 'Last Changed Rev:' | awk '{print $4}'`
    SVN_DATE=`svn info | grep 'Last Changed Date:' | awk '{print $4,$5}'`
    SVN_NOW=`date -u +'%F %T'`
    #echo "-D__SVN_REVISION_H__ -D- -D"
    echo "#ifndef __SVN_REVISION_H__"
    echo "#define __SVN_REVISION_H__"
    echo "#define SVN_REVISION \"${SVN_REVISION}\""
    echo "#define SVN_DATE \"${SVN_DATE}\""
    echo "#define SVN_NOW \"${SVN_NOW} (UTC)\""
    echo "#endif __SVN_REVISION_H__"
}

if [ ${UPDATE_FILE} -ne 0 ]; then
    get_transcli_ver > ${VERSION_HDR}
    echo "================================"
    echo "    ${VERSION_HDR}"
    echo "================================"
    cat ${VERSION_HDR}
else
    get_transcli_ver
fi