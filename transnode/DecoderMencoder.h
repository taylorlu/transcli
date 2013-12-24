#ifndef _DECODER_MENCODER_H_
#define _DECODER_MENCODER_H_

#include "Decoder.h"

class CDecoderMencoder : public CDecoder
{
public:
	CDecoderMencoder(void);
	std::string GetCmdString(const char* mediaFile);
	~CDecoderMencoder(void);
private:
	std::string GenAudioFilterOptions();
	std::string GenVideoFilterOptions();
	std::string GenSubTitleOptions(const char* srcFile, bool& useAss);
	
};

#endif
