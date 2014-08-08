#ifndef __SOURCE_DATA_H
#define __SOURCE_DATA_H

#include <stdio.h>

#ifdef WIN32
#define fseek _fseeki64
#define ftell _ftelli64
#else
#define _FILE_OFFSET_BITS 64
#define __USE_FILE_OFFSET64
#define __USE_LARGEFILE64
#define _LARGEFILE64_SOURCE
#include <stdio.h>
#define fseek fseeko64
#define fell ftello64
#endif

enum SrcDatType
{
	SRCTYPE_AUDIO,
	SRCTYPE_VIDEO,
};

class SourceData
{
public:
	SourceData(unsigned int type,unsigned int datasize = 0,double timstamp = 0,unsigned long long datapos = 0,unsigned int dts = 0);
	~SourceData(void);

public:
	unsigned int mType;
	unsigned int mDataSize;
	double mTimeStamp;
	unsigned long long mDataPos;
	unsigned int mDts;
	char *mData;
};

#endif