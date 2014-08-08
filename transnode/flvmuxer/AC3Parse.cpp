#include "AC3Parse.h"
#include "GlobalDefine.h"
#include "writeBits.h"

struct SAC3Header
{
	uint32_t bitrate;
	uint32_t sample_rate;
	uint32_t framesize;
	uint32_t channels;
	uint32_t is_ec3;
	/*only set if full parse*/
	uint8_t fscod, bsid, bsmod, acmod, lfon, brcode;
};

struct __ec3_stream
{
	uint8_t fscod;
	uint8_t bsid;
	uint8_t bsmod;
	uint8_t acmod;
	uint8_t lfon;
	/*only for EC3*/
	uint8_t nb_dep_sub;
	uint8_t chan_loc;
};

/*AC3 config record*/
typedef struct SAC3Config
{
	uint8_t is_ec3;
	uint8_t nb_streams;		//1 for AC3, max 8 for EC3
	uint16_t brcode;		//if AC3 is bitrate code, otherwise cumulated data rate of EC3 streams
	__ec3_stream streams[8];
} ;

static const uint32_t ac3_sizecod_to_bitrate[] = {
	32000, 40000, 48000, 56000, 64000, 80000, 96000,
	112000, 128000, 160000, 192000, 224000, 256000,
	320000, 384000, 448000, 512000, 576000, 640000
};

static const uint32_t ac3_sizecod2_to_framesize[] = {
	96, 120, 144, 168, 192, 240, 288, 336, 384, 480, 576, 672,
	768, 960, 1152, 1344, 1536, 1728, 1920
};

static const uint32_t ac3_sizecod1_to_framesize[] = {
	69, 87, 104, 121, 139, 174, 208, 243, 278, 348, 417, 487,
	557, 696, 835, 975, 1114, 1253, 1393
};
static const uint32_t ac3_sizecod0_to_framesize[] = {
	64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 448, 
	512, 640, 768, 896, 1024, 1152, 1280
};

static const uint16_t ac3_channels_tab[] = {
	2, 1, 2, 3, 3, 4, 4, 5
};
static const uint8_t eac3_blocks[4] = {
	1, 2, 3, 6
};
static const uint32_t ac3_sample_rate_tab[3] = { 48000, 44100, 32000 };

typedef enum {
	EAC3_FRAME_TYPE_INDEPENDENT = 0,
	EAC3_FRAME_TYPE_DEPENDENT,
	EAC3_FRAME_TYPE_AC3_CONVERT,
	EAC3_FRAME_TYPE_RESERVED
} EAC3FrameType;

AC3Parse::AC3Parse(void)
{}

AC3Parse::~AC3Parse(void)
{}

bool AC3Parse::findAc3SyncCode()
{
	uint64_t pos = ftell(mFile);
	uint64_t end = mFileLen - 6;
	uint32_t b1, b2;
	while (pos <= end) {
		b1 = Read8(mFile);
		b2 = Read8(mFile);
		if (b1 == 0x0b && b2 == 0x77) {
			fseek(mFile, pos, SEEK_SET);
			return true;
		}
		pos++;
	}
	return false;
}

