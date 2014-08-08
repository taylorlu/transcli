#ifndef __AUDIO_SOURCE_DATA_H
#define __AUDIO_SOURCE_DATA_H

#include "SourceData.h"

class AudioSourceData : public SourceData
{
public:
	AudioSourceData(unsigned int datasize = 0,unsigned long long datapos = 0,double timestamp = 0);
	~AudioSourceData(void);

public:
};

#endif