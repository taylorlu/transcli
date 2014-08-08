#ifndef __VIDEO_SOURCE_DATA_H
#define __VIDEO_SOURCE_DATA_H

#include "SourceData.h"

class VideoSourceData : public SourceData
{
public:
	VideoSourceData(unsigned int datasize = 0,double timestamp = 0,bool isheadslice = false,bool iskeyframe = false,unsigned long long datapos = 0,short dts = 0,bool issei = false);
	~VideoSourceData(void);

public:
	bool mIsKeyFrame;
	bool mIsHeadSlice;
	bool mIsSei;
};


#endif