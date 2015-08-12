#!/bin/bash
#Descripion: Auto print SVN revisions
#Author: jfzheng <jianfengzheng@pptv.com>

SVN_REVISION=`svn info | grep 'Last Changed Rev:' | awk '{print $4}'`
SVN_DATE=`svn info | grep 'Last Changed Date:' | awk '{print $4,$5}'`
SVN_NOW=`date -u +'%F %T'`
#echo "-D__SVN_REVISION_H__ -D- -D"
echo "#ifndef __SVN_REVISION_H__"
echo "#define __SVN_REVISION_H__"
echo "#define SVN_REVISION \"${SVN_REVISION}\""
echo "#define SVN_DATE \"${SVN_DATE}\""
echo "#define SVN_NOW \"${SVN_NOW}\""
echo "#endif __SVN_REVISION_H__"
