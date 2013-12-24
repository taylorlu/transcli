#ifndef _XVID_ENCODE_H_
#define _XVID_ENCODE_H_

#include "VideoEncode.h"

#include "xvid.h"

class CXvidEncode : public CVideoEncoder
{
public:
	CXvidEncode(CStreamOutput* outStream);
	CXvidEncode(const char* outFileName);
	~CXvidEncode(void);
	bool Initialize();
	int EncodeFrame(uint8_t* pFrameBuf, int bufSize, bool bLast);
	bool Stop();

private:
	CXvidEncode(void) {};
	int  getCpuAndEncInfo(int& threadNum);

	void*			  m_pXvidHandle;
	xvid_enc_frame_t  m_xvidFrame;

	//xvid_enc_stats_t  m_xvidStats;
};

#endif