bool AC3Parse::parseAc3BitStream(SAC3Header *hdr, SAC3Config* config) 
{
	uint32_t fscod, frmsizecod, bsid, ac3_mod, freq, framesize, bsmod, sync_word, num_blocks;
	uint64_t pos;
	uint32_t subsid;
	
	// check sync word
	if (!findAc3SyncCode()) return false;
	pos = ftell(mFile);
	sync_word = Read16(mFile);

	// read ahead to get bsid to distinguish between AC-3 and E-AC-3
	bsid = Read32(mFile);
	bsid >>= 3;
	bsid &= 0x1F;
	if(bsid > 16) return false;

	// Rewind 4 bytes
	Skip(mFile, -4);

	num_blocks = 6;
	if(bsid <= 10) {	// AC3
		hdr->is_ec3 = 0;
		Skip(mFile, 2);
		uint32_t tmp = Read8(mFile);
		fscod = (tmp >> 6) & 0x03;	// front 2 bits
		frmsizecod = tmp & 0x3F;	// rear 6 bits

		tmp = Read8(mFile);
		bsid = (tmp >> 3) & 0x1F;	// front 5 bits
		bsmod = tmp & 0x07;			// rear 3 bits

		hdr->bitrate = ac3_sizecod_to_bitrate[frmsizecod / 2];
		if (bsid > 8) hdr->bitrate = hdr->bitrate >> (bsid - 8);

		switch (fscod) {
		case 0:
			freq = 48000;
			framesize = ac3_sizecod0_to_framesize[frmsizecod / 2] * 2;
			break;
		case 1:
			freq = 44100;
			framesize = (ac3_sizecod1_to_framesize[frmsizecod / 2] + (frmsizecod & 0x1)) * 2;
			break;
		case 2:
			freq = 32000;
			framesize = ac3_sizecod2_to_framesize[frmsizecod / 2] * 2;
			break;
		default:
			return 0;
		}
		hdr->sample_rate = freq;
		hdr->framesize = framesize;

		tmp = Read8(mFile);
		ac3_mod = (tmp >> 5) & 0x07;	// front 3 bits
		hdr->channels = ac3_channels_tab[ac3_mod];
		int bitsRead = 3;
		if (ac3_mod == 0x2) {	// Stereo mode
			bitsRead += 2;
		} else {
			if ((ac3_mod & 0x1) && (ac3_mod != 1)) {
				bitsRead += 2;
			}
			if (ac3_mod & 0x4) {
				bitsRead += 2;
			}
		}
		
		//LFE on
		int bLFEOn = 0;
		if(bitsRead == 3) {
			bLFEOn = tmp & 0x10;
		} else if(bitsRead == 5) {
			bLFEOn = tmp & 0x4;
		} else if(bitsRead == 7) {
			bLFEOn = tmp & 0x1;
		}
		if (bLFEOn) {
			hdr->channels += 1;
			hdr->lfon = 1;
		}
	} else {			// E-AC3
		hdr->is_ec3 = 1;
		uint32_t tmp = Read8(mFile);
		uint32_t frame_type = (tmp >> 6) & 0x3;		// front 2 bits
		if(frame_type == EAC3_FRAME_TYPE_RESERVED) {
			return false;
		}
		subsid = (tmp >> 3) & 0x7;				// mid 3 bits

		uint32_t frameSizeFront3b = tmp & 0x7;
		tmp = Read8(mFile);
		framesize = (frameSizeFront3b << 8 | tmp + 1) << 1;
		if(framesize < 7) return 0;

		tmp = Read8(mFile);
		fscod = (tmp >> 6) & 0x3;	// Front 2 bits
		if(fscod == 3) {
			uint32_t fscod2 = (tmp >> 4) & 0x3;	// Following 2 bits
			if(fscod2 == 3) return 0;
			hdr->sample_rate = ac3_sample_rate_tab[fscod2] / 2;
		} else {
			uint32_t numBlkIdx = (tmp >> 4) & 0x3;	// Following 2 bits
			hdr->sample_rate = ac3_sample_rate_tab[fscod];
			num_blocks = eac3_blocks[numBlkIdx];
		}

		ac3_mod = (tmp >> 1) & 0x7;		// Following 3 bits
		hdr->lfon = tmp & 0x1;			// last 1 bit
		hdr->channels = ac3_channels_tab[ac3_mod] + hdr->lfon;
		hdr->bitrate = (uint32_t)((framesize+1)*hdr->sample_rate/(num_blocks*16.0));
	}

	hdr->framesize = framesize;

	if (config) {
		config->is_ec3 = hdr->is_ec3;
		if(!hdr->is_ec3) {
			config->nb_streams = 1;
			config->brcode = frmsizecod / 2;
			config->streams[0].acmod = ac3_mod;
			config->streams[0].bsid = bsid;
			config->streams[0].bsmod = bsmod;
			config->streams[0].fscod = fscod;
			config->streams[0].lfon = hdr->lfon;
		} else {
			uint8_t i=0;
			config->nb_streams = subsid + 1;
			config->brcode = hdr->bitrate;
			for(i=0; i<config->nb_streams; ++i) {
				config->streams[0].acmod = ac3_mod;
				config->streams[0].bsid = bsid;
				config->streams[0].bsmod = 0;
				config->streams[0].fscod = fscod;
				config->streams[0].lfon = hdr->lfon;
			}
		}
	}

	fseek(mFile, pos, SEEK_SET);
	return true;
}

