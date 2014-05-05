#include "bitconfig.h"

#include "fdkAacEncode.h"
#include "MEvaluater.h"
#include "StreamOutput.h"


CFdkAacEncode::CFdkAacEncode(const char* outFileName) : CAudioEncoder(outFileName),
	m_sHandle(NULL), m_pOutBuf(NULL)
{
	m_logModuleType = LOGM_TS_AE_FDK;
}

CFdkAacEncode::~CFdkAacEncode(void)
{
}

bool CFdkAacEncode::Initialize()
{
	int rcModeIdx = m_pXmlPrefs->GetInt("audioenc.fdkaac.mode");
	int proIdx = m_pXmlPrefs->GetInt("audioenc.fdkaac.profile");
	bool bEldSbr = m_pXmlPrefs->GetBoolean("audioenc.fdkaac.eldSbr");
	int channelOrder = m_pXmlPrefs->GetInt("audioenc.fdkaac.chOrder");
	int containerIdx = m_pXmlPrefs->GetInt("audioenc.fdkaac.container");
	bool afterBurner = m_pXmlPrefs->GetBoolean("audioenc.fdkaac.afterBurner");
	int signalMode = m_pXmlPrefs->GetBoolean("audioenc.fdkaac.signaling");

	m_bitrate = m_pXmlPrefs->GetInt("audioenc.fdkaac.bitrate")*1000;

	int rcMode = 0;
	if(rcModeIdx == 1) {
		rcMode = 8;
	}

	int profiles[] = {2,5,29,23,39,129,132,156};
	int profile = profiles[proIdx];

	int containers[] = {0,1,2,6,7,10};
	int container = containers[containerIdx];

	CHANNEL_MODE channelMode;
	switch (m_aInfo.out_channels) 
	{
	case 1: 
		channelMode = MODE_1;       
		break;
	case 2: 
		channelMode = MODE_2;       
		break;
	case 3: 
		channelMode = MODE_1_2;     
		break;
	case 4: 
		channelMode = MODE_1_2_1;   
		break;
	case 5: 
		channelMode = MODE_1_2_2;   
		break;
	case 6: 
		channelMode = MODE_1_2_2_1; 
		break;
	default:
		return false;
	}

	if (aacEncOpen(&m_sHandle, 0, m_aInfo.out_channels) != AACENC_OK) {
		return false;
	}

	
	if (aacEncoder_SetParam(m_sHandle, AACENC_AOT, profile) != AACENC_OK) {
		return false;
	}
	if (profile == 39 && bEldSbr) {
		if (aacEncoder_SetParam(m_sHandle, AACENC_SBR_MODE, 1) != AACENC_OK) {
			return false;
		}
	}
	if (aacEncoder_SetParam(m_sHandle, AACENC_SAMPLERATE, m_aInfo.out_srate) != AACENC_OK) {
		return false;
	}
	if (aacEncoder_SetParam(m_sHandle, AACENC_CHANNELMODE, channelMode) != AACENC_OK) {
		return false;
	}
	if (aacEncoder_SetParam(m_sHandle, AACENC_CHANNELORDER, channelOrder) != AACENC_OK) {
		return false;
	}

	if (rcMode > 0) {
		if (aacEncoder_SetParam(m_sHandle, AACENC_BITRATEMODE, rcMode) != AACENC_OK) {
			return false;
		}
	} else {
		if (aacEncoder_SetParam(m_sHandle, AACENC_BITRATE, m_bitrate) != AACENC_OK) {
			return false;
		}
	}

	if (aacEncoder_SetParam(m_sHandle, AACENC_TRANSMUX, container) != AACENC_OK) {
		return false;
	}

	if(container == 1 || container == 2) {	// ADIF or ADTS (Only support Implicit backward compatible signaling)
		signalMode = 0;
	}
	if (aacEncoder_SetParam(m_sHandle, AACENC_SIGNALING_MODE, signalMode) != AACENC_OK) {
		return false;
	}

	int iAfterBurner = afterBurner? 1 : 0;
	if (aacEncoder_SetParam(m_sHandle, AACENC_AFTERBURNER, iAfterBurner) != AACENC_OK) {
		return false;
	}

	if (aacEncEncode(m_sHandle, NULL, NULL, NULL, NULL) != AACENC_OK) {
		return false;
	}

	AACENC_InfoStruct aacInfo = {0};
	if (aacEncInfo(m_sHandle, &aacInfo) != AACENC_OK) {
		return false;
	}

	m_frameLen = m_aInfo.out_channels * aacInfo.frameLength * (m_aInfo.bits >>3) ;

	m_iOutBufLen = aacInfo.maxOutBufBytes;
	if (m_pOutBuf == NULL) {
		m_pOutBuf = (uint8_t*)malloc(m_iOutBufLen);
	}

	if(!m_strOutFile.empty()) {
		m_pStreamOut = getStreamOutput(CStreamOutput::FILE_STREAM);
		if(!m_pStreamOut->Open(m_strOutFile.c_str())) {
			logger_err(m_logModuleType, "Create output file failed.\n");
			return false;
		}
	}

	m_bClosed = 0;
	m_bitrate /= 1000;
	return true;
}

int64_t CFdkAacEncode::EncodeBuffer(uint8_t* pAudioBuf, int bufLen)
{
	AACENC_BufDesc in_buf = { 0 }, out_buf = { 0 };
	AACENC_InArgs in_args = { 0 };
	AACENC_OutArgs out_args = { 0 };
	int in_identifier = IN_AUDIO_DATA;
	int in_size, in_elem_size;
	int out_identifier = OUT_BITSTREAM_DATA;
	int out_size, out_elem_size;
	int read, i;
	void *in_ptr, *out_ptr;
	AACENC_ERROR err;

	//////这个大小端转换，不一定要转换
	/*for (i = 0; i < bufLen / 2; i++) 
	{
		const uint8_t* in = &pAudioBuf[2*i];
		m_pConverBuf[i] = in[0] | (in[1] << 8);
	}*/
	
	in_ptr = pAudioBuf;
	in_size = bufLen;
	in_elem_size = 2;

	in_args.numInSamples = bufLen/2;
	in_buf.numBufs = 1;
	in_buf.bufs = &in_ptr;
	in_buf.bufferIdentifiers = &in_identifier;
	in_buf.bufSizes = &in_size;
	in_buf.bufElSizes = &in_elem_size;
	
	out_ptr = m_pOutBuf;
	out_size = m_iOutBufLen;
	out_elem_size = 1;
	out_buf.numBufs = 1;
	out_buf.bufs = &out_ptr;
	out_buf.bufferIdentifiers = &out_identifier;
	out_buf.bufSizes = &out_size;
	out_buf.bufElSizes = &out_elem_size;

	if ((err = aacEncEncode(m_sHandle, &in_buf, &out_buf, &in_args, &out_args)) != AACENC_OK) {
		if(err == AACENC_ENCODE_EOF) {
			return 1;	// Not an error, should continue
		}
		logger_err(m_logModuleType, "Fdk encode frame failed, error code:%d.\n", err);
		return -1;
	}

	int outBytes = out_args.numOutBytes;
	if (outBytes == 0) {
		return 1;		// Not an error, should continue
	}
	
	if(m_pStreamOut && m_pStreamOut->Write((char*)m_pOutBuf, outBytes) <= 0) {
		logger_err(m_logModuleType, "Write file failed.\n");
		return -1;
	}
	m_encodedBytes += outBytes;
	return outBytes;
}

bool CFdkAacEncode::Stop()
{
	if(m_pStreamOut) {
		destroyStreamOutput(m_pStreamOut);
		m_pStreamOut = NULL;
	}

	/*if(m_pConverBuf) {
		free(m_pConverBuf);
		m_pOutBuf = NULL;
	}*/

	if(m_pOutBuf) {
		free(m_pOutBuf);
		m_pOutBuf = NULL;
	}
	if(m_sHandle) {
		aacEncClose(&m_sHandle);
		m_sHandle = NULL;
	}
	return CAudioEncoder::Stop();
}
