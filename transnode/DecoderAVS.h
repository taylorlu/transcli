#ifndef _DECODER_AVS_H_
#define _DECODER_AVS_H_

#include "Decoder.h"

class CDecoderAVS : public CDecoder
{
public:
	CDecoderAVS(void);
	std::string GetCmdString(const char* mediaFile);
	bool Start(const char* sourceFile);
	void Cleanup();
	~CDecoderAVS(void);
private:
	bool CreateScriptFile(const char* mediaFile);
	bool RemoveScriptFile();
	std::string GetPluginPath();
	std::string GenDelogoScript(int movieStartFrame, int movieEndFrame, float inFps);
	bool m_bKeepScriptFile;
	std::string m_mainScriptFile;
	std::vector<std::string> m_subScriptFiles;
};

#endif
