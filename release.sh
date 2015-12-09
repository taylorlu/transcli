#Package transcli for redhat
SVN_REVISION=`svn info | grep 'Last Changed Rev:' | awk '{print $4}'`
LSB_RLS=`lsb_release -r|awk -F':\t' '{print $2}'`

mkdir release_redhat
cd release_redhat
mkdir transcli
cp -f ../bin/x64_linux/transcli ./transcli
cp -f ../bin/x64_linux/mcnt.xml ./transcli
cp -rf ../bin/x64_linux/preset ./transcli
cp -rf ../bin/x64_linux/APPSUBFONTDIR ./transcli
cd transcli
mkdir codecs tools muxer 
cd ..
cp -r ../bin/x64_linux/codecs/ffmpeg ./transcli/codecs
cp -r ../bin/x64_linux/codecs/ffprobe ./transcli/codecs
cp -rf ../bin/x64_linux/codecs/fonts ./transcli/codecs

cp -r ../bin/x64_linux/tools/mediainfo ./transcli/tools
cp -r ../bin/x64_linux/tools/mp42ts ./transcli/tools
cp -r ../bin/x64_linux/tools/MP4Box ./transcli/tools
cp -r ../bin/x64_linux/tools/yamdi ./transcli/tools
cp -r ../bin/x64_linux/muxer/ffmpeg ./transcli/muxer

tar -zcvf transcli_r${SVN_REVISION}_RHEL.${LSB_RLS}_`date "+%Y%m%d"`.tar.gz --exclude=.svn --exclude=.git transcli
