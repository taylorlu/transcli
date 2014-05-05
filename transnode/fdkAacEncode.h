#ifndef _FDK_AAC_ENCODE_H_
#define _FDK_AAC_ENCODE_H_

#include "AudioEncode.h"
#include "fdk-aac/aacenc_lib.h"

class CFdkAacEncode : public CAudioEncoder
{
public:
	CFdkAacEncode(const char* outFileName);
	~CFdkAacEncode(void);

	bool    Initialize() ;
	int64_t EncodeBuffer(uint8_t* pAudioBuf, int bufLen) ;	
	bool    Stop();

private:
	CFdkAacEncode();
	
	HANDLE_AACENCODER m_sHandle;
	uint8_t*		m_pOutBuf;
	uint32_t		m_iOutBufLen;
};

#endif