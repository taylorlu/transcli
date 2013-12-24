#include "AVEncoder.h"
#include "logger.h"
#include "MEvaluater.h"
#include "bit_osdep.h"
#include "StreamOutput.h"

CAVEncoder::CAVEncoder(void) : m_tid(0), m_pAudioPrefs(NULL), m_pVideoPrefs(NULL), 
	m_logModuleType(LOGM_TS_AVENC), m_encodedBytes(0), m_frameCount(0),
	m_bEnableAudio(false), m_bEnableVideo(false), m_yuvFrameSize(0)
{
}

CAVEncoder::CAVEncoder(const char* outFileName) : m_tid(0), m_pAudioPrefs(NULL), m_pVideoPrefs(NULL), 
	m_logModuleType(LOGM_TS_AVENC), m_encodedBytes(0), m_frameCount(0),
	m_bEnableAudio(false), m_bEnableVideo(false), m_yuvFrameSize(0)
{
	m_strOutFile = outFileName;
}

CAVEncoder::~CAVEncoder(void)
{
	Stop();
}

void CAVEncoder::SetAudioInfo(const audio_info_t& aInfo, CXMLPref* pXmlPrefs)
{
	if(m_pAudioPrefs == NULL) {
		m_aInfo = aInfo;
		m_pAudioPrefs = pXmlPrefs;
	}
}

void CAVEncoder::SetVideoInfo(const video_info_t& vInfo, CXMLPref* pXmlPrefs)
{
	if(m_pVideoPrefs == NULL) {
		m_vInfo = vInfo;
		m_pVideoPrefs = pXmlPrefs;
	}
}

bool CAVEncoder::InitWaterMark()
{
	return true;
}
