#include "AudioSourceData.h"

AudioSourceData::AudioSourceData(void)
{
	::SourceData();
	mType = SRCTYPE_AUDIO;
}

AudioSourceData::AudioSourceData( unsigned int datasize /*= 0*/,unsigned long long datapos /*= 0*/,double timestamp /*= 0*/ ):SourceData(SRCTYPE_AUDIO,datasize,timestamp,datapos)
{

}

AudioSourceData::~AudioSourceData(void)
{

}
