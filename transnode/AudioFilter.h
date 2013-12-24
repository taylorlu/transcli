#ifndef _AUDIO_FILTER_H_
#define _AUDIO_FILTER_H_

#ifndef HIGHPREC
typedef float REAL;
#else
typedef double REAL;
#endif

class CAudioFilter
{
public:
	struct AfConfig {
		int m_sfrq, m_dfrq, m_nch, m_dither, m_pdf, m_fast;
		AfConfig(int srcFreq,int destFreq,int srcChannelNum,int dither,int pdf,int fast=1)
		{
			m_sfrq		= srcFreq;
			m_dfrq		= destFreq;
			m_nch		= srcChannelNum;
			m_dither	= dither;
			m_pdf		= pdf;
			m_fast		= fast;
		}	
	};

	CAudioFilter* CreateAudioFilter(const AfConfig& afConfig);
	bool DoSample(uint8_t* pInBuf, uint32_t inSize, bool ifEnd) = 0;
	virtual ~CAudioFilter(void);

protected:
	CAudioFilter(void);
	bool upSample(uint8_t* pInBuf, uint32_t inSize, bool ifEnd);
	bool downSample(uint8_t* pInBuf, uint32_t inSize, bool ifEnd);

	double  m_peak;
	int		m_nch;
	int		m_sfrq;
	int		m_dfrq;
	double  m_gain;

	bool    m_bDownSample;

	double AA;
	double DF;
	int FFTFIRLEN;
};

#endif