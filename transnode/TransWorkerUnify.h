#ifndef _TRANSWORKER_UNIFY_H_
#define _TRANSWORKER_UNIFY_H_

#include "TransWorker.h"

class CAVEncoder;
class CStreamPrefs;
class CWaterMarkFilter;

class CTransWorkerUnify : public CTransWorker
{
public:
	CTransWorkerUnify();
	~CTransWorkerUnify(void);
	
	bool ParseSetting();
	int  StartWork();
	void CleanUp();
	
private:
	bool SetPreset(CXMLPref* pAudioPref, CXMLPref* pVideoPref, CXMLPref* pMuxerPref);
	bool setSourceAVInfo(StrPro::CXML2* mediaInfo);
	bool initAVSrcAttrib(StrPro::CXML2* mediaInfo);
	bool initialize();
	bool initAudioEncodePart();
	bool initVideoEncodePart();
	static THREAD_RET_T WINAPI mainThread(void* transWorker);

	void fixAudioParam(audio_info_t* ainfo);
	void fixVideoParam(video_info_t* vinfo);

	void benchmark();

	CAVEncoder* m_pAVEncoder;
};

#endif
