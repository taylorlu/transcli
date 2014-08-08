#ifndef __AC3_PARSE_H__
#define __AC3_PARSE_H__
#include "AudioParse.h"

struct SAC3Header;
struct SAC3Config;

class AC3Parse : public AudioParse
{
public:
	AC3Parse(void);
	~AC3Parse(void);

	bool Parse(const char* esFileName);
	uint32_t GetFrameLength();
	uint8_t  GetObjectTypeIndication();
	const char* GetAudioType();

private:
	bool parseAc3BitStream(SAC3Header *hdr, SAC3Config* config);
	bool findAc3SyncCode();

	uint8_t mOjectTypeIndication;
};

#endif
