#include "bitconfig.h"

#ifdef HAVE_LIBFAAC
#include "FaacEncode.h"
#include "MEvaluater.h"
#include "StreamOutput.h"

#if defined(WIN32) && defined(HAVE_GPL)
#pragma comment(lib,"libfaac.lib")
#endif

CFaacEncode::CFaacEncode(const char* outFileName) : CAudioEncoder(outFileName), m_hEncoder(NULL),
m_samplesInput(0), m_maxBytesOutput(0)
{
	m_logModuleType = LOGM_TS_AE_FAAC;
}

CFaacEncode::~CFaacEncode(void)
{
}

bool CFaacEncode::Initialize()
{
	// Check version
	char *faac_id_string;
	char *faac_copyright_string;
	if (faacEncGetVersion(&faac_id_string, &faac_copyright_string) != FAAC_CFG_VERSION) {
		logger_err(m_logModuleType, "wrong libfaac version\n");
		return false;
	}
	
	faacEncConfigurationPtr fconfig = NULL;
	int rcMode = m_pXmlPrefs->GetInt("audioenc.faac.mode");
	unsigned long quantqual = m_pXmlPrefs->GetInt("audioenc.faac.quality");
	m_bitrate    = m_pXmlPrefs->GetInt("audioenc.faac.bitrate")*1000;
	unsigned int objectType = m_pXmlPrefs->GetInt("audioenc.faac.object");
	unsigned int mpegVersion = m_pXmlPrefs->GetInt("audioenc.faac.version") ? 0: 1;
	unsigned int useTns = m_pXmlPrefs->GetBoolean("audioenc.faac.tns") ? 1 : 0;
	unsigned int useMidSide = m_pXmlPrefs->GetBoolean("audioenc.faac.noMidSide") ? 0 : 1;
	int streamFormat = m_pXmlPrefs->GetBoolean("audioenc.faac.raw") ? 0 : 1;
	//int resampleRate = m_aInfo.out_srate;
	
	m_hEncoder = faacEncOpen(m_aInfo.out_srate, m_aInfo.out_channels, &m_samplesInput, &m_maxBytesOutput);
	if(!m_hEncoder) {
		logger_err(m_logModuleType, "Faac encoder open failed.\n");
		return false;
	}
	
	//m_pRawBuf = (float *)malloc(m_samplesInput*sizeof(float));
	m_frameLen = m_samplesInput*(m_aInfo.bits>>3);		// m_samplesInput is related to frame length
	m_pOutBuf = (uint8_t*)malloc(m_maxBytesOutput);

	fconfig = faacEncGetCurrentConfiguration(m_hEncoder);
	if(!fconfig) {
		logger_err(m_logModuleType, "Create configuration failed.\n");
		return false;
	}

	fconfig->mpegVersion = mpegVersion;

	switch (objectType) {
		case 0:				// Main
			fconfig->aacObjectType = MAIN;
			break;
		case 1:				// Low (LC)
			fconfig->aacObjectType = LOW;
			break;
		case 3:				// LTP
			fconfig->aacObjectType = LTP;
			fconfig->mpegVersion = MPEG4;
			break;
		default:
			logger_err(m_logModuleType, "Unrecognized object type: %d.\n", objectType);
			break;
	}
	
	fconfig->useTns = useTns;
	fconfig->shortctl = SHORTCTL_NOSHORT;		// FIX ME (Should be set outside, SHORTCTL_NOLONG)
	if(m_aInfo.out_channels >= 6) {
		fconfig->useLfe = 1;
	}
	fconfig->allowMidside = useMidSide;
	
	if(m_bitrate > 0 && rcMode == 1) {
		fconfig->bitRate = m_bitrate/m_aInfo.out_channels;
		fconfig->quantqual = 0;
	}
	if (quantqual > 0 && rcMode == 0) {
		fconfig->quantqual = quantqual;
		fconfig->bitRate = 0;
	}
	fconfig->outputFormat = streamFormat;
	if(m_aInfo.bits == 16) {
		fconfig->inputFormat = FAAC_INPUT_16BIT;
	} else {
		fconfig->inputFormat = FAAC_INPUT_32BIT;
	}

	fconfig->bandWidth = 0;
	if (!faacEncSetConfiguration(m_hEncoder, fconfig)) {
		logger_err(m_logModuleType, "Set configuration failed, unsupported output format.\n");
		return false;
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

void remapChannel(uint8_t* pAudioBuf, int bufLen) 
{
	// If there is 6 channel, remap channel layout, faac's layout is wrong
	int16_t* pBuf = (int16_t*)(pAudioBuf);
	int16_t tmpBlock[6] = {0};
	if(bufLen%12 != 0) bufLen -= bufLen%12;
	for(int i=0; i<bufLen/2; i+=6) {
		// Standard wav 5.1 layout is FL FR FC LFE SL SR
		// FAAC wrong layout maybe    FC FL FR SL SR LFE
		tmpBlock[0] = pBuf[i+2];
		tmpBlock[1] = pBuf[i];
		tmpBlock[2] = pBuf[i+1];
		tmpBlock[3] = pBuf[i+4];
		tmpBlock[4] = pBuf[i+5];
		tmpBlock[5] = pBuf[i+3];
		memcpy(pBuf+i, tmpBlock, 12);
	}
}

int64_t CFaacEncode::EncodeBuffer(uint8_t* pAudioBuf, int bufLen)
{
	int64_t encodeRet = -1;
	uint32_t  sampleSize = m_aInfo.bits>>3;
	uint32_t  totalSamples  = bufLen / sampleSize;
	uint32_t  processedSamples = 0;
	if(m_aInfo.out_channels == 6) {
		remapChannel(pAudioBuf, totalSamples*sampleSize);
	}
	while (processedSamples < totalSamples) {
		int inSamples = (totalSamples - processedSamples < m_samplesInput) 
			? totalSamples - processedSamples : m_samplesInput;

		uint8_t* curBuf = pAudioBuf + processedSamples*sampleSize;
		int outBytes = faacEncEncode(m_hEncoder, (int32_t*)(curBuf), inSamples, m_pOutBuf, m_maxBytesOutput);
		if(outBytes > 0) {
			if(m_pStreamOut && m_pStreamOut->Write((char*)m_pOutBuf, outBytes) <= 0) {
				encodeRet = -1;
				logger_err(m_logModuleType, "Write file failed.\n");
				break;
			}
			m_encodedBytes += outBytes;
			encodeRet = m_encodedBytes;
			processedSamples += inSamples;
		} else if(outBytes == 0){
			continue;
		} else {
			encodeRet = -1;
			logger_err(m_logModuleType, "Faac encode failed.\n");
			break;
		}
	}
	//benchmark();
	return encodeRet;
}

bool CFaacEncode::Stop()
{
	if(m_hEncoder) {
		 faacEncClose(m_hEncoder);
		 m_hEncoder = NULL;
	}

	if(m_pStreamOut) {
		destroyStreamOutput(m_pStreamOut);
		m_pStreamOut = NULL;
	}

	// Write handle is closed in trans worker
	if(m_fdRead > 0) {
		_close(m_fdRead);
		m_fdRead = -1;
	}

	if(m_pOutBuf) {
		free(m_pOutBuf);
		m_pOutBuf = NULL;
	}

	return CAudioEncoder::Stop();
}

#endif
