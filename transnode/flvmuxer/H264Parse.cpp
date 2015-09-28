#include "H264Parse.h"
#include "DecodeConfig.h"
#include "CommonDef.h"
#include <stdio.h>
#include <string.h>
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#define min(a,b)            (((a) < (b)) ? (a) : (b))

H264Parse::H264Parse(void)
{
	mPPS = NULL;
	mSPS = NULL;
	mVPS = NULL;
	mFile = NULL;
	mVideoFrameDuration = 0;
	mVideoFrameRate = 0;
	mWidth = 0;
	mHeight = 0;
	mSPSSize = 0;
	mPPSSize = 0;
	mVPSSize = 0;
	mFlg = 0;
	mIsMultiSlice  = false;
}

H264Parse::~H264Parse(void)
{
	Clear();
}

void H264Parse::Clear()
{
	if (mPPS)
	{
		delete[] mPPS;
		mPPS = NULL;
	}
	if (mSPS)
	{
		delete[] mSPS;
		mSPS = NULL;
	}
	if (mVPS)
	{
		delete[] mVPS;
		mVPS = NULL;
	}
	if (mFile)
	{
		fclose(mFile);
		mFile = NULL;
	}
	while(mVideos.size())
	{
		VideoSourceData *src = mVideos[mVideos.size() - 1];
		mVideos.pop_back();
		delete src;
		src = NULL;
	}
	mVideoFrameDuration = 0;
	mVideoFrameRate = 0;
	mWidth = 0;
	mHeight = 0;
	mSPSSize = 0;
	mPPSSize = 0;
	mVPSSize = 0;
	mFlg = 0;
	mIsMultiSlice  = false;
	mKeyFrameTimes.clear();
	mKeyFrameLocation.clear();
	mKeyFrameIndex.clear();
}

bool H264Parse::Init(const char* tempfile)
{
	Clear();

	mFile = fopen(tempfile,"wb+");
	if (!mFile)
	{
		return false;
	}
	return true;
}

bool H264Parse::Parse264File(const char *file )
{
	Clear();

	if (!file)
	{
		return false;
	}

	mFile = fopen(file,"rb+");
	if (!mFile)
	{
		return false;
	}

	fseek(mFile,0,SEEK_END);
	unsigned long long filelength = ftell(mFile);
	fseek(mFile,0,SEEK_SET);

	unsigned int datasize = 1024 * 1024;
	char *data = new char[datasize];
	unsigned long long prenal = 0;
	unsigned long long curnal = 0;
	unsigned int flg = 0;
	unsigned long long offset = 0;

	while(offset < filelength)
	{
		unsigned int currentsize = datasize;
		if (currentsize > filelength - offset)
		{
			currentsize = filelength - offset;
		}
		fseek(mFile,offset,SEEK_SET);
		fread(data,currentsize,1,mFile);
		for (int i = 0; i < currentsize; i ++)
		{
			flg <<= 8;
			flg |= data[i] & 0xff;
			if (flg == 1)
			{
				curnal = offset + i + 1;
				VideoSourceData *src = ParseNal(data + prenal - offset,curnal - prenal - 4);
				if (src)
				{
					src->mDataPos = prenal;
					mVideos.push_back(src);
				}
				prenal = curnal;
			}
			else
			{
				flg &= 0xffffff;
				if (flg == 1)
				{
					curnal = offset + i + 1;
					VideoSourceData *src = ParseNal(data + prenal - offset,curnal - prenal - 3);
					if (src)
					{
						src->mDataPos = prenal;
						mVideos.push_back(src);
					}
					prenal = curnal;
				}
			}
		}
		if (offset + currentsize >= filelength)
		{
			break;
		}
		offset = prenal;
	}

	VideoSourceData *src = ParseNal(data + prenal - offset,filelength - prenal);
	if (src)
	{
		src->mDataPos = prenal;
		mVideos.push_back(src);
	}

	delete[] data;

	return true;
}

