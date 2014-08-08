#ifndef __Audio_PARSE_H__
#define __Audio_PARSE_H__

#include "AudioSourceData.h"
#include <stdint.h>
#include <vector>
#include "ComDef.h"

#define AUDIO_OTI_MP4A 0x40
#define AUDIO_OTI_AC3 0xA5
#define AUDIO_OTI_EC3 0xA6

class AudioParse
{
public:
	AudioParse(void);
	virtual bool Parse(const char* esFileName) = 0;
	virtual uint32_t GetFrameLength() = 0;
	virtual uint8_t GetObjectTypeIndication() = 0;
	virtual const char* GetAudioType() = 0;

	uint8_t* GetConfig();
	uint32_t GetConfigLength();
	uint32_t Size();
	uint32_t GetSizeByIndex(uint32_t index);
	bool GetDataByIndex(uint32_t index,char* data, uint32_t &size);
	double GetPTSByIndex(uint32_t index);
	uint32_t GetSampleRate();
	uint32_t GetChannelCount() {return mChannelCount;}
	uint64_t GetFileLength() {return mFileLen;}
	void SetContainerType(int outType = OUTPUT_TYPE_FLV) {mOutputType = outType;}
	void Clear();
	~AudioParse(void);

protected:
	std::vector<AudioSourceData> mAudios;
	FILE *mFile;
	uint8_t* mConfig;
	unsigned int mConfigSize;
	unsigned int mSampleRate;
	uint32_t mChannelCount;
	char* mTempfile;
	int   mOutputType;
	uint64_t mFileLen;
};


#endif