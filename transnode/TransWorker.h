#ifndef _TRANS_WORKER_H_
#define _TRANS_WORKER_H_

#include <string>
#include <vector>
#include "bitconfig.h"
#include "BiThread.h"
#include "FileQueue.h"
#include "StrProInc.h"
#include "bit_osdep.h"

//#define DEBUG_DUMP_RAWVIDEO
//#define DEBUG_DUMP_RAWAUDIO
#define LOWER_ENCODE_THREAD
#define UPDATE_FRAMES_INTERVAL 15

class CRootPrefs;
class CXMLPref;
class CRootPrefs;
class CDecoder;
class CWaterMarkManager;
class CWorkManager;
class CStreamOutput;
class CVideoEncoder;
class CPlaylistGenerator;

typedef void (*PFNCOMPLETEDCALLBACK)();

class CTransWorker
{
public:
	CTransWorker();
	virtual ~CTransWorker(void);
	
	//parse XML_RPC request
	virtual  bool ParseSetting() = 0;
	
	virtual int CreateTask(const char *prefs,  char* outFileName /*OUT*/);
	int  EndWork();
	virtual void CleanUp();
	
	std::string GetOutputFileName(int index);

	virtual bool Pause();
	virtual bool Resume();
	virtual bool Cancel();
	virtual bool Stop();

	void SetId(int id) {m_id = id;}				//local Id
	void SetWorkType(worker_type_t wType) {m_workType = wType;}
	worker_type_t GetWorkType() {return m_workType;}
	int  GetId() const {return m_id;}
	void SetGlobalId(int globalId) {m_uuid = globalId;}	// Global Id
	int  GetGlobalId() const {return m_uuid;}
	//virtual video_encoder_t GetVideoEncoder() {return VE_X264;}
	void SetErrorCode(error_code_t errCode) {if (m_errCode == EC_NO_ERROR) m_errCode = errCode;}
	error_code_t GetErrorCode() {return m_errCode;}

	//query information
	virtual int  GetElapsedTime() {return m_elapseSecs;}		// Elapse seconds
	virtual std::string GetSpeedString(){return m_strEncodeSpeed;}	// Get encoding speed(FPS or X) from decoding side
	virtual float GetProgress() {return m_progress;}			// Get encoding progress (percentage, ex: 56.7)
	virtual task_state_t GetState() {return m_taskState;}
	virtual task_state_t GetAndResetState();

	virtual bool GetTaskInfo(ts_task_info_t* taskInfo /*OUT*/);

	// Set task state
	void SetState(task_state_t taskState) {m_savedState = m_taskState; m_taskState = taskState;}
	__int64 GetLastActiveTime() {return m_lastTime;}
	int GetTaskDuration() {return m_tmpBenchData.mainDur;}

	bool IsUsingCUDA() {return m_bUseCudaEncoder;}
	
	void SetFinishCallbackForCli(PFNCOMPLETEDCALLBACK pCbForCli) {m_pFinishCbForCli = pCbForCli;}
protected:
	struct benchmark_data_t {
		int64_t lastFrameNum;
		int mainDur;
		int fpsNum;
		int fpsDen;
		float audioEncTime;
		float videoEncTime;
	};

	// Parse basic info such as 'inpu url', 'output dir', 'temp dir', etc.
	bool parseBasicInfo();
	bool parseDurationInfo(CXMLPref* pTaskPref, StrPro::CXML2* pMediaPref);

	// Set source file name
	bool setSrcFile(const char* srcUrl);

	// Adjust audio/video info according to original wav/yuv info
	void checkAudioParam(audio_info_t* ainfo, wav_info_t* pWav);
	void checkVideoParam(video_info_t* vinfo, yuv_info_t* pYuv);
	
	void parseMediaAudioInfoNode(StrPro::CXML2* mediaInfo, attr_audio_t* pAudioAttrib);
	void parseMediaVideoInfoNode(StrPro::CXML2* mediaInfo, attr_video_t* pVideoAttrib);

	// Ajust dest audio/video properties according to source attribute
	void setAudioEncAttrib(audio_info_t* pAInfo,CXMLPref* audioPref,attr_audio_t* pAudioAttrib);
	void setVideoEncAttrib(video_info_t* pVInfo,CXMLPref* videoPref,attr_video_t* pVideoAttrib);

	// Audio encoding thread
	bool createTranscodeAudioThread();
	static THREAD_RET_T WINAPI transcodeAudioEntry(void* transWorker);	// Entry for audio encoding
	virtual THREAD_RET_T transcodeAudio() {return 0;}
	
	// Video encoding thread
	bool createTranscodeVideoThread();
	static THREAD_RET_T WINAPI transcodeVideoEntry(void* worker);		// Entry for video encoding
	virtual THREAD_RET_T transcodeVideo() {return 0;}
	
	virtual void closeDecoders() {}

	// Auto-detection of black band
	void performBlackBandAutoDetect(const char* srcFile, CVideoEncoder* pVideoEnc);
	// Calc new output dar when crop 
	void calcDarWhenCropAndNormalizeCrop(CVideoEncoder* pVideoEnc);
	// If output width or height <0, then adjust size according to display aspect ratio
	// If output bitrate < specified, using original bitrate.
	void adjustVideoOutParam(CVideoEncoder* pVideoEnc, int overallBr);

	// Fix absolute logo position and size if output size is different from original(resize or crop)
	void normalizeLogoTimeRect(CVideoEncoder* pVideoEnc);

	// Delogo params may be relative axis value (0<axisVal<1)
	void normalizeDelogoTimeRect(CVideoEncoder* pVideoEnc);

	virtual int StartWork() = 0;

	bool validateTranscode(int decoderExitCode = -1);	// Validate a/v transcoding
	bool processLosslessClip();		// Use mp4box to split and merge.
	void ResetParams();
protected:
	int					m_frameBufSize;			// Original yuv frame buffer size
	int					m_pcmBufSize;			// Original pcm buffer size
	uint8_t*			m_yuvBuf;
	uint8_t*			m_pcmBuf;

	attr_video_t*		m_srcVideoAttrib;
	std::vector<attr_audio_t*> m_srcAudioAttribs;

	bool				m_bEnableDecoderVf;
	bool				m_bEnableDecoderAf;

	int					m_id;
	worker_type_t		m_workType;
	int					m_uuid;					// Global Id, setting by master
	
	CFileQueue			m_streamFiles;			// Queue of temp stream files

	CBIThread::Handle m_hMainThread;
	CBIThread::Handle m_hAudioThread;
	CBIThread::Handle m_hVideoThread;

	CRootPrefs*			m_pRootPref;
	error_code_t		m_errCode;

	// Benchmark & Stats Info
	int64_t             m_startTime;
	int64_t				m_endTime;
	int64_t				m_lastTime;
	int				    m_elapseSecs;
	task_state_t        m_taskState;
	task_state_t		m_savedState;
	float				m_progress;
	std::string			m_strEncodeSpeed;
	int64_t				m_encodedFrames;
	//int64_t				m_encodedPcmLen;
	benchmark_data_t    m_tmpBenchData;		// Save temp data used in benchmark, no need calculate every time

	bool				m_bUseCudaEncoder;

#ifdef DEMO_RELEASE
	CWaterMarkManager*  m_pWaterMarkMan;
#endif
	
	CPlaylistGenerator* m_pPlaylist;
	bool				m_bLosslessClip;
	int					m_logType;

	// Clip and joining(Cut the commercials)
	std::vector<int> m_clipStartSet;
	std::vector<int> m_clipEndSet;
	size_t audioClipIndex;		// Current clip index of audio transcoding(in which clip interval)
	size_t videoClipIndex;
	volatile int m_audioClipEnd;
	volatile int m_videoClipEnd;

	int m_audioFractionOfSecond;
	//std::string m_dstMd5;
	int64_t m_encAudioBytes;

	bool m_bAutoVolumeGain;		// Replay gain analyse
	float m_fVolumeNormalDB;	// Set normal volume db, standard is 89 DB, can be adjusted
	
#ifdef DEBUG_DUMP_RAWVIDEO
	CStreamOutput* m_rawvideo_output;
#endif
#ifdef DEBUG_DUMP_RAWAUDIO
	CStreamOutput* m_rawaudio_output;
#endif

	PFNCOMPLETEDCALLBACK m_pFinishCbForCli;
};

#endif
