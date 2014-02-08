#include "processwrapper.h"
#include "AudioEncode.h"
#include "VideoEncode.h"

//#ifndef WIN32
//#define COMMUNICATION_USE_FIFO
//#else
//#undef COMMUNICATION_USE_FIFO
//#endif

//#ifndef WIN32
#define USE_NEW_X264
//#endif

class CCLIVideoEncoder : public CVideoEncoder
{
public:
	CCLIVideoEncoder(const char* outFileName) : CVideoEncoder(outFileName) {}
	virtual bool Initialize();
	virtual std::string GetCommandLine() = 0;
	virtual int EncodeFrame(uint8_t* pFrameBuf, int bufSize, bool bLast);
	virtual bool Stop();
	virtual int GetCLIPid() { return m_proc.GetPid(); }
	virtual bool IsRunning();		// true: process is running, false: process is down

protected:
#ifdef COMMUNICATION_USE_FIFO
	int makeVideoFifo();
	#endif

	CProcessWrapper m_proc;
	std::string m_strFifo;
	int m_fdWrite;
};

class CCLIAudioEncoder : public CAudioEncoder
{
public:
	CCLIAudioEncoder(const char* outFileName) : CAudioEncoder(outFileName) {}
	virtual	bool Initialize();
	virtual	int64_t EncodeBuffer(uint8_t* pAudioBuf, int bufLen);
	virtual std::string GetCommandLine() = 0;
	virtual bool Stop();
	virtual int GetCLIPid() {return m_proc.GetPid();}
	virtual bool IsRunning();		// true: process is running, false: process is down
protected:
#ifdef COMMUNICATION_USE_FIFO
	int makeAudioFifo();
	#endif

	CProcessWrapper m_proc;
	std::string m_strFifo;
	int m_fdWrite;
};

class CFFmpegVideoEncoder : public CCLIVideoEncoder
{
public:
	CFFmpegVideoEncoder(const char* outFileName) : CCLIVideoEncoder(outFileName),
								m_fdWriteForExe(-1), m_fdExeRead(-1){}
	bool Initialize();
	int EncodeFrame(uint8_t* pFrameBuf, int bufSize, bool bLast);
	std::string GetCommandLine();
	bool Stop();
	
private: 
	const char* GetCodecName(int id);
	int m_fdWriteForExe;
	int m_fdExeRead;
};

class CX264CLIEncoder : public CCLIVideoEncoder
{
public:
	CX264CLIEncoder(const char* outFileName) : CCLIVideoEncoder(outFileName) {}
	std::string GetCommandLine();
};

class CCudaCLIEncoder : public CCLIVideoEncoder
{
public:
	CCudaCLIEncoder(const char* outFileName) : CCLIVideoEncoder(outFileName) {}
	std::string GetCommandLine();
};

class CMindCLIEncoder : public CCLIVideoEncoder
{
public:
	CMindCLIEncoder(const char* outFileName) : CCLIVideoEncoder(outFileName) {}
	std::string GetCommandLine();
};

class CFFmpegAudioEncoder : public CCLIAudioEncoder
{
public:
	CFFmpegAudioEncoder(const char* outFileName) : CCLIAudioEncoder(outFileName) {}
	std::string GetCommandLine();

};

class CFaacCLI : public CCLIAudioEncoder
{
public:
	CFaacCLI(const char* outFileName) : CCLIAudioEncoder(outFileName) {}
	std::string GetCommandLine();
};

class CNeroAudioEncoder : public CCLIAudioEncoder
{
public:
	CNeroAudioEncoder(const char* outFileName) : CCLIAudioEncoder(outFileName) {}
	std::string GetCommandLine();
	bool Initialize();
private: 
	const char* GetCodecName(int id);
};
