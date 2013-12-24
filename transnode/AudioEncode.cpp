#include "AudioEncode.h"
#include "StreamOutput.h"
#include "logger.h"
#include "MEvaluater.h"
#include "bit_osdep.h"

CAudioEncoder::CAudioEncoder(const char* outFileName) : m_strOutFile(outFileName), m_pStreamOut(NULL),
                                         m_fdRead(-1), m_fdWrite(-1), m_tid(0), m_frameLen(0),
                                         m_pXmlPrefs(NULL), m_logModuleType(LOGM_TS_AE),
                                         m_encodedBytes(0), m_sourceIndex(0),
                                         m_bClosed(1), m_bitrate(0)
{
	memset(&m_aInfo, 0, sizeof(m_aInfo));
}

CAudioEncoder::~CAudioEncoder(void)
{
}

bool CAudioEncoder::GeneratePipeHandle()
{
	int pipeRdWr[2] = {-1};
	int ret = 0;
	//int bufLen = (m_aInfo.bits >> 3)*m_aInfo.in_srate*m_aInfo.in_channels;
#ifdef WIN32
    ret = _pipe(pipeRdWr, 1<<17, O_BINARY);		//128K
#else
    ret = pipe(pipeRdWr);
#endif

	if(ret == 0) {
		m_fdRead = pipeRdWr[0];
		m_fdWrite = pipeRdWr[1];
		return true;
	} 

	logger_err(LOGM_TS_AE, "CAudioEncode::generatePipeHandle failed.");
	return false;
}

void CAudioEncoder::SetAudioInfo(const audio_info_t& aInfo, CXMLPref* pXmlPrefs)
{
	if(m_pXmlPrefs == NULL) {
		m_aInfo = aInfo;
		m_pXmlPrefs = pXmlPrefs;
	}
}

void CAudioEncoder::CloseReadHandle()
{
	if(m_fdRead >= 0) {
		_close(m_fdRead);
		m_fdRead = -1;
	}
}
 
void CAudioEncoder::CloseWriteHandle()
{
	if(m_fdWrite >= 0) {
		//_commit(m_fdWrite);
		_close(m_fdWrite);
		m_fdWrite = -1;
	}
}

bool CAudioEncoder::Stop()
{
	m_bClosed = 1;
	return true;
}

// void CAudioEncoder::getStartTime()
// {
// 	if(m_startTime == 0) {
// 		m_startTime = GetTickCount();
// 	}
// }
// 
// void CAudioEncoder::benchmark()
// {
// 	static const int encode_time_limit = 10;	// 10s
// 	if(m_lastTime == 0) {
// 		m_lastTime = m_startTime;
// 	}
// 
// 	int64_t curTime = GetTickCount();
// 	float elapseTime = (curTime - m_startTime)/1000.f;
// 
// 	if( (curTime-m_lastTime)/1000 % encode_time_limit == 0) {
// 		float playTime = m_encodedBytes/float(m_aInfo.bits/8*m_aInfo.in_srate*m_aInfo.in_channels);
// 		m_curXSpeed = (playTime-m_lastPlayTime)/encode_time_limit;
// 		m_averageXSpeed = playTime/elapseTime;
// 		m_lastTime = curTime;
// 		m_lastPlayTime = playTime;
// 	}
// }
