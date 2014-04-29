#include "AACParse.h"
#include "Box.h"
#include <time.h>
static int frequencies[] = { 96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350};
AACParse::AACParse(void)
{
	mFile = NULL;
	mConfig = NULL;
	mConfigSize = 0;
	mSampleRate = 0;
	mTempfile = NULL;
}

AACParse::~AACParse(void)
{
	Clear();
}

bool AACParse::ParseAACFile(const char* filepath )
{
	//unsigned long long t = time(0);
	Clear();
	if (!filepath)
	{
		return false;
	}

	mFile = fopen(filepath,"rb+");
	if (!mFile)
	{
		return false;
	}

	if (!analyze_mp4head(mFile))
	{
		mAudios.clear();
		fclose(mFile);
		return false;
	}

	if (mAudios.size())
	{
		mAudios[0].mTimeStamp =0;
		for (int i = 1; i < mAudios.size(); i ++)
		{
			mAudios[i].mTimeStamp += mAudios[i - 1].mTimeStamp;
		}
	}
	return true;
}

int AACParse::Get_One_ADTS_Frame(unsigned char* buffer, size_t buf_size, unsigned char* data ,size_t* data_size)
{
	size_t size = 0;  
  
    if(!buffer /*|| !data*/ || !data_size )  
    {  
        return -1;  
    }  
  
	//static int i = 0;
    while(1)  
    {  
        if(buf_size  < 7 )  
        {  
            return -1;  
        }  
  
        if((buffer[0] == 0xff) && ((buffer[1] & 0xf0) == 0xf0) )  
        {  
            size |= ((buffer[3] & 0x03) <<11);     //high 2 bit  
            size |= buffer[4]<<3;                //middle 8 bit  
            size |= ((buffer[5] & 0xe0)>>5);        //low 3bit  
			/*printf("adts frame = %d\n", ++i);
			printf("frame size = %d\n", size);*/
            break;  
        }  
		
        --buf_size;  
        ++buffer;  
    }  
  
	if(buf_size < size)  
	{  
		return -1;  
	}  

	memcpy(data, buffer, size);  
    *data_size = size;  
      
    return 0;  
}

#define BUFFER_MAX_LEN 1024*1024
#define FRAME_MAX_LEN 1024*1024

bool AACParse::ParseADTS(const char* filepath, bool bmp4)
{
	if (!filepath)
	{
		return false;
	}
	mFile = fopen(filepath, "rb");
	if (!mFile)
	{
		return false;
	}
	fseek(mFile,0,SEEK_END);
	unsigned long long filelength = ftell(mFile);
	fseek(mFile,0,SEEK_SET);
	
	unsigned char* data = new unsigned char[BUFFER_MAX_LEN];
	unsigned long long preadts = 0;
	unsigned long long curadts = -1;
	unsigned short int flg = 0;
	unsigned long long offset = 0;
	unsigned long long proleft = 0;
	unsigned long long data_size = 0;
	bool bGot = false;
	fseek(mFile, offset, SEEK_SET);
	
	unsigned char * input_data = data;
	unsigned char *frame = new unsigned char[FRAME_MAX_LEN];
	unsigned char *tempbuffer = new unsigned char[1024 * 1024];
	size_t size = 0; 
	int curpos = 0;
	int totalPos = 0;
	int iGetADTS = 0;
	unsigned int profile;
	unsigned int sampling_frequency_index;
	unsigned int channel_configuration;
	unsigned int number_of_raw_data_blocks_in_frame;
	if (bmp4)
	{
		mConfigSize = 7;
	}else
	{
		mConfigSize = 2;
	}
	
	if (mConfig)
	{
		delete[] mConfig;
		mConfig = NULL;
	}
	mConfig = new char[mConfigSize];
	while(offset < filelength)
	{ 
		proleft = 0;
		totalPos = offset;
		if (iGetADTS < 0)
		{
			memcpy(tempbuffer, data + curpos, data_size);
			totalPos -= data_size;
			memcpy(data, tempbuffer, data_size);
			proleft = data_size;
		}
		
		data_size = fread(data + data_size, 1, BUFFER_MAX_LEN - data_size, mFile);
		input_data = data;
		curpos = 0;
		offset += data_size;
		data_size += proleft;
		
		while ((iGetADTS = Get_One_ADTS_Frame(input_data, data_size, frame, &size)) == 0)
		{
			if (!bGot)
			{
				bGot = true;
				profile = ((frame[2] & 0xC0) >> 6) + 1;
				sampling_frequency_index = (frame[2] & 0x3C) >> 2;
				channel_configuration = ((frame[2] & 0x01) << 2) | ((frame[3] & 0xC0) >> 6);	
				number_of_raw_data_blocks_in_frame = ((frame[6] & 0x03) + 1) * 1024;
				mConfig[0] = (profile << 3) | ((sampling_frequency_index & 0xe) >> 1);
				mConfig[1] = ((sampling_frequency_index & 0x1) << 7) | (channel_configuration << 3);
			}
			input_data += size;
			data_size -= size;
			AudioSourceData temp(size - 7, totalPos + curpos + 7);
			mAudios.push_back(temp);
			curpos += size;

		}
		
	} 
	for (int i = 0; i < mAudios.size(); i++)
	{
		mAudios[i].mTimeStamp = i * number_of_raw_data_blocks_in_frame * 1000.0 / frequencies[sampling_frequency_index];
	}
	
	mSampleRate = frequencies[sampling_frequency_index];
	if (data)
	{
		delete[] data;
		data = NULL;
	}
	if (frame)
	{
		delete[] frame;
		frame = NULL;
	}
	if (tempbuffer)
	{
		delete[] tempbuffer;
		tempbuffer = NULL;
	}
	return true;
}

