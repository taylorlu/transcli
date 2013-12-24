#ifndef _AUDIO_ENCODE_H_
#define _AUDIO_ENCODE_H_
#include "encoderCommon.h"

/*
	Read me:
	Function: Encode PCM audio buffer to mp3 bitstream.
	Providing pass in 1/5 second audio data every time, so the count of samples passed in equal to "sample rate/5".
*/

class CAudioEncoder
{
public:
	CAudioEncoder(const char* outFileName);
	virtual ~CAudioEncoder(void);
	virtual bool    GeneratePipeHandle();
	void			SetAudioInfo(const audio_info_t& aInfo, CXMLPref* pXmlPrefs);
	virtual	bool    Initialize() = 0;
	virtual	int64_t EncodeBuffer(uint8_t* pAudioBuf, int bufLen) = 0;	// If success return encoded data length else -1
	virtual bool    Stop();
	virtual bool    IsRunning() {return true;}

	void SetOutFileName(const char* outFile) {if(outFile) m_strOutFile = outFile;}
	std::string GetOutFileName() {return m_strOutFile;}
	int		GetBitrate() {return m_bitrate;}		// Unit: kbit

	audio_encoder_t GetEncoderType() {return m_aInfo.encoder_type;}
	audio_info_t*	GetAudioInfo() {return &m_aInfo;}
	CXMLPref*		GetAudioPref() {return m_pXmlPrefs;}

	int  GetWirteHandle() {return m_fdWrite;}
	int  GetReadHandle() {return m_fdRead;}

	void CloseWriteHandle();
	void CloseReadHandle();

	void SetWorkerId(int id) {m_tid = id;}
	int  GetLogModuleType() {return m_logModuleType;}
	int	 GetFrameLen() {return m_frameLen;}

	void SetSourceIndex(int srcIdx = 0) {m_sourceIndex = srcIdx;}
	int  GetSourceIndex() {return m_sourceIndex;}

	bool IsStopped() {return m_bClosed == 1;}
	//float GetAverageSpeed() {return m_averageXSpeed;}
	//float GetCurrentSpeed() {return m_curXSpeed;}
	//if the encoder is not a CLI, return -1;
	virtual int GetCLIPid() { return -1;}

protected:
	CAudioEncoder();
	
	std::string		m_strOutFile;
	CStreamOutput*	m_pStreamOut;
	int				m_fdRead;
	int				m_fdWrite;
	int				m_tid;
	int				m_frameLen;
	
	audio_info_t	m_aInfo;
	CXMLPref*		m_pXmlPrefs;

	//int				m_state;
	int				m_logModuleType;
	int64_t			m_encodedBytes;

	int				m_sourceIndex;		// Source decoder index (0 is main decoder, other is aux decoder)
	int				m_bClosed;
	int				m_bitrate;
	// Benchmark
// 	int64_t			m_startTime;
// 	int64_t			m_lastTime;
// 	float			m_lastPlayTime;
// 	float			m_curXSpeed;		// Current speed compare to playback speed
// 	float			m_averageXSpeed;	// Average speed compare to playback speed
};

#endif
