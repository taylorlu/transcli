#include "SourceData.h"

SourceData::SourceData(void)
{
	mType = -1;
	mDataSize = 0;
	mTimeStamp = 0;
	mDataPos = 0;
	mDts = 0;
}

SourceData::SourceData( unsigned int type,unsigned int datasize /*= 0*/,double timstamp /*= 0*/,unsigned long long datapos /*= 0*/,unsigned int dts /*= 0*/ )
{
	mType = type;
	mDataSize = datasize;
	mTimeStamp = 0;
	mDataPos = datapos;
	mDts = dts;
}

SourceData::~SourceData(void)
{

}
