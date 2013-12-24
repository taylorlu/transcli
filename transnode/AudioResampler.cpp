#include <stdio.h>

#include "AudioResampler.h"
#include "logger.h"

CAudioResampler::CAudioResampler(int srcSrate, int channelNum, int dstSrate, int bits, int quality):
m_srcSamplerate(0), m_dstSamplerate(0), m_srateRatio(-1.0), m_channelNum(0),
m_quality(AUDIO_RESAMPLE_MEDIUM), m_initBufLen(0),  m_srcState(NULL),
m_inBuffer(NULL), m_outBuffer(NULL)
{
	m_srcSamplerate = srcSrate;
	m_channelNum = channelNum;
	m_dstSamplerate = dstSrate;
	m_quality = quality;
	m_sampleBits = bits;
}

CAudioResampler::~CAudioResampler(void)
{
	UinitResampler();
}

bool CAudioResampler::InitResampler()
{
	if(!m_srcState) {
		int errorCode = 0;
		m_srcState = src_new(m_quality, m_channelNum, &errorCode);
		if(!m_srcState) {
			logger_err(LOGM_AF_RESAMPLE, "Audio resample error: %s.\n", src_strerror(errorCode));
			return false;
		}
		if(m_srateRatio < 0) {
			if(m_dstSamplerate == 0) {
				logger_err(LOGM_AF_RESAMPLE, "Audio resample: please set dest samplerate or samprate ratio.\n");
				src_delete(m_srcState);
				return false;
			}
			m_srateRatio = (double)m_dstSamplerate / m_srcSamplerate;
		}
		m_srcData.end_of_input = 0 ; /* Set this later. */
		m_srcData.input_frames = 0 ;
		m_srcData.src_ratio = m_srateRatio ;
	}
	return true;
}

union float2int_u { float f; int32_t i; };

bool CAudioResampler::Process(uint8_t* srcBuf, size_t srcLen, uint8_t* destBuf, int& outLen)
{
	const int sampleBytes = m_sampleBits >> 3;

	if(!m_inBuffer) {	// Allocate in and out buffer
		m_inBuffer = new float[srcLen/sampleBytes];
		int bufferRatio = m_srateRatio > 1 ? (int)(m_srateRatio+1) : 1;
		int outBufferLen = srcLen * bufferRatio / sampleBytes;
		m_outBuffer = new float[outBufferLen];
		if(!m_inBuffer || !m_outBuffer) {
			logger_err(LOGM_AF_RESAMPLE, "Audio resample error: Failed to allocate buffer.\n");
			return false;
		}
		m_srcData.output_frames = outBufferLen/m_channelNum;
	}

	if(m_initBufLen == 0) {
		m_initBufLen = srcLen;
	}

	if(srcLen < m_initBufLen) m_srcData.end_of_input = 1;

	// Fill in buffer
	for (size_t i=0; i<srcLen/sampleBytes; i++) {
 		int16_t int16Val = *(int16_t*)(srcBuf+i*sampleBytes);
 		m_inBuffer[i] = (float) (int16Val / (1.0 * 0x8000)) ;
	}

	m_srcData.data_in = m_inBuffer;
	m_srcData.data_out = m_outBuffer;
	m_srcData.input_frames = srcLen/sampleBytes/m_channelNum;
	
	int errorCode = src_process(m_srcState, &m_srcData);
	if(errorCode) {
		logger_err(LOGM_AF_RESAMPLE, "Audio resample error: %s.\n", src_strerror(errorCode));
		return false;
	}

	// Fill out buffer
	size_t sampleNum = m_srcData.output_frames_gen*m_channelNum;
	for (size_t i=0; i<sampleNum; ++i) {
		
#if 0
		double scaled_value = m_outBuffer[i] * (8.0 * 0x10000000);
		if (scaled_value >= (1.0 * 0x7FFFFFFF)) {
			*((int16_t*)(destBuf+i*sampleBytes)) = 32767 ;
		} else {
			*((int16_t*)(destBuf+i*sampleBytes)) = (int16_t)((int(scaled_value))>>16);
		}
#else
		/* This is walken's trick based on IEEE float format. */
		float2int_u u;
		u.f = m_outBuffer[i] + 384.0f;
		if ( u.i > 0x43c07fff ) *((int16_t*)(destBuf+i*sampleBytes)) = 32767;
		else if ( u.i < 0x43bf8000 ) *((int16_t*)(destBuf+i*sampleBytes)) = -32768;
		else *((int16_t*)(destBuf+i*sampleBytes)) = u.i - 0x43c00000;
#endif
	}

	outLen = sampleNum*sampleBytes;
	return true;
}

bool CAudioResampler::UinitResampler()
{
	if(m_srcState) {
		src_delete(m_srcState);
		m_srcState = NULL;
	}

	if(m_inBuffer) {
		delete[] m_inBuffer;
		m_inBuffer = NULL;
	}

	if(m_outBuffer) {
		delete[] m_outBuffer;
		m_outBuffer = NULL;
	}
	return true;
}