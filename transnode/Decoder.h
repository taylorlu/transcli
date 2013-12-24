#ifndef _DECODER_H_
#define _DECODER_H_
#include <string>

#include "processwrapper.h"
#include "tsdef.h"
class CXMLPref;
class CFileQueue;

#define AUDIO_PIPE_BUFFER 1<<18
#define VIDEO_PIPE_BUFFER 1<<24
#define DECODER_START_TIME 800		// ms

class CDecoder
{
public:
	CDecoder(void);
	void SetEnableAudio(bool ifDecAudio) {m_bDecAudio = ifDecAudio;}
	void SetEnableVideo(bool ifDecVideo) {m_bDecVideo = ifDecVideo;}
	void SetEnableVf(bool ifDecoderVf)   {m_bEnableVf = ifDecoderVf;}
	void SetEanbleAf(bool ifDecoderAf)   {m_bEnableAf = ifDecoderAf;}
	void SetCopyAudio(bool ifCopy) {m_bCopyAudio = ifCopy;}
	void SetCopyVideo(bool ifCopy) {m_bCopyVideo = ifCopy;}
	void SetForceAnnexbCopy(bool bForceAnnexb) {m_bForceAnnexb = bForceAnnexb;}
	void BindVideoInfo(video_info_t* pVInfo) {m_pVInfo = pVInfo;}
	void BindVideoPref(CXMLPref* pVideoPref) {m_pVideoPref = pVideoPref;}
	void BindAudioInfo(audio_info_t* pAInfo) {m_pAInfo = pAInfo;}
	void BindAudioPref(CXMLPref* pAuidoPref) {m_pAudioPref = pAuidoPref;}
	
	void BindStreamFiles(CFileQueue* pFileQueue) {m_pFileQueue = pFileQueue;}
	void BindWorkerId(int workerId) {m_workerId = workerId;}
	
	void SetDumpFileName(const std::string& dumpFile) {m_dumpFile = dumpFile;}
	void SetLastPass(bool bLastPass) { m_bLastPass = bLastPass; }

	audio_info_t* GetAudioInfo() {return m_pAInfo;}
	video_info_t* GetVideoInfo() {return m_pVInfo;}

	int GetAudioReadHandle() {return m_fdReadAudio;}
	int GetVideoReadHandle() {return m_fdReadVideo;}
	void CloseAudioReadHandle();
	void CloseVideoReadHandle();

	// Mplayer can't exit automatically, so when reading 0 byte set end flag
	bool GetDecodingEnd() {return m_bDecodeEnd;}
	void SetDecodingEnd(bool bEnd) {m_bDecodeEnd = bEnd;}

	yuv_info_t* GetYuvInfo() {return &m_yuvinfo;}
	wav_info_t* GetWavInfo() {return &m_wavinfo;}
	virtual bool ReadAudioHeader();
	bool ReadVideoHeader();
	int ReadAudio(uint8_t* pBuf, int bufSize);
	int ReadVideo(uint8_t* pBuf, int bufSize);
	
	bool IsRunning() {return m_proc.IsProcessRunning();}
	int Wait(int timeout = 600000) {return m_proc.Wait(timeout);}
	virtual std::string GetCmdString(const char* mediaFile){return "";};
	virtual bool Start(const char* sourceFile);
	virtual void Cleanup();
	virtual ~CDecoder(void);
	void SetErrorCode(int code);
	int GetCLIPid() {return m_proc.GetPid();}
	void ResetCommandLine() {m_cmdString.clear();}
	void SetForceResample(bool bForce) {m_bForceResample = bForce;}
	int GetExitCode() {return m_exitCode;}
	//void SetDuration(int dur) {m_duration = dur;}
protected:
	virtual void AutoCropOrAddBand(const char* fileName);
	std::string GetDVDPlayCmd(const char* dvdStr); 
	std::string GetVCDPlayCmd(const char* vcdStr);
	std::string GetSubFile(const char* srcFile);

	CProcessWrapper		m_proc;
	bool				m_bDecAudio;
	bool				m_bDecVideo;
	bool				m_bEnableVf;
	bool				m_bEnableAf;
	bool				m_bCopyAudio;
	bool				m_bCopyVideo;
	bool				m_bLastPass;
	bool				m_bDecodeEnd;
	bool				m_bForceResample;
	bool				m_bForceAnnexb;
	
	audio_info_t*       m_pAInfo;
	video_info_t*		m_pVInfo;
	CXMLPref*			m_pVideoPref;
	CXMLPref*			m_pAudioPref;
	CFileQueue*			m_pFileQueue;
	int				    m_workerId;

	int					m_fdReadAudio;
	int					m_fdReadVideo;
	yuv_info_t			m_yuvinfo;
	wav_info_t			m_wavinfo;
	std::string			m_dumpFile;
	std::string			m_cmdString;
	int					m_exitCode;
	//int                 m_duration;
};

#endif
