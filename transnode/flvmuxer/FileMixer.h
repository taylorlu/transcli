#pragma once
#include "AACParse.h"
#include "H264Parse.h"
#include "MetaUnit.h"
#include "FileWriter.h"

#define	H265_TYPE_VER_10  0x19

#if 0
enum OUTPUT_TYPE{
	OUTPUT_TYPE_MKV,
	OUTPUT_TYPE_MP4,
	OUTPUT_TYPE_FLV
};
#endif

class FileMixer
{
public:
	FileMixer(void);
	~FileMixer(void);

private:
	H264Parse mVParser;
	AACParse mAParser;

	FileWriter mFile;

	//flv
	char *mFileVersion;
	double mDuration;
	double mBitRate;
	double mFileLength;
	double mMp4Boxpos;
	double mMp4Headerpos;
	double mSlimmp4HeaderSize;
	double mMp4HeadLen;

	vector<unsigned long long> mFrameOffset;
	vector<unsigned char> mFrameType;
	vector<unsigned int> mKeyFrameIndex;
	vector<unsigned short> mVideoDelta;
	double mVideoDuration;
	double mAudioDuration;

	double mLastKeyFrameTimeStamp;
	double mLastKeyFrameLocation;
	vector<vector<double>> mKeyFrames;

	unsigned long long mVideoStcoPos;
	unsigned long long mAudioStcoPos;

	char *mOutputFile;

public:
	bool ParseAACFile(char* file);
	bool ParseAACFiles(vector<char*> files);//,vector<double> videopts);
	bool ParseADTS(char* file);
	bool Parse264File(char* file);
	bool WriteOutPutFile(char* file,int outputtype = 1);
	bool InitTempFile(char* tempfile);

	double GetVideoPtsByFilePos(unsigned long long pos);

private:
	//flv
	bool WriteFileFlv();
	void WriteFlvHead();
	void WriteFlvMeta();
	void WriteFlvData();

	void WriteFlvVideoTag(unsigned int size,unsigned int timestamp,char* data,bool iskeyframe = false,bool isconfig = false,short dts = 0);
	void WriteFlvAudioTag(unsigned int size,unsigned int timestamp,char* data,bool isconfig = false);

	MetaUnit GetMetaString(const char* strtitle,const char* stringdada);
	MetaUnit GetMetaDouble(const char* strtitle,double doubledata);
	MetaUnit GetArrayData(const char* strtitle,vector<char*> subtitles,vector<vector<double> > arraydata);

	unsigned int descrLength(unsigned int len);
	void putDescr(int tag,unsigned int size);
};
