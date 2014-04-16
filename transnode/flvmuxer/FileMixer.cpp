#include "FileMixer.h"
#include "GlobalDefine.h"
#include <time.h>

FileMixer::FileMixer(void)
{
	mFileVersion = "1.0.0.0";
	//mOutputFile = NULL;
}

FileMixer::~FileMixer(void)
{
}

bool FileMixer::ParseAACFile(const char* file )
{
	bool ret = mAParser.ParseAACFile(file);
	if (!ret)
	{
		//Trace("[Error][FileMixer]:Parse AAC File Error\n");
	}
	return ret;
}

bool FileMixer::ParseADTS(const char* file)
{
	bool ret = mAParser.ParseADTS(file);
	if (!ret)
	{

	}
	return ret;
}

bool FileMixer::Parse264File(const char* file )
{
	bool ret = mVParser.Parse264File(file);
	mVParser.FinishParsing();
	return ret;
}

bool FileMixer::WriteOutPutFile(const char* file,int outputtype )
{
	//Trace("[Info][FileMixer]:Start Write Output File\n");
	unsigned long long t = time(0);
	if (!file)
	{
		//Trace("[Error][FileMixer]:Output Path Null\n");
		return false;
	}

	//Trace("[Info][FileMixer]:Open File %s\n",file);
	mFile.Open(file);
	if (!mFile.IsOpen())
	{
		//Trace("[Error][FileMixer]:Unable To Open %s\n",file);
		return false;
	}
	//mOutputFile = file;

	//Trace("[Info][FileMixer]:Analyze Video Info\n");
	mVParser.FinishParsing();
	if (1 > mVParser.Size())
	{
		//Trace("[Error][FileMixer]:Tags of video is empty\n");
		return false;
	}
	if (1 > mAParser.Size())
	{
		//Trace("[Error][FileMixer]:Tags of audio is empty\n");
		return false;
	}
	mVideoDuration = mVParser.GetPTSByIndex(mVParser.Size() - 1);
	mAudioDuration = mAParser.GetPTSByIndex(mAParser.Size() - 1);
	//Trace("[Info][FileMixer]:Video Duration:%d,Audio Duration:%d\n",mVideoDuration,mAudioDuration);

	bool ret = true;

	//============
	//outputtype = OUTPUT_TYPE_MP4;
	//============

	switch(outputtype)
	{
	case 1:
		{
			//Trace("[Info][FileMixer]:OutPut Type Flv\n");
			ret = WriteFileFlv();
			break;
		}
	default:
		{
			//Trace("[Error][FileMixer]:Unknown Output Type %d\n",outputtype);
			return false;
		}
	}

	if (ret)
	{
		//Trace("[Info][FileMixer]:Output File Success\n");
	}
	else
	{
		//Trace("[Warning][FileMixer]:Output File Error\n");
	}
	mFile.Close();
	unsigned int tt = time(0) - t;
	//Trace("[Info][FileMixer]:Mix Used %d\n",time(0) - t);

	return ret;
}

bool FileMixer::WriteFileFlv()
{
	//flv head
	//Trace("[Info][FileMixer]:Write Flv Head\n");
	WriteFlvHead();
	//meta
	//Trace("[Info][FileMixer]:Write Flv Meta\n");
	mKeyFrames.push_back(mVParser.mKeyFrameLocation);
	mKeyFrames.push_back(mVParser.mKeyFrameTimes);
	WriteFlvMeta();

	//vidoeconfig
	//Trace("[Info][FileMixer]:Write Flv Video Config\n");
	
	char* vconfig = mVParser.GetConfig();
	if (vconfig)
	{
		WriteFlvVideoTag(mVParser.GetConfigLength(),0,vconfig,true,true);
		delete[] vconfig;
	}
	else
	{
		//Trace("[Error][FileMixer]:No Video Config\n");
		return false;
	}
	
	//audioconfig
	//Trace("[Info][FileMixer]:Write Flv Audio Config\n");
	char* aconfig = mAParser.GetConfig();
	if (aconfig)
	{
		WriteFlvAudioTag(mAParser.GetConfigLength(),0,aconfig,true);
		delete[] aconfig;
	}
	else
	{
		//Trace("[Error][FileMixer]:No Audio Config\n");
		return false;
	}

	//audio & video data
	//Trace("[Info][FileMixer]:Write Flv Audio & Video Data\n");
	WriteFlvData();

	mFileLength = mFile.Tell();
	mBitRate = mFileLength / mDuration / 1000 * 8; 
	//Trace("[Info][FileMixer]:Flv File Length:%d,Bitrate:%d\n",mFileLength,mBitRate);

	WriteFlvMeta();

	if (mFile.Error())
	{
		return false;
	}

	return true;
}

void FileMixer::WriteFlvHead()
{
	mFile.WriteBytes("FLV",3);
	mFile.Write8(1);
	mFile.Write8(5);
	mFile.Write32(9);
	mFile.Write32(0);
}

void FileMixer::WriteFlvMeta()
{
	vector<MetaUnit> metaunits;
	vector<char*> keyframesubtitle;

	keyframesubtitle.push_back("filepositions");
	keyframesubtitle.push_back("times");

	metaunits.push_back(GetMetaString("MetaCreator","PPLive"));
	metaunits.push_back(GetMetaDouble("duration",mDuration));
	metaunits.push_back(GetMetaDouble("width",mVParser.mWidth));
	metaunits.push_back(GetMetaDouble("height",mVParser.mHeight));
	metaunits.push_back(GetMetaDouble("bitrate",mBitRate));
	metaunits.push_back(GetMetaDouble("filelength",mFileLength));
	metaunits.push_back(GetMetaDouble("lastkeyframetimestamp",mLastKeyFrameTimeStamp));
	metaunits.push_back(GetMetaDouble("lastkeyframelocation",mLastKeyFrameLocation));
	metaunits.push_back(GetArrayData("keyframes",keyframesubtitle,mKeyFrames));
	
	mFile.Seek(9 + 4,SEEK_SET);
	mFile.Write8(18);
	mFile.Write24(0);
	mFile.Write32(0);
	mFile.Write24(0);

	mFile.Write8(2);
	mFile.Write16(10);
	mFile.WriteBytes("onMetaData",10);
	mFile.Write8(8);
	mFile.Write32(metaunits.size());
	for (int i = 0; i < metaunits.size(); i ++)
	{
		metaunits[i].WriteMetaUnit(mFile.Pointer());
	}
	mFile.Write24(9);
	unsigned long long pos = mFile.Tell();
	mFile.Write32(pos - 13);
	mFile.Seek(14,SEEK_SET);
	mFile.Write24(pos - 24);
	mFile.Seek(0,SEEK_END);
}

MetaUnit FileMixer::GetMetaString( const char* strtitle,const char* stringdada )
{
	MetaUnit metaunit;
	metaunit.GetStringData(strtitle,stringdada);
	return metaunit;
}

MetaUnit FileMixer::GetMetaDouble( const char* strtitle,double doubledata )
{
	MetaUnit metaunit;
	metaunit.GetDoubleData(strtitle,doubledata);
	return metaunit;
}

MetaUnit FileMixer::GetArrayData( const char* strtitle,vector<char*> subtitles,vector<vector<double> > arraydata )
{
	MetaUnit metaunit;
	metaunit.GetArrayData(strtitle,subtitles,arraydata);
	return metaunit;
}

void FileMixer::WriteFlvVideoTag( unsigned int size,unsigned int timestamp,char* data,bool iskeyframe /*= false*/,bool isconfig /*= false*/,short dts /*= 0*/ )
{
	mFile.Write8(9);
	mFile.Write24(size + 5);
	mFile.Write24((timestamp & 0xffffff));
	mFile.Write8((timestamp >> 24) & 0xff);
	mFile.Write24(0);
	
	if (iskeyframe)
	{
		mFile.Write8(0x17);
	}
	else
	{
		mFile.Write8(0x27);
	}
	
	if (isconfig)
	{
		mFile.Write8(0);
	}
	else
	{
		mFile.Write8(1);
	}
	mFile.Write24(dts);
	mFile.WriteBytes(data,size);
	mFile.Write32(size + 16);
}

void FileMixer::WriteFlvAudioTag(unsigned int size,unsigned int timestamp,char* data,bool isconfig /*= false*/ )
{
	/*if(isconfig)
	{
		mFile.Flush();

		unsigned short config = 0;

		char audioconfig[2] = "";
		memcpy(audioconfig, mAParser.GetConfig(), 2);

		mFile.Write8(8);
		mFile.Write24(4);
		mFile.Write32(0);
		mFile.Write24(0);
		mFile.Write8(0xaf);
		mFile.Write8(0);
		mFile.WriteBytes(audioconfig,2);
		mFile.Write32(15);

		return;
	}*/
	mFile.Write8(8);
	mFile.Write24(size + 2);
	mFile.Write24((timestamp & 0xffffff));
	mFile.Write8((timestamp >> 24) & 0xff);
	mFile.Write24(0);
	mFile.Write8(0xaf);
	if (isconfig)
	{
		mFile.Write8(0);
	}
	else
	{
		mFile.Write8(1);
	}
	mFile.WriteBytes(data,size);
	mFile.Write32(size + 13);
}

void FileMixer::WriteFlvData()
{
	unsigned int videoindex = 0;
	unsigned int audioindex = 0;

	unsigned int videopts = 0;
	unsigned int audiopts = 0;

	unsigned int keyframeindex = 0;

	char *data = new char[1024 * 1024];
	//Trace("[Info][FileMixer]:Audio Count:%d,Video Count:%d\n",mVParser.Size(),mAParser.Size());

	while(videoindex < mVParser.Size() || audioindex < mAParser.Size())
	{
		if (videoindex >= mVParser.Size())//video over write audio
		{
			unsigned int size = 1024 * 1024;
			bool ret = mAParser.GetDataByIndex(audioindex,data,size);
			if (ret)
			{
				mFrameOffset.push_back((unsigned long long)(mFile.Tell()) + 13);
				mFrameType.push_back(1);
				WriteFlvAudioTag(size,audiopts,data);
			}
			audioindex ++;
			audiopts = mAParser.GetPTSByIndex(audioindex);
		}
		else
		{
			if (audioindex >= mAParser.Size())//audio over write video
			{
				unsigned int size = 1024 * 1024;
				bool ret = mVParser.GetDataByIndex(videoindex,data,size);
				bool iskeyframe = mVParser.IsKeyFrame(videoindex);
				if (iskeyframe)
				{
					mKeyFrames[0][keyframeindex] = mFile.Tell();
					mKeyFrames[1][keyframeindex ++] = videopts / 1000.;
					mKeyFrameIndex.push_back(videoindex + 1);
				}
				if (ret)
				{
					mFrameOffset.push_back((unsigned long long)(mFile.Tell()) + 16);
					mFrameType.push_back(0);
					mVideoDelta.push_back(0);
					WriteFlvVideoTag(size,videopts,data,iskeyframe);
				}
				videoindex ++;
				videopts = mVParser.GetPTSByIndex(videoindex);
			}
			else
			{
				if (audiopts > videopts)//write video
				{
					unsigned int size = 1024 * 1024;
					bool ret = mVParser.GetDataByIndex(videoindex,data,size);
					bool iskeyframe = mVParser.IsKeyFrame(videoindex);
					if (iskeyframe)
					{
						mKeyFrames[0][keyframeindex] = mFile.Tell();
						mKeyFrames[1][keyframeindex ++] = videopts / 1000.;
						mKeyFrameIndex.push_back(videoindex + 1);
					}
					if (ret)
					{
						mFrameOffset.push_back((unsigned long long)(mFile.Tell()) + 16);
						mFrameType.push_back(0);
						mVideoDelta.push_back(0);
						WriteFlvVideoTag(size,videopts,data,iskeyframe);
					}
					videoindex ++;
					videopts = mVParser.GetPTSByIndex(videoindex);
				}
				else//write audio
				{
					unsigned int size = 1024 * 1024;
					bool ret = mAParser.GetDataByIndex(audioindex,data,size);
					if (ret)
					{
						mFrameOffset.push_back((unsigned long long)(mFile.Tell()) + 13);
						mFrameType.push_back(1);
						WriteFlvAudioTag(size,audiopts,data);
					}
					audioindex ++;
					audiopts = mAParser.GetPTSByIndex(audioindex);
				}
			}
		}
	}

	delete[] data;

	mVideoDuration = videopts;
	mAudioDuration = audiopts;
	mDuration = mVideoDuration > mAudioDuration ? mVideoDuration : mAudioDuration;
	mDuration /= 1000;
	if (mKeyFrames[0].size())
	{
		mLastKeyFrameLocation = mKeyFrames[0][mKeyFrames[0].size() - 1];
	}
	else
	{
		mLastKeyFrameLocation = 0;
	}
	if (mKeyFrames[1].size())
	{
		mLastKeyFrameTimeStamp = mKeyFrames[1][mKeyFrames[1].size() - 1];
	}
	else
	{
		mLastKeyFrameTimeStamp = 0;
	}
}

bool FileMixer::InitTempFile(const char* tempfile )
{
	return mVParser.Init(tempfile);
}

double FileMixer::GetVideoPtsByFilePos( unsigned long long pos )
{
	return mVParser.GetPtsByFilePos(pos);
}

unsigned int FileMixer::descrLength( unsigned int len )
{
	int i;
	for(i=1; len>>(7*i); i++);
	return len + 1 + i;
}

void FileMixer::putDescr( int tag,unsigned int size )
{
	int i= descrLength(size) - size - 2;
	mFile.Write8(tag);
	for(; i>0; i--)
		mFile.Write8((size>>(7*i)) | 0x80);
	mFile.Write8(size & 0x7F);
}