VideoSourceData* H264Parse::ParseNal( char *data,unsigned int size )
{
	if (data == NULL || size == 0)
	{
		return NULL;
	}

	unsigned char naltype = data[0];
	unsigned char nal = naltype & 0x1f;
	unsigned char nal_ref = (naltype & 0x60) >> 5;
	switch(nal)
	{
	case 7:
		{
			if (mSPS)
			{
				return NULL;
			}
			ConfigDecoder config;
			config.read_config_sps((unsigned char*)data,size);
			mVideoFrameRate = (double)(config.m_sSPS.time_scale) / config.m_sSPS.num_units_in_tick / 2;
			mVideoFrameRate = (int)(mVideoFrameRate * 10000 + 0.5) / 10000.0f;
			mVideoFrameDuration = 1000 / mVideoFrameRate;
			if (config.m_sSPS.frame_crop_right_offset == 0xffffffff)
			{
				config.m_sSPS.frame_crop_right_offset = 0;
			}
			if (config.m_sSPS.frame_crop_bottom_offset == 0xffffffff)
			{
				config.m_sSPS.frame_crop_bottom_offset = 0;
			}
			mWidth = 16 * (config.m_sSPS.pic_width_in_mbs_minus1 + 1) - 2 * min(config.m_sSPS.frame_crop_right_offset, 7);
			if (config.m_sSPS.frame_mbs_only_flag)
			{
				mHeight = 16 * (config.m_sSPS.pic_height_in_map_units_minus1 + 1) - 2 * min(config.m_sSPS.frame_crop_bottom_offset/* / 2*/,7);
			}
			else
			{
				mHeight = 16 * (config.m_sSPS.pic_height_in_map_units_minus1 + 1) - 4 * min(config.m_sSPS.frame_crop_bottom_offset/* / 2*/,3);
			}
			mSPS = new char[size];
			memcpy(mSPS,data,size);
			mSPSSize = size;
			return NULL;
		}
	case 8:
		{
			if (mPPS)
			{
				return NULL;
			}
			mPPS = new char[size];
			memcpy(mPPS,data,size);
			mPPSSize = size;
			return NULL;
		}
	case 5:
		{
			unsigned char flg = data[1];
			bool headerslice = false;
			if (flg & 0x80)
			{
				headerslice = true;
			}
			VideoSourceData *src = new VideoSourceData(size,0,headerslice,true);
			return src;
		}
	case 1:
		{
			unsigned char flg = data[1];
			bool headerslice = false;
			if (flg & 0x80)
			{
				headerslice = true;
			}
			VideoSourceData *src = new VideoSourceData(size,0,headerslice);
			return src;
		}
	default:
		{
			return NULL;
		}
	}
	return NULL;
}

bool H264Parse::FinishParsing()
{
	if (mVideos.size())
	{
		if (mVideos[mVideos.size() - 1]->mTimeStamp > 0)
		{
			return true;
		}
	}
	else
	{
		return false;
	}
	int loopcount = mVideos.size() > 10 ? 10 : mVideos.size();
	for (int i = 0; i < loopcount; i ++)
	{
		if (mVideos[i]->mIsKeyFrame)
		{
			if (mVideos[i]->mIsHeadSlice)
			{
				mIsMultiSlice = true;
			}
			break;
		}
	}

	double videopts = 0;
	for (int i = 0; i < mVideos.size(); i ++)
	{
		mVideos[i]->mTimeStamp = videopts;
		if (mIsMultiSlice)
		{
			if (mVideos[i]->mIsHeadSlice)
			{
				mVideoHeadIndexs.push_back(i);
				if (mVideos[i]->mIsKeyFrame)
				{
					mKeyFrameTimes.push_back(0);
					mKeyFrameLocation.push_back(0);
					mKeyFrameIndex.push_back(mVideoHeadIndexs.size());
				}
				videopts += mVideoFrameDuration;
			}
		}
		else
		{
			mVideoHeadIndexs.push_back(i);
			if (mVideos[i]->mIsKeyFrame)
			{
				mKeyFrameTimes.push_back(0);
				mKeyFrameLocation.push_back(0);
				mKeyFrameIndex.push_back(mVideoHeadIndexs.size());
			}
			videopts += mVideoFrameDuration;
		}
	}
	return true;
}

char* H264Parse::GetConfig()
{
	if (!mSPS || !mPPS)
	{
		return NULL;
	}

	char* config = new char[GetConfigLength()];
	config[0] = 0x01;
	config[1] = mSPS[1];
	config[2] = mSPS[2];
	config[3] = mSPS[3];
	config[4] = 0xff;
	config[5] = 0xe1;
	config[6] = (mSPSSize & 0xff00) >> 8;
	config[7] = (mSPSSize & 0xff);
	memcpy(config + 8,mSPS,mSPSSize);
	config[8 + mSPSSize] = 0x01;
	config[9 + mSPSSize] = (mPPSSize & 0xff00) >> 8;
	config[10 + mSPSSize] = (mPPSSize & 0xff);
	memcpy(config + 11 + mSPSSize,mPPS,mPPSSize);

	return config;
}

unsigned int H264Parse::GetConfigLength()
{
	return 11 + mSPSSize + mPPSSize;
}

unsigned int H264Parse::Size()
{
	return mVideoHeadIndexs.size();
}

