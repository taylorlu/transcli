#include "AudioParse.h"
#include <time.h>

AudioParse::AudioParse(void)
{
	mFile = NULL;
	mConfig = NULL;
	mConfigSize = 0;
	mSampleRate = 0;
	mTempfile = NULL;
	mFileLen = 0;
}

AudioParse::~AudioParse(void)
{
	Clear();
}


void AudioParse::Clear()
{
	mAudios.clear();
	if (mFile)
	{
		fclose(mFile);
		mFile = NULL;
	}
	if (mConfig)
	{
		delete[] mConfig;
		mConfig = NULL;
	}
	mConfigSize = 0;
	mSampleRate = 0;
	if (mTempfile)
	{
		remove(mTempfile);
		delete[] mTempfile;
		mTempfile = NULL;
	}
}

uint8_t* AudioParse::GetConfig()
{
	if (mConfigSize && mConfig) {
		return mConfig;
	}
	return NULL;
}

uint32_t AudioParse::GetConfigLength()
{
	return mConfigSize;
}

uint32_t AudioParse::Size()
{
	return mAudios.size();
}

bool AudioParse::GetDataByIndex( uint32_t index,char* data, uint32_t &size )
{
	if (index >= mAudios.size())
	{
		return false;
	}

	AudioSourceData src = mAudios[index];
	if (!data || size < src.mDataSize)
	{
		return false;
	}
	size = src.mDataSize;
	fseek(mFile,src.mDataPos,SEEK_SET);
	fread(data,size,1,mFile);
	return true;
}

double AudioParse::GetPTSByIndex( uint32_t index )
{
	if (mAudios.size() == 0)
	{
		return 0;
	}
	if (index >= mAudios.size())
	{
		return mAudios[mAudios.size() - 1].mTimeStamp;
	}

	return mAudios[index].mTimeStamp;
}

uint32_t AudioParse::GetSampleRate()
{
	return mSampleRate;
}

uint32_t AudioParse::GetSizeByIndex( uint32_t index )
{
	if (index >= mAudios.size())
	{
		return 0;
	}

	return mAudios[index].mDataSize;
}