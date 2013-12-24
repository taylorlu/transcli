#include <stdio.h>

#include "Mp3Encode.h"
#include "logger.h"
#include "StreamOutput.h"
#include "MEvaluater.h"

CMp3Encode::~CMp3Encode(void)
{
	Stop();
}

CMp3Encode::CMp3Encode(const char *outFile) : CAudioEncoder(outFile), m_pParams(NULL),
                                            m_pLeftChAudio(NULL), m_pRightChAudio(NULL),
                                            m_pMp3StreamBuf(NULL), m_mp3BufSize(0)
{
	m_logModuleType = LOGM_TS_AE_MP3;
}

bool CMp3Encode::Initialize()
{
	m_pParams = lame_init();
	if(!m_pParams) {
		logger_err(LOGM_TS_AE_MP3, "Mp3Encoder encode error: Lame init failed\n");
		return false;
	}

	// Every time passed in 1 sec data
	//lame_set_num_samples(m_pParams, m_aInfo.in_srate);
	lame_set_in_samplerate(m_pParams, m_aInfo.out_srate);
	lame_set_num_channels(m_pParams, m_aInfo.out_channels);
	lame_set_out_samplerate(m_pParams, m_aInfo.out_srate);
	
	//If original channel is 2, but output channel is 1, set MONO mode
	if(m_aInfo.out_channels == 1) {
		lame_set_mode(m_pParams, MONO);
	}

	m_bitrate = m_pXmlPrefs->GetInt("audioenc.lame.bitrate");
	if(m_bitrate <= 0) m_bitrate = 128;
	lame_set_brate(m_pParams, m_bitrate);
	lame_set_bWriteVbrTag(m_pParams, 1);
	lame_set_quality(m_pParams, 5);					//Default quality is 5
	
	switch(m_pXmlPrefs->GetInt("audioenc.lame.mode")) {
		case 0:		// VBR
			lame_set_VBR(m_pParams, vbr_mtrh);
			break;		
		case 1:		// ABR
			lame_set_VBR(m_pParams, vbr_abr);
			break;	
		case 2:		// CBR
			lame_set_VBR(m_pParams, vbr_off);
			break;
	}

	/* turn off automatic writing of ID3 tag data into mp3 stream 
	* we have to call it before 'lame_init_params', because that
	* function would spit out ID3v2 tag data.
	*/
	lame_set_write_id3tag_automatic(m_pParams, 0);

	if(lame_init_params(m_pParams) < 0) {
		logger_err(LOGM_TS_AE_MP3, "Mp3Encoder encode error: Lame init params failed\n");
		lame_close(m_pParams);
	}

	//-------------------------- Init Encode Params ---------------------------
	// Create output stream
	if(!m_strOutFile.empty()) {
		m_pStreamOut = getStreamOutput(CStreamOutput::FILE_STREAM);
		if(m_pStreamOut->Open(m_strOutFile.c_str()) == false) {
			logger_err(LOGM_TS_AE_MP3, "CMp3Encode::Initialize error: Open output stream failed\n");
			if(m_pStreamOut) destroyStreamOutput(m_pStreamOut);
			return false;
		}
	}

	// Init mp3 stream buffer
	int sampleNumPerCh = m_aInfo.out_srate/5;
	m_mp3BufSize = (int)(1.25f * sampleNumPerCh) + 7200;
	m_pMp3StreamBuf = (uint8_t*)malloc(m_mp3BufSize);
	if(!m_pMp3StreamBuf) {
		logger_err(LOGM_TS_AE_MP3, "CMp3Encode::Initialize error: Allocate mp3Stream buffer failed.\n");
		return false;
	}

	// Init Left and Right channel audio buffer
	const int samplaSize = sizeof(int16_t);
	m_pLeftChAudio  = (int16_t*)malloc(sampleNumPerCh*samplaSize);
	if(!m_pLeftChAudio) {
		logger_err(LOGM_TS_AE_MP3, "CMp3Encode::Initialize error: Allocate left channel buffer failed.\n");
		return false;
	}
	m_pRightChAudio = (int16_t*)malloc(sampleNumPerCh*samplaSize);
	if(!m_pRightChAudio) {
		logger_err(LOGM_TS_AE_MP3, "CMp3Encode::Initialize error: Allocate right channel buffer failed.\n");
		return false;
	}

	m_bClosed = 0;
	return true;
}

int64_t CMp3Encode::EncodeBuffer(uint8_t* pAudioBuf, int bufLen)
{
	int64_t encodeRet = 1;
	int bytesPerSample = m_aInfo.bits>>3;
	int channelNum = m_aInfo.out_channels;
	int samplesReadPerCh = bufLen/bytesPerSample/channelNum;
	//getStartTime();
	
	do {
		// Convert uint8 pcm buffer to int16 buffers
		if(m_aInfo.bits == 8) {
			conv8(pAudioBuf, bufLen, m_pLeftChAudio, m_pRightChAudio,  channelNum);
		} else if (m_aInfo.bits == 16) {
			conv16(pAudioBuf, bufLen, m_pLeftChAudio, m_pRightChAudio, channelNum, false);
		} else {
			logger_err(LOGM_TS_AE_MP3, "CMp3Encode::encode error, %d bits per sample not supported", m_aInfo.bits);
			encodeRet = -1;
			break; 
		}
		
		// Encode audio buffer
		int realMp3BufSize = lame_encode_buffer(m_pParams, m_pLeftChAudio, m_pRightChAudio, samplesReadPerCh, m_pMp3StreamBuf, m_mp3BufSize);
		if(realMp3BufSize > 0) {
			if(m_pStreamOut) {
				if(m_pStreamOut->Write((char*)m_pMp3StreamBuf, realMp3BufSize) < 0) {
					logger_err(LOGM_TS_AE_MP3, "CMp3Encode::encode error, write mp3buf failed");
					encodeRet = -1;
					break;
				}
				m_encodedBytes += realMp3BufSize;
				//benchmark();
				encodeRet = m_encodedBytes;
			}
		} else if (realMp3BufSize < 0){
			dumpErrInfo(realMp3BufSize);
			encodeRet = -1;
			break;
		}
	} while (false);
	
	return encodeRet;
}

bool CMp3Encode::Stop()
{
	if(m_pStreamOut) {
		destroyStreamOutput(m_pStreamOut);
		m_pStreamOut = NULL;
	}

	if(m_pParams) {
		lame_close(m_pParams);
		m_pParams = NULL;
	}
	
	if(m_pMp3StreamBuf) {
		free(m_pMp3StreamBuf);
		m_pMp3StreamBuf = NULL;
	}
	if(m_pLeftChAudio) {
		free(m_pLeftChAudio);
		m_pLeftChAudio = NULL;
	}
	if(m_pRightChAudio) {
		free(m_pRightChAudio);
		m_pRightChAudio = NULL;
	}
	return CAudioEncoder::Stop();
}

void CMp3Encode::dumpErrInfo(int errType)
{
	switch (errType) {
	case -1:
		logger_err(LOGM_TS_AE_MP3, "CMp3Encode::encode error, mp3buf was too small.\n");
		break;
	case -2:
		logger_err(LOGM_TS_AE_MP3, "CMp3Encode::encode error, malloc() problem.\n");
		break;
	case -3:
		logger_err(LOGM_TS_AE_MP3, "CMp3Encode::encode error, lame_init_params() not called.\n");
		break;
	case -4:
		logger_err(LOGM_TS_AE_MP3, "CMp3Encode::encode error, psycho acoustic problems.\n");
		break;
	default:
		logger_err(LOGM_TS_AE_MP3, "CMp3Encode::encode error.\n");
		break;
	}
}