bool H264Parse::GetDataByIndex( unsigned int index,char* data,unsigned int &size,unsigned int sizelength )
{
	if (index >= mVideoHeadIndexs.size())
	{
		return false;
	}

	if (mIsMultiSlice)
	{
		unsigned int count = 1;
		unsigned int sliceindex = mVideoHeadIndexs[index];
		unsigned int totalcount = mVideos[sliceindex]->mDataSize + sizelength;
		for (int i = sliceindex + 1; i < mVideos.size(); i ++)
		{
			VideoSourceData src = *(mVideos[i]);
			if (src.mIsHeadSlice)
			{
				break;
			}
			totalcount += src.mDataSize + sizelength;
			count ++;
		}
		unsigned int offset = 0;
		
		if (!data || size < totalcount)
		{
			return false;
		}
		size = totalcount;
		for (int i = sliceindex; i < sliceindex + count; i ++)
		{
			VideoSourceData src = *(mVideos[i]);
			for (int j = 0; j < sizelength; j ++)
			{
				data[offset ++] = ((src.mDataSize) >> ((sizelength - j - 1) * 8)) & 0xff;
			}
			fseek(mFile,src.mDataPos,SEEK_SET);
			fread(data + offset,src.mDataSize,1,mFile);
			offset += src.mDataSize;
		}
		return true;
	}
	else
	{
		VideoSourceData src = *(mVideos[index]);
		unsigned int seisize = 0;
		if (!data || size < src.mDataSize + sizelength + seisize)
		{
			return false;
		}
		fseek(mFile,src.mDataPos,SEEK_SET);
		size = src.mDataSize + sizelength;
		for (int i = 0; i < sizelength; i ++)
		{
			data[i + seisize] = ((src.mDataSize) >> ((sizelength - i - 1) * 8)) & 0xff;
		}
		fread(data + sizelength + seisize,src.mDataSize,1,mFile);
		
		return true;
	}
}

double H264Parse::GetFrameDuration()
{
	return mVideoFrameDuration;
}

bool H264Parse::IsKeyFrame( unsigned int index )
{
	if (index >= mVideoHeadIndexs.size())
	{
		return false;
	}
	unsigned int sliceindex = mVideoHeadIndexs[index];
	return mVideos[sliceindex]->mIsKeyFrame;
}

double H264Parse::GetFrameRate()
{
	return mVideoFrameRate;
}

double H264Parse::GetPTSByIndex( unsigned int index )
{
	if (index >= mVideoHeadIndexs.size())
	{
		return mVideos[mVideos.size() - 1]->mTimeStamp;
	}
	unsigned int sliceindex = mVideoHeadIndexs[index];
	return mVideos[sliceindex]->mTimeStamp;
}

unsigned int H264Parse::GetSizeByIndex( unsigned int index,unsigned int sizelength /*= 4*/ )
{
	if (index >= mVideoHeadIndexs.size())
	{
		return 0;
	}

	if (mIsMultiSlice)
	{
		unsigned int count = 1;
		unsigned int sliceindex = mVideoHeadIndexs[index];
		unsigned int totalcount = mVideos[sliceindex]->mDataSize + sizelength;
		for (int i = sliceindex + 1; i < mVideos.size(); i ++)
		{
			VideoSourceData src = *(mVideos[i]);
			if (src.mIsHeadSlice)
			{
				break;
			}
			totalcount += src.mDataSize + sizelength;
			count ++;
		}
		return totalcount;
	}
	else
	{
		return mVideos[index]->mDataSize + sizelength;
	}
}

unsigned long long H264Parse::GetFileLength()
{
	if (!mFile)
	{
		return 0;
	}	
	unsigned long long current = ftell(mFile);
	fseek(mFile,0,SEEK_END);
	unsigned long long ret = ftell(mFile);
	fseek(mFile,current,SEEK_SET);
	return ret;
}

double H264Parse::GetPtsByFilePos( unsigned long long pos )
{
	if (mVideos.size())
	{
		if (pos > mVideos[mVideos.size() - 1]->mDataPos)
		{
			return mVideos[mVideos.size() - 1]->mTimeStamp;
		}
	}
	else
	{
		return 0;
	}
	for (int i = 0; i < mVideos.size(); i ++)
	{
		if (mVideos[i]->mDataPos >= pos)
		{
			return mVideos[i]->mTimeStamp;
		}
	}
	return 0;
}

char* H264Parse::GetSPS()
{
	return mSPS;
}

char* H264Parse::GetPPS()
{
	return mPPS;
}

char* H264Parse::GetVPS()
{
	return mVPS;
}

unsigned short H264Parse::GetSPSSize()
{
	return mSPSSize;
}

unsigned short H264Parse::GetPPSSize()
{
	return mPPSSize;
}

unsigned short H264Parse::GetVPSSize()
{
	return mVPSSize;
}
