#ifndef _MP3_ENCODE_H_
#define _MP3_ENCODE_H_

#include "AudioEncode.h"
#include "lame.h"

class CMp3Encode : public CAudioEncoder
{
public:
	CMp3Encode(const char* outFileName);
	~CMp3Encode(void);

	bool    Initialize() ;
	int64_t EncodeBuffer(uint8_t* pAudioBuf, int bufLen) ;	
	bool    Stop();
private:
	CMp3Encode();
	void dumpErrInfo(int errType);

	lame_global_flags*	m_pParams;
	int16_t*			m_pLeftChAudio;
	int16_t*			m_pRightChAudio;
	uint8_t*			m_pMp3StreamBuf;
	int					m_mp3BufSize;

};

#endif


