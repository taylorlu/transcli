#ifndef _AUDIO_RESAMPLER_H_
#define _AUDIO_RESAMPLER_H_

#include <stdint.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include "samplerate.h"

enum AudioResampleQualityRank {
	AUDIO_RESAMPLE_BEST = SRC_SINC_BEST_QUALITY,		
	AUDIO_RESAMPLE_MEDIUM = SRC_SINC_MEDIUM_QUALITY	,	
	AUDIO_RESAMPLE_FAST = SRC_SINC_FASTEST	,		
	AUDIO_RESAMPLE_FASTER = SRC_ZERO_ORDER_HOLD	,	
	AUDIO_RESAMPLE_FASTEST = SRC_LINEAR
};

class CAudioResampler
{
public:
	CAudioResampler(int srcSrate, int channelNum, int dstSrate=0, int bits=16, int quality=AUDIO_RESAMPLE_MEDIUM);
	void SetSampleRateRatio(double rateRatio) {m_srateRatio = rateRatio;}

	bool InitResampler();
	bool Process(uint8_t* srcBuf, size_t srcLen, uint8_t* destBuf, int& outLen);
	bool UinitResampler();

	~CAudioResampler(void);

private:
	int		m_srcSamplerate;
	int		m_dstSamplerate;
	double	m_srateRatio;
	int     m_channelNum;
	int     m_quality;
	size_t	m_initBufLen;
	int		m_sampleBits;

	SRC_STATE* m_srcState;		// Implement using LibSamplerate (SRC)
	SRC_DATA   m_srcData;

	float*	   m_inBuffer;		// uint8 stream should be converted to float array
	float*     m_outBuffer;		// out buffer should be float array
};

#endif
