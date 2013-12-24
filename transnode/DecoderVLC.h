#ifndef _DECODER_VLC_H_
#define _DECODER_VLC_H_

#include "Decoder.h"

class CDecoderVLC : public CDecoder
{
public:
	CDecoderVLC(void);
	~CDecoderVLC(void);

	bool Start(const char* sourceFile);
private:
	std::string GenVideoFilterOptions();
	std::string GenAudioFilterOptions();
	std::string GenFilterOptions();
	bool TestRun();
};

#endif