static uint32_t getAc3ConfigLength(const SAC3Config& ac3Config)
{
	if (ac3Config.is_ec3) {
		uint32_t i;
		uint32_t configSize = 2;
		for (i=0; i<ac3Config.nb_streams; i++) {
			configSize += 3;
			if (ac3Config.streams[i].nb_dep_sub) 
				configSize += 1;
		}
		return configSize;
	} else {
		return 3;
	}
}

bool AC3Parse::Parse(const char* esFileName)
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

	fseek(mFile, 0, SEEK_SET);
	SAC3Header ac3Hdr = {0};
	SAC3Config ac3Config = {0};
	if (!parseAc3BitStream(&ac3Hdr, &ac3Config)) {
		fclose(mFile);
		return false;
	}
	if(ac3Hdr.is_ec3) {
		mOjectTypeIndication = 0xA6;
	} else {
		mOjectTypeIndication = 0xA5;
	}

	mChannelCount = ac3Hdr.channels;
	mSampleRate = ac3Hdr.sample_rate;

	// Get config
	mConfigSize = getAc3ConfigLength(ac3Config);
	mConfig = new uint8_t[mConfigSize];
	PutBitContext pbc;
	init_put_bits(&pbc, mConfig, mConfigSize);
	if (ac3Config.is_ec3) {
		put_bits(&pbc, 13, ac3Config.brcode);
		put_bits(&pbc, 3, ac3Config.nb_streams - 1);
		uint32_t i;
		for (i=0; i<ac3Config.nb_streams; i++) {
			put_bits(&pbc, 2, ac3Config.streams[i].fscod);
			put_bits(&pbc, 5, ac3Config.streams[i].bsid);
			put_bits(&pbc, 5, ac3Config.streams[i].bsmod);
			put_bits(&pbc, 3, ac3Config.streams[i].acmod);
			put_bits(&pbc, 1, ac3Config.streams[i].lfon);
			put_bits(&pbc, 3, 0);
			put_bits(&pbc, 4, ac3Config.streams[i].nb_dep_sub);
			if (ac3Config.streams[i].nb_dep_sub) {
				put_bits(&pbc, 9, ac3Config.streams[i].chan_loc);
			} else {
				put_bits(&pbc, 1, 0);
			}
		}
	} else {
		put_bits(&pbc, 2, ac3Config.streams[0].fscod);
		put_bits(&pbc, 5, ac3Config.streams[0].bsid);
		put_bits(&pbc, 3, ac3Config.streams[0].bsmod);
		put_bits(&pbc, 3, ac3Config.streams[0].acmod);
		put_bits(&pbc, 1, ac3Config.streams[0].lfon);
		put_bits(&pbc, 5, ac3Config.brcode);
		put_bits(&pbc, 5, 0);
	}

	// Parse all audio frames
	uint32_t samplesCount = 0;
	uint64_t dataOffset = ftell(mFile);
	while (parseAc3BitStream(&ac3Hdr, 0)) {
		uint32_t dataLen = ac3Hdr.framesize;
		
		double dTimeStamp = samplesCount*1000.0/mSampleRate;
		AudioSourceData curAudioData(dataLen, dataOffset, dTimeStamp);
		mAudios.push_back(curAudioData);
		samplesCount += 1536;
		dataOffset += dataLen;
		fseek(mFile, dataOffset, SEEK_SET);
	}
	return true;
}

uint32_t AC3Parse::GetFrameLength()
{
	return 1536;
}
uint8_t AC3Parse::GetObjectTypeIndication()
{
	return mOjectTypeIndication;
}

const char* AC3Parse::GetAudioType()
{
	if(mOjectTypeIndication == 0xA6) {
		return "ec-3";
	} else {
		return "ac-3";
	}
}