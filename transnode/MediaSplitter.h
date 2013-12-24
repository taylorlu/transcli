#ifndef _MEDIA_SPLITTER_H_
#include <vector>
#include <string>
#include <stdint.h>

#define CONVERT_SIZE_TO_TIME

class CFileQueue;
class CAudioEncoder;
class CVideoEncoder;
class CTransWorkerSeperate;

/** Unit of file size is Bytes (byte)
*** Unit of time is milliseconds (ms)
*/

enum SegType_t {
	SEG_TYPE_NONE = 0,
	SEG_TYPE_AVERAGE,
	SEG_TYPE_NORMAL
};

enum SegUnitType_t {
	SEG_UNIT_BYSIZE = 0,  // default value
	SEG_UNIT_BYTIME,
};

class CMediaSplitter
{
public:
	CMediaSplitter(void);
	// Configure file related functions
	void SetSrcUrl(const char* url) {if(url) m_srcUrl = url;}
	//void SetTaskPrefName(const char* prefName) {if(prefName) m_taskPrefName = prefName;}
	void AddDestSegFile(const char* destSegFile);

	void Init();
	void BindWorker(CTransWorkerSeperate* pWorker) {m_worker = pWorker;}
	void BindFileQueue(CFileQueue* pFileQueue) {m_pFileQueue = pFileQueue;}
	void BindAudioEncoder(CAudioEncoder* pAudioEnc) {m_audioEnc = pAudioEnc;}
	void BindVideoEncoder(CVideoEncoder* pVideoEnc) {m_videoEnc = pVideoEnc;}
	void SetDuration(int64_t totalMs) {m_durationMs = totalMs;}
	void SetOverallKBits(int kbits);
	void SetLastSegDeltaPercent(int percent) {m_lastSegDeltaPercent = percent;}
	void SetVideoFrameBytes(int bytes) {m_videoFrameBytes = bytes;}
	void SetMuxInfoSizeRatio(float ratio) {m_muxInfoSizeRatio = ratio;}

	void SetSegInterval(int intervalMs) {m_interval = intervalMs;}
	void SetSegmentType(SegType_t segType) {m_type = segType;}
	void SetPostfix(const char* postfix) {if(postfix) m_postfix = postfix;}
	void SetUnitType(SegUnitType_t unitType) {m_unitType = unitType;}
	void SetPrefix(const char* prefix) {m_prefix = prefix;}
	void SetMarkString(const char *strMarks);
	std::string GetPostfix() {return m_postfix;}
	std::string GetPrefix() {return m_prefix;}
	SegUnitType_t GetUnitType() {return m_unitType;}

	void UpdateAudioTime(int64_t audioTime) {m_audioTime = audioTime;}
	void UpdateVideoTime(int64_t videoTime) {m_videoTime = videoTime;}
	void UpdateAudioSize(int64_t audioBytes);
	void UpdateVideoSize(int64_t videoBytes);

	// Return true for discarding data
	bool BeginSplitAudio();

	// If reduce update interval to every time return true
	// (if increase update interval to 1 second return false)
	bool EndSplitAudio();

	// Return true for discarding data
	bool BeginSplitVideo();

	// If reduce update interval to every time return true
	// (if increase update interval to 1 second return false)
	bool EndSplitVideo();

	void GenSplitConfig();
	
	std::vector<int64_t>& GetSegmentTimes() {return m_vctSplitPointTime;}

	~CMediaSplitter(void);

private: 
	
	enum split_method {
		AVARAGE_BY_TIME,
		AVARAGE_BY_SIZE,
		NORMAL_BY_TIME,
		NORMAL_BY_SIZE
	};

	void RestartAudio();
	void RestartVideo();
	void ResetAudioClip();
	void ResetVideoClip();

	CTransWorkerSeperate*	m_worker;
	CFileQueue*		m_pFileQueue;
	CAudioEncoder*  m_audioEnc;
	CVideoEncoder*  m_videoEnc;
	split_method    m_splitMethod;

	int64_t			m_audioTime;
	int64_t			m_videoTime;
	int64_t			m_audioBytes;
	int64_t			m_videoBytes;

	int64_t			m_processedSum;
	int				m_audioClipIndex;
	int				m_videoClipIndex;
	int				m_curMuxingIdx;					// Which clip is muxed
	int64_t			m_audioClipStart;
	int64_t			m_audioClipEnd;
	int64_t			m_videoClipStart;
	int64_t			m_videoClipEnd;
	
	int64_t			m_lastAudioSegSize;
	int64_t			m_lastVideoSegSize;
	int64_t			m_lastAudioSegTime;
	int64_t			m_lastVideoSegTime;

	int             m_lastSegDeltaTime;
	int				m_lastSegDeltaPercent;

	int				m_overallKbits;
	int				m_oneSecondDataBytes;	// Two one data evaluate size in bytes
	int				m_videoFrameBytes;
	int64_t			m_audioEndTimeBySize;	// When split by size, the evaluate end time(video & audio should end at)
	int64_t			m_videoEndTimeBySize;
	int64_t			m_durationMs;

	float			m_muxInfoSizeRatio;

	int m_interval;
	SegType_t m_type;
	SegUnitType_t m_unitType;
	std::string m_prefix;
	std::string m_postfix;

	// Configure file related variables
	std::string m_srcUrl;
	//std::string m_taskPrefName;
	std::vector<std::string> m_vctSegmentFiles;
	std::vector<int64_t> m_vctSplitPointTime;

	std::vector<std::pair<int64_t, int64_t> > m_markSegs;
	//std::vector<std::string> m_vctSegTimes;
};

#define _MEDIA_SPLITTER_H_
#endif

