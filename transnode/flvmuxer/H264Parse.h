#ifndef __H264_PARSE_H
#define __H264_PARSE_H

#include "VideoSourceData.h"
#include <vector>
using namespace std;

class H264Parse
{
public:
	H264Parse(void);
	~H264Parse(void);

public:
	bool Init(const char* tempfile);
	bool Parse264File(const char *file);
	bool FinishParsing();
	void Clear();
	char* GetConfig();
	unsigned int GetConfigLength();
	unsigned int Size();
	unsigned int GetSizeByIndex(unsigned int index,unsigned int sizelength = 4);
	bool GetDataByIndex(unsigned int index,char* data,unsigned int &size,unsigned int sizelength = 4);
	double GetPTSByIndex(unsigned int index);
	double GetFrameDuration();
	double GetFrameRate();
	bool IsKeyFrame(unsigned int index);
	unsigned long long GetFileLength();
	double GetPtsByFilePos(unsigned long long pos);
	char* GetSPS();
	char* GetPPS();
	char* GetVPS();
	unsigned short GetSPSSize();
	unsigned short GetPPSSize();
	unsigned short GetVPSSize();

private:
	VideoSourceData* ParseNal(char *data,unsigned int size);

private:
	vector<VideoSourceData*> mVideos;

	char *mPPS;
	char *mSPS;
	char *mVPS;

	unsigned short mPPSSize;
	unsigned short mSPSSize;
	unsigned short mVPSSize;

	FILE *mFile;

	double mVideoFrameRate;
	double mVideoFrameDuration;

	unsigned int mFlg;

	bool mIsMultiSlice;
	vector<unsigned int> mVideoHeadIndexs;

public:
	vector<double> mKeyFrameLocation;
	vector<double> mKeyFrameTimes;
	vector<unsigned int> mKeyFrameIndex;

	unsigned int mWidth;
	unsigned int mHeight;
};

#endif