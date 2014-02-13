#Package transcli for redhat
mkdir release_redhat
cd release_redhat
mkdir transcli
cp -f ../bin/transcli ./transcli
cp -f ../bin/mcnt.xml ./transcli
cp -rf ../bin/preset ./transcli
cp -rf ../bin/APPSUBFONTDIR ./transcli
cd transcli
mkdir codecs tools
cd ..
cp -r ../bin/codecs/ffmpeg ./transcli/codecs
cp -r ../bin/codecs/ffprobe ./transcli/codecs
cp -rf ../bin/codecs/fonts ./transcli/codecs

cp -r ../bin/tools/mediainfo ./transcli/tools
cp -r ../bin/tools/mp42ts ./transcli/tools
cp -r ../bin/tools/MP4Box ./transcli/tools
cp -r ../bin/tools/yamdi ./transcli/tools

tar -zcvf transcli_redhat_`date "+%Y%m%d"`.tar.gz --exclude=.svn --exclude=.git transcli