bool AACParse::analyze_mp4head( FILE *infile,unsigned long long frontlength /*= 0*/ )
{
	fseek(infile, 0, SEEK_END);
	long long filelen = ftell(infile);

	fseek(infile, 0, SEEK_SET);
	Box root("root",filelen,0);
	root.FindAllChild(infile);
	Box trak = root.GetChild("trak");
	Box hdlr = root.GetChild("hdlr");
	Box mdhd = root.GetChild("mdhd");
	unsigned int traktype = hdlr.BRead32(infile,8);
	unsigned int audiotimescale = 0;
	unsigned int lastindex = mAudios.size();
	if (mConfig)
	{
		delete[] mConfig;
		mConfig = NULL;
	}
	if (traktype == 'soun')
	{
		audiotimescale = mdhd.BRead32(infile,12);
		mSampleRate = audiotimescale;
		Box stsd = root.GetChild("mdia").FindBox(infile,"minf").FindBox(infile,"stbl").FindBox(infile,"stsd");
		if (stsd.pos)
		{
			for (int i = 0; i < stsd.size; i ++)
			{
				if (stsd.BRead32(infile,i) == 'esds')
				{
					i += 4;
					int nCount = 4;
					unsigned char tagflag = 0x3;
					unsigned int nSize = 0;
					if (stsd.BRead8(infile,i + nCount) == tagflag)
					{
						nCount ++;
						while (stsd.BRead8(infile,i + nCount) & 0x80)
						{
							nSize = (nSize << 7) | (stsd.BRead8(infile,i + nCount) & 0x7f);
							nCount ++;
						}
						nSize = (nSize << 7) | (stsd.BRead8(infile,i + nCount) & 0x7f);
						nCount ++;
					}
					tagflag = 0x4;
					nCount += 3;
					nSize = 0;
					if (stsd.BRead8(infile,i + nCount) == tagflag)
					{
						nCount ++;
						while (stsd.BRead8(infile,i + nCount) & 0x80)
						{
							nSize = (nSize << 7) | (stsd.BRead8(infile,i + nCount) & 0x7f);
							nCount ++;
						}
						nSize = (nSize << 7) | (stsd.BRead8(infile,i + nCount) & 0x7f);
						nCount ++;
					}
					tagflag = 0x5;
					nCount += 13;
					nSize = 0;
					if (stsd.BRead8(infile,i + nCount) == tagflag)
					{
						nCount ++;
						while (stsd.BRead8(infile,i + nCount) & 0x80)
						{
							nSize = (nSize << 7) | (stsd.BRead8(infile,i + nCount) & 0x7f);
							nCount ++;
						}
						nSize = (nSize << 7) | (stsd.BRead8(infile,i + nCount) & 0x7f);
						nCount ++;
					}
					if (!mConfig)
					{
						string config = stsd.BReadBytes(infile,i + nCount, nSize);
						mConfigSize = config.length();
						mConfig = new char[mConfigSize];
						for (int i = 0; i < config.length(); i ++)
						{
							mConfig[i] = config[i];
						}
					}

					break;
				}
			}
			unsigned int nSampleCount = 0;

			//data size
			Box stsz = trak.FindBox(infile,"mdia").FindBox(infile,"minf").FindBox(infile,"stbl").FindBox(infile,"stsz");
			if (stsz.pos)
			{
				unsigned int samplesize = stsz.BRead32(infile,4);
				unsigned int nCount = stsz.BRead32(infile,8);
				if (samplesize == 0)
				{
					for (int i = 0; i < nCount; i ++)
					{
						AudioSourceData temp(stsz.BRead32(infile,i * 4 + 12));
						if (i * 4 + 12 >= stsz.size - 8)
						{
							mAudios.clear();
							return false;
						}
						mAudios.push_back(temp);
					}
				}
				else
				{
					for (int i = 0; i < nCount; i ++)
					{
						AudioSourceData temp(samplesize);
						mAudios.push_back(temp);
					}
				}
				nSampleCount = nCount;
			}
			else
			{
				return false;
			}

			//pts
			Box stts = trak.FindBox(infile,"mdia").FindBox(infile,"minf").FindBox(infile,"stbl").FindBox(infile,"stts");
			if (stts.pos)
			{
				unsigned int nSamples = 0;
				unsigned int nCount = stts.BRead32(infile,4);

				for (int i = 0; i < nCount && nSamples < nSampleCount; i ++)
				{
					unsigned int sample_count = stts.BRead32(infile,i * 8 + 8);
					unsigned int sample_delta = stts.BRead32(infile,i * 8 + 12);

					for (int j = nSamples; j < nSamples + sample_count && nSamples < nSampleCount; j ++)
					{
						mAudios[j].mTimeStamp = (sample_delta * 1000.0) / (double)audiotimescale;
					}
					nSamples += sample_count;
				}
			}
			else
			{
				return false;
			}

			//dts
			Box ctts = trak.FindBox(infile,"mdia").FindBox(infile,"minf").FindBox(infile,"stbl").FindBox(infile,"ctts");
			if (ctts.pos)
			{
				unsigned int nSamples = 0;
				unsigned int nCount = ctts.BRead32(infile,4);

				for (int i = 0; i < nCount && nSamples < nSampleCount; i ++)
				{
					unsigned int sample_count = ctts.BRead32(infile,i * 8 + 8);
					unsigned int sample_offset = ctts.BRead32(infile,i * 8 + 12);

					for (int j = nSamples; j < nSamples + sample_count && nSamples < nSampleCount; j ++)
					{
						mAudios[j + lastindex].mDts = (sample_offset * 1000.0) / (double)audiotimescale;
					}
					nSamples += sample_count;
				}
			}

			//data pos
			Box stsc = trak.FindBox(infile,"mdia").FindBox(infile,"minf").FindBox(infile,"stbl").FindBox(infile,"stsc");
			Box stco = trak.FindBox(infile,"mdia").FindBox(infile,"minf").FindBox(infile,"stbl").FindBox(infile,"stco");
			if (stsc.pos && stco.pos)
			{

				unsigned int nSamples = 0;
				unsigned int nSTSC = stsc.BRead32(infile,4);
				unsigned int nSTCO = stco.BRead32(infile,4);

				for(int i = 0; i < nSTSC && nSamples < nSampleCount; i ++)
				{
					unsigned int first_chunk = stsc.BRead32(infile,i * 12 + 8) - 1;
					unsigned int sample_pre_chunk = stsc.BRead32(infile,i * 12 + 12);
					unsigned int last_chunk = nSTCO;
					if (i != nSTSC - 1)
					{
						last_chunk = stsc.BRead32(infile,(i + 1) * 12 + 8) - 1;
					}
					for (int chunk = first_chunk; chunk < last_chunk && nSamples < nSampleCount; chunk ++)
					{
						unsigned int first_sample = nSamples;
						unsigned int last_sample = first_sample + sample_pre_chunk;

						long offset = 0;
						for (int index = first_sample; index < last_sample && nSamples < nSampleCount; index ++)
						{
							mAudios[index + lastindex].mDataPos = stco.BRead32(infile,chunk * 4 + 8) + offset + frontlength;
							offset += mAudios[index + lastindex].mDataSize;
						}

						nSamples += last_sample - first_sample;
					}
				}
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
	return true;
}

void AACParse::Clear()
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

char* AACParse::GetConfig()
{
	if (mConfigSize && mConfig)
	{
		char* config = new char[mConfigSize];
		memcpy(config,mConfig,mConfigSize);
		return config;
	}
	return NULL;
}

unsigned int AACParse::GetConfigLength()
{
	return mConfigSize;
}

unsigned int AACParse::Size()
{
	return mAudios.size();
}

bool AACParse::GetDataByIndex( unsigned int index,char* data,unsigned int &size )
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
	return 1;
}

double AACParse::GetPTSByIndex( unsigned int index )
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

unsigned int AACParse::GetSampleRate()
{
	return mSampleRate;
}

unsigned int AACParse::GetSizeByIndex( unsigned int index )
{
	if (index >= mAudios.size())
	{
		return 0;
	}

	return mAudios[index].mDataSize;
}

unsigned long long AACParse::GetFileLength()
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
