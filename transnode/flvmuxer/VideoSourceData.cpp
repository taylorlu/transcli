#include "VideoSourceData.h"

VideoSourceData::VideoSourceData(void)
{
	::SourceData();
	mType = SRCTYPE_VIDEO;
	mDts = 0;
	mIsKeyFrame = false;
	mIsHeadSlice = false;
	mIsSei = false;
}

VideoSourceData::VideoSourceData( unsigned int datasize /*= 0*/,double timestamp /*= 0*/,bool isheadslice /*= false*/,bool iskeyframe /*= false*/,unsigned long long datapos /*= 0*/,short dts /*= 0*/,bool issei /*= false*/ ):SourceData(SRCTYPE_VIDEO,datasize,timestamp,datapos,dts)
{
	//::SourceData(SRCTYPE_VIDEO,datasize,datapos);
	mIsKeyFrame = iskeyframe;
	mIsHeadSlice = isheadslice;
	mIsSei = issei;
}

VideoSourceData::~VideoSourceData(void)
{

}
