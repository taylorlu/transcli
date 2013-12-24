#ifndef _AV_ENCODER_FFMPEG_H_
#define _AV_ENCODER_FFMPEG_H_

#include "AVEncoder.h"
#include "processwrapper.h"

class CFFMpegAVEncoder : public CAVEncoder
{
public:
	CFFMpegAVEncoder(const char* outFileName);
	~CFFMpegAVEncoder(void);

	bool    Initialize();
	bool    Stop();
	CProcessWrapper* GetProcessWrapper() {return &m_proc;}
private:
	CFFMpegAVEncoder(void);
	std::string getCommandLine();
	void autoCropOrAddBand();
	CProcessWrapper m_proc;
};

#endif