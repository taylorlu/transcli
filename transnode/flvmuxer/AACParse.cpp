#include "AACParse.h"
#include "Box.h"
#include <time.h>
#include <string.h>
#include "writeBits.h"

static int frequencies[] = { 96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350};
AACParse::AACParse(void)
{
}

AACParse::~AACParse(void)
{
}

bool AACParse::Parse(const char* esFileName)
{
	Clear();
	if (!esFileName) {
		return false;
	}

	mFile = fopen(esFileName,"rb");
	if (!mFile) {
		return false;
	}

	// Get file length
	fseek(mFile, 0, SEEK_END);
	mFileLen = ftell(mFile);

	// Judge es container type:ADTS or M4A
	fseek(mFile, 4, SEEK_SET);
	uint8_t fileHeader[4] = {0};
	fread(fileHeader, 1, 4, mFile);
	if(fileHeader[0] == 'f' && fileHeader[1] == 't' &&
	   fileHeader[2] == 'y' && fileHeader[3] == 'p') {	// Mp4 file type
		return parseM4A();
	} else {
		return parseADTS();
	}
}

bool AACParse::parseM4A()
{
	//uint64_t t = time(0);
	if (!analyze_mp4head(mFile)) {
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

int AACParse::getOneADTSFrame(unsigned char* buffer, size_t buf_size, unsigned char* data ,size_t* data_size)
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

bool AACParse::parseADTS()
{
	unsigned char* data = new unsigned char[BUFFER_MAX_LEN];
	uint64_t preadts = 0;
	uint64_t curadts = -1;
	unsigned short int flg = 0;
	uint64_t offset = 0;
	uint64_t proleft = 0;
	uint64_t data_size = 0;
	bool bGot = false;
	fseek(mFile, offset, SEEK_SET);
	
	unsigned char * input_data = data;
	unsigned char *frame = new unsigned char[FRAME_MAX_LEN];
	unsigned char *tempbuffer = new unsigned char[1024 * 1024];
	size_t size = 0; 
	int curpos = 0;
	int totalPos = 0;
	int iGetADTS = 0;
	uint32_t profile;
	uint32_t sr_index;
	uint32_t channel_configuration;
	uint32_t number_of_raw_data_blocks_in_frame;
	if (mOutputType == OUTPUT_TYPE_MP4) {
		mConfigSize = 5;
	} else {
		mConfigSize = 2;
	}
	
	if (mConfig)
	{
		delete[] mConfig;
		mConfig = NULL;
	}
	mConfig = new uint8_t[mConfigSize];
	
	while(offset < mFileLen)
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
		
		while ((iGetADTS = getOneADTSFrame(input_data, data_size, frame, &size)) == 0)
		{
			if (!bGot)
			{
				bGot = true;
				profile = ((frame[2] & 0xC0) >> 6) + 1;
				sr_index = (frame[2] & 0x3C) >> 2;
				mChannelCount = ((frame[2] & 0x01) << 2) | ((frame[3] & 0xC0) >> 6);	
				number_of_raw_data_blocks_in_frame = ((frame[6] & 0x03) + 1) * 1024;
				mSampleRate = frequencies[sr_index];

				mConfig[0] = (profile << 3) | ((sr_index & 0xe) >> 1);
				mConfig[1] = ((sr_index & 0x1) << 7) | (mChannelCount << 3);

				uint32_t sbr_sr_index = sr_index;
				for (int i=0; i<sizeof(frequencies)/sizeof(int); i++) {
					if (frequencies[i] == 2*mSampleRate) {
						sbr_sr_index = i;
						break;
					}
				}
				PutBitContext pbc;
				init_put_bits(&pbc, mConfig, mConfigSize);
				put_bits(&pbc, 5, profile);
				put_bits(&pbc, 4, sr_index);
				put_bits(&pbc, 4, mChannelCount);
				put_bits(&pbc, 3, 0);	// pad 3bits
				if (mOutputType == OUTPUT_TYPE_MP4) {
					put_bits(&pbc, 11, 0x2b7);
					put_bits(&pbc, 5, 5);
					put_bits(&pbc, 1, 1);
					put_bits(&pbc, 4, sbr_sr_index);
					put_bits(&pbc, 3, 0);	// pad 3bits
				}
			}
			input_data += size;
			data_size -= size;
			AudioSourceData temp(size - 7, totalPos + curpos + 7);
			mAudios.push_back(temp);
			curpos += size;
		}
	} 

	for (int i = 1; i < mAudios.size(); i++)
	{
		mAudios[i].mTimeStamp = i * number_of_raw_data_blocks_in_frame * 1000.0 / mSampleRate;
	}
	
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

bool AACParse::analyze_mp4head( FILE *infile, uint64_t frontlength /*= 0*/ )
{
	fseek(infile, 0, SEEK_SET);
	Box root("root", mFileLen,0);
	root.FindAllChild(infile);
	Box trak = root.GetChild("trak");
	Box hdlr = root.GetChild("hdlr");
	Box mdhd = root.GetChild("mdhd");
	uint32_t traktype = hdlr.BRead32(infile,8);
	uint32_t audiotimescale = 0;
	uint32_t lastindex = mAudios.size();
	if (mConfig) {
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
					uint32_t nSize = 0;
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
						mConfig = new uint8_t[mConfigSize];
						for (int i = 0; i < config.length(); i ++)
						{
							mConfig[i] = config[i];
						}
					}

					break;
				}
			}
			uint32_t nSampleCount = 0;

			//data size
			Box stsz = trak.FindBox(infile,"mdia").FindBox(infile,"minf").FindBox(infile,"stbl").FindBox(infile,"stsz");
			if (stsz.pos)
			{
				uint32_t samplesize = stsz.BRead32(infile,4);
				uint32_t nCount = stsz.BRead32(infile,8);
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
				uint32_t nSamples = 0;
				uint32_t nCount = stts.BRead32(infile,4);

				for (int i = 0; i < nCount && nSamples < nSampleCount; i ++)
				{
					uint32_t sample_count = stts.BRead32(infile,i * 8 + 8);
					uint32_t sample_delta = stts.BRead32(infile,i * 8 + 12);

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
				uint32_t nSamples = 0;
				uint32_t nCount = ctts.BRead32(infile,4);

				for (int i = 0; i < nCount && nSamples < nSampleCount; i ++)
				{
					uint32_t sample_count = ctts.BRead32(infile,i * 8 + 8);
					uint32_t sample_offset = ctts.BRead32(infile,i * 8 + 12);

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

				uint32_t nSamples = 0;
				uint32_t nSTSC = stsc.BRead32(infile,4);
				uint32_t nSTCO = stco.BRead32(infile,4);

				for(int i = 0; i < nSTSC && nSamples < nSampleCount; i ++)
				{
					uint32_t first_chunk = stsc.BRead32(infile,i * 12 + 8) - 1;
					uint32_t sample_pre_chunk = stsc.BRead32(infile,i * 12 + 12);
					uint32_t last_chunk = nSTCO;
					if (i != nSTSC - 1)
					{
						last_chunk = stsc.BRead32(infile,(i + 1) * 12 + 8) - 1;
					}
					for (int chunk = first_chunk; chunk < last_chunk && nSamples < nSampleCount; chunk ++)
					{
						uint32_t first_sample = nSamples;
						uint32_t last_sample = first_sample + sample_pre_chunk;

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

uint32_t AACParse::GetFrameLength()
{
	return 1024;
}
uint8_t AACParse::GetObjectTypeIndication()
{
	return 0x40;
}
