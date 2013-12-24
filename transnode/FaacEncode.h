#ifndef _FAAC_ENCODE_H_
#define _FAAC_ENCODE_H_

#include "AudioEncode.h"
#include "faac.h"

class CFaacEncode : public CAudioEncoder
{
public:
	CFaacEncode(const char* outFileName);
	~CFaacEncode(void);

	bool    Initialize() ;
	int64_t EncodeBuffer(uint8_t* pAudioBuf, int bufLen) ;	
	bool    Stop();

private:
	CFaacEncode();
	
	faacEncHandle	m_hEncoder;
	//float*			m_pRawBuf;
	uint8_t*		m_pOutBuf;
	unsigned long	m_samplesInput;
	unsigned long	m_maxBytesOutput;
};

#endif