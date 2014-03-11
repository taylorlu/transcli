#ifndef _DECODER_FFMPEG_H_
#define _DECODER_FFMPEG_H_

#include "Decoder.h"

class CDecoderFFMpeg : public CDecoder
{
public:
	CDecoderFFMpeg(void);
	std::string GetCmdString(const char* mediaFile);
	bool ReadAudioHeader();
	~CDecoderFFMpeg(void);

private:
	std::string GenAudioFilterOptions();
	std::string GenVideoFilterOptions(int subType);
	std::string GenTextSubOptions(const char* mediaFile, std::string& subFile);
	std::vector<std::string> tmpPathVct;
	void ExtractTextSub(const char* mediaFile, int subId, std::string& subFile);
};

#endif
