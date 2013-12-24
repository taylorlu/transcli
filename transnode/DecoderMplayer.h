#ifndef _DECODER_MPLAYER_H_
#define _DECODER_MPLAYER_H_

#include "Decoder.h"

class CDecoderMplayer : public CDecoder
{
public:
	CDecoderMplayer(void);
	std::string GetCmdString(const char* mediaFile);
	~CDecoderMplayer(void);

private:
	std::string GenAudioFilterOptions();
};

#endif
