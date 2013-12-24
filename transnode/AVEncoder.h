#ifndef _AV_ENCODER_H_
#define _AV_ENCODER_H_

#include "encoderCommon.h"
#include "BiThread.h"

class CProcessWrapper;

class CAVEncoder
{
public:
	CAVEncoder(const char* outFileName);
	virtual ~CAVEncoder(void);

	bool			GenerateAudioPipe();
	bool			GenerateVideoPipe();
	void			SetAudioInfo(const audio_info_t& aInfo, CXMLPref* pXmlPrefs);
	void			SetVideoInfo(const video_info_t& vInfo, CXMLPref* pXmlPrefs);
	void            SetMuxerPref(CXMLPref* pXmlPrefs) {m_pMuxerPrefs = pXmlPrefs;}
	virtual	bool    Initialize() = 0;
	virtual bool    Stop() {return false;};
	
	audio_info_t*	GetAudioInfo() {return &m_aInfo;}
	CXMLPref*		GetAudioPref() {return m_pAudioPrefs;}

	video_info_t*	GetVideoInfo() {return &m_vInfo;}
	CXMLPref*		GetVideoPref() {return m_pVideoPrefs;}

	void SetWorkerId(int id) {m_tid = id;}
	int  GetLogModuleType() {return m_logModuleType;}
	void SetEnableAudio(bool ifEnable) {m_bEnableAudio = ifEnable;}
	bool GetEnableAudio() {return m_bEnableAudio;}
	void SetEnableVideo(bool ifEnable) {m_bEnableVideo = ifEnable;}
	bool GetEnableVideo() {return m_bEnableVideo;}
	bool  InitWaterMark();
	std::string GetOutFileName() {return m_strOutFile;}
	void  SetSourceFile(const char* strSrcFile) {if(strSrcFile) m_strSrcFile = strSrcFile;}
	virtual CProcessWrapper* GetProcessWrapper() {return NULL;}

protected:
	CAVEncoder(void);
	std::string		m_strOutFile;
	std::string     m_strSrcFile;
	int				m_tid;
	
	audio_info_t	m_aInfo;
	CXMLPref*		m_pAudioPrefs;
	video_info_t	m_vInfo;
	CXMLPref*		m_pVideoPrefs;
	CXMLPref*       m_pMuxerPrefs;

	int				m_logModuleType;
	int64_t			m_encodedBytes;
	int				m_frameCount;

	bool			m_bEnableAudio;
	bool			m_bEnableVideo;
	int				m_yuvFrameSize;
};

#endif
