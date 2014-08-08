#ifndef __AAC_PARSE_H__
#define __AAC_PARSE_H__
#include "AudioParse.h"

class AACParse : public AudioParse
{
public:
	AACParse(void);
	~AACParse(void);

	bool Parse(const char* esFileName);
	uint32_t GetFrameLength();
	uint8_t  GetObjectTypeIndication();
	const char* GetAudioType() {return "mp4a";}
	//void SetAacEncodeType(int type) {mAacEncType = type;}

private:
	//int mAacEncType;
	bool analyze_mp4head(FILE *infile, uint64_t frontlength = 0);
	int  getOneADTSFrame(uint8_t* buffer, size_t buf_size, uint8_t* data ,size_t* data_size);
	bool parseADTS();
	bool parseM4A();
};


#endif