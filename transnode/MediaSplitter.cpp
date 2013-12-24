#include "MediaSplitter.h"
#include "TransWorkerSeperate.h"
#include "AudioEncode.h"
#include "VideoEncode.h"
#include "FileQueue.h"
#include "StrProInc.h"
#include "util.h"

using namespace StrPro;

CMediaSplitter::CMediaSplitter(void): m_worker(NULL),m_pFileQueue(NULL),m_audioEnc(NULL),m_videoEnc(NULL), 
									m_splitMethod(AVARAGE_BY_TIME),m_audioTime(0),
									m_videoTime(0), m_audioBytes(0), m_videoBytes(0),
									m_audioClipIndex(0), m_videoClipIndex(0), m_curMuxingIdx(0),
									m_audioClipStart(0), m_audioClipEnd(0), m_videoClipStart(0), 
									m_videoClipEnd(0), m_lastAudioSegSize(0),
									m_lastVideoSegSize(0), m_lastAudioSegTime(0), m_lastVideoSegTime(0),
                                    m_lastSegDeltaTime(1000), m_lastSegDeltaPercent(10), m_overallKbits(0), 
									m_oneSecondDataBytes(0), m_videoFrameBytes(0),m_audioEndTimeBySize(0),
									m_videoEndTimeBySize(0), m_durationMs(0), m_muxInfoSizeRatio(0.f),
									m_interval(0), m_type(SEG_TYPE_NONE),m_unitType(SEG_UNIT_BYTIME)
{
}

CMediaSplitter::~CMediaSplitter(void)
{
}

void CMediaSplitter::SetMarkString(const char *strMarks)
{
	if (strMarks == NULL || *strMarks == '\0') return;

	const char *delim = "|";
	int64_t last_end = 0;
	char *markStr = _strdup(strMarks);
	char *tmpStr = markStr;
	m_markSegs.clear();
	char * p;
	while((p = Strsep(&tmpStr, delim)) != NULL) {
		int64_t start, end;
		char *t = strchr(p, '-');

		if (t == NULL) {
			start = end = atoi(p);
		}
		else {
			*t = '\0';
			start = atoi(p);
			end = atoi(t+1);
			if(m_unitType == SEG_UNIT_BYTIME) {			// Convert time unit from s to ms
				start *= 1000;
				end *= 1000;
			} else if(m_unitType == SEG_UNIT_BYTIME) {	// Convert size unit from kb to bytes
				start *= 1024;
				end *= 1024;
			}
		}

		if (start < last_end) start = last_end;
		if (end != 0 && start > end) break;

		m_markSegs.push_back(std::pair<int64_t, int64_t>(start, end));

		if(end == 0) break;

		last_end = end;
	}
	
	free(markStr);
}

void CMediaSplitter::Init()
{
	if(m_type == SEG_TYPE_NONE) return;
	if(m_type == SEG_TYPE_AVERAGE) {			// Average splitting
		// Init first clip (start & end)
		m_audioClipEnd = m_interval;
		if(m_unitType == SEG_UNIT_BYTIME) {
			m_splitMethod = CMediaSplitter::AVARAGE_BY_TIME;
		} else if(m_unitType == SEG_UNIT_BYSIZE) {
			#ifndef CONVERT_SIZE_TO_TIME
			m_splitMethod = CMediaSplitter::AVARAGE_BY_SIZE;
			#endif
		}
	} else if(m_type == SEG_TYPE_NORMAL) {	// Arbitary splitting
		// Init first clip (start & end)
		std::pair<int64_t, int64_t> clipPair = m_markSegs[0];
		m_audioClipStart = clipPair.first;
		m_audioClipEnd = clipPair.second;
		if(m_unitType == SEG_UNIT_BYTIME) {
			m_splitMethod = CMediaSplitter::NORMAL_BY_TIME;
		} else if(m_unitType == SEG_UNIT_BYSIZE) {
			#ifdef CONVERT_SIZE_TO_TIME
			m_splitMethod = CMediaSplitter::AVARAGE_BY_TIME;
			#else
			m_splitMethod = CMediaSplitter::NORMAL_BY_SIZE;
			#endif
		}
	}
	m_videoClipStart = m_audioClipStart;
	m_videoClipEnd = m_audioClipEnd;
	if(m_splitMethod == CMediaSplitter::AVARAGE_BY_TIME) {
		m_lastSegDeltaTime = m_interval/100*m_lastSegDeltaPercent;
	} else {
		m_lastSegDeltaTime = 0;
	}
}

void CMediaSplitter::SetOverallKBits(int kbits)
{
	m_overallKbits = kbits;
	m_oneSecondDataBytes = (int)(kbits*125*(1+m_muxInfoSizeRatio)); // kbits/8*1000, convert to bytes
	#ifdef CONVERT_SIZE_TO_TIME
	if(m_unitType == SEG_UNIT_BYSIZE) {
		m_unitType = SEG_UNIT_BYTIME;
		float size2msFactor = 1000.f/m_oneSecondDataBytes;
		m_videoClipStart = (int)(m_videoClipStart*size2msFactor);	// To ms
		m_videoClipEnd = (int)(m_videoClipEnd*size2msFactor);	// To ms
		m_audioClipStart = m_videoClipStart;
		m_audioClipEnd = m_videoClipEnd;
		if(m_type == SEG_TYPE_AVERAGE) {
			int interval = (int)(m_interval*size2msFactor);
			m_interval = interval;
		} else {
			for(size_t i=0; i<m_markSegs.size(); ++i) {
				m_markSegs[i].second = (int)(m_markSegs[i].second*size2msFactor);
			}
		}
	}
	#endif
}

void CMediaSplitter::AddDestSegFile(const char* destSegFile)
{
	if(/*m_prefs->EanbleConfigGenerate() &&*/ destSegFile) {
		m_vctSegmentFiles.push_back(destSegFile);
	}
}

bool CMediaSplitter::BeginSplitAudio()
{
	if(!m_audioEnc) return false;
	int64_t curAudioTimeOrSize = m_audioTime;
	if(m_unitType == SEG_UNIT_BYSIZE) {
		curAudioTimeOrSize = m_audioBytes;
	}

	// If current audio time or size is less than clip start, then discard data
	if(curAudioTimeOrSize < m_audioClipStart) return true;
	
	return false;
}

#define UPDATE_INTERVAL_HTRESHOLD 0.7f
//#define LAST_SEG_DELTA_TIME 1000

bool CMediaSplitter::EndSplitAudio()
{
	if(!m_audioEnc) return false;
	bool splitRet = false;		// update interval is 1 second
	bool bRestart = false;
	if(m_unitType == SEG_UNIT_BYTIME) { // Time reach clip end
		if (m_audioTime-m_lastAudioSegTime >= (int64_t)((m_audioClipEnd-m_audioClipStart)*UPDATE_INTERVAL_HTRESHOLD)) {
			splitRet = true;
		}
		// If current time bigger than clip end restart(If less than 1s left then don't restart)
		if(m_audioTime >= m_audioClipEnd && m_durationMs-m_audioTime >= m_lastSegDeltaTime) bRestart = true;
	} else if(m_unitType == SEG_UNIT_BYSIZE) {
		if(!m_videoEnc) {	// If there is no video, compare size immediately
			if (m_audioBytes-m_lastAudioSegSize >= (int64_t)((m_audioClipEnd-m_audioClipStart)*UPDATE_INTERVAL_HTRESHOLD)) {
				splitRet = true;
			}
			// If current size bigger than clip end restart(If less than 200Ms left then don't restart)
			if(m_audioBytes >= m_audioClipEnd && m_durationMs-m_audioTime >= m_lastSegDeltaTime) bRestart = true;
		} else {
			int predictSize = (int)(m_oneSecondDataBytes*2.2f);
			int videoDelayFrames = m_videoEnc->GetDelayedFrames();
			if(videoDelayFrames > 0) {
				predictSize = videoDelayFrames*m_videoFrameBytes+m_videoFrameBytes;
			}
			if(m_audioBytes+m_videoBytes+predictSize-m_lastAudioSegSize-m_lastVideoSegSize >= (int64_t)((m_audioClipEnd-m_audioClipStart)*UPDATE_INTERVAL_HTRESHOLD)) {
				splitRet = true;
			}

			// If there is video, we should compare size and evaluate the clip end time,
			// both audio and video should end at the time
			if(m_audioEndTimeBySize > 0) {
				if(m_audioTime >= m_audioEndTimeBySize && m_durationMs-m_audioTime >= m_lastSegDeltaTime) bRestart = true;
			} else if(m_audioBytes+m_videoBytes+predictSize >= m_audioClipEnd) {	// Video has delay frames
				//int64_t curTime = m_audioTime > m_videoTime ? m_audioTime : m_videoTime;
				m_audioEndTimeBySize = m_audioTime + 800;	// 200 ms is allowable error
				m_videoEndTimeBySize = m_audioEndTimeBySize;
			}
		}
	}

	if(bRestart) {
		splitRet = false;
		RestartAudio();
		m_audioEndTimeBySize = 0;
		bool bDomux = false;
		if(!m_videoEnc) {	// If there is no video
			bDomux = true;
		} else if(m_audioClipIndex <= m_videoClipIndex && m_audioClipIndex-1 == m_curMuxingIdx) {
			// If there is video, should increase "m_curMuxingIdx", ensure muxing is mutually exclusive
			m_curMuxingIdx++;
			bDomux = true;
		}
		if(bDomux) {
			m_vctSplitPointTime.push_back(m_audioTime);
			m_worker->doMux();
		}
	}
	
	return splitRet;
}

bool CMediaSplitter::BeginSplitVideo()
{
	if(!m_videoEnc) return false;
	int64_t curVideoTimeOrSize = m_videoTime;
	if(m_unitType == SEG_UNIT_BYSIZE) {
		curVideoTimeOrSize = m_videoBytes;
	}

	// If current video time or size is less than clip start, then discard data
	if(curVideoTimeOrSize < m_videoClipStart) return true;

	return false;
}

bool CMediaSplitter::EndSplitVideo()
{
	if(!m_videoEnc) return false;

	bool bRestart = false;
	bool splitRet = false;
	if(m_unitType == SEG_UNIT_BYTIME) { // Time reach clip end
		if(m_videoTime-m_lastVideoSegTime >= (int64_t)((m_videoClipEnd-m_videoClipStart)*UPDATE_INTERVAL_HTRESHOLD)) {
			splitRet = true;
		}
		// If current time bigger than clip end restart(If less than 200Ms left then don't restart)
		if(m_videoTime >= m_videoClipEnd && m_durationMs-m_videoTime >= m_lastSegDeltaTime) bRestart = true;
	} else if(m_unitType == SEG_UNIT_BYSIZE) {
		int predictSize = (int)(m_oneSecondDataBytes*2.2f);
		int videoDelayFrames = m_videoEnc->GetDelayedFrames();
		if(videoDelayFrames > 0) {
			predictSize = videoDelayFrames*m_videoFrameBytes+m_oneSecondDataBytes;
		}

		if(!m_audioEnc) {	// If there is no audio, compare size immediately
			if(m_videoBytes+predictSize-m_lastVideoSegSize >= (int64_t)((m_videoClipEnd-m_videoClipStart)*UPDATE_INTERVAL_HTRESHOLD)) {
				splitRet = true;
			}
			// If current size bigger than clip end restart(If less than 200Ms left then don't restart)
			if(m_videoBytes+predictSize >= m_videoClipEnd && m_durationMs-m_videoTime >= m_lastSegDeltaTime) bRestart = true;
		} else {
			if(m_audioBytes+m_videoBytes+predictSize-m_lastAudioSegSize-m_lastVideoSegSize >= (int64_t)((m_videoClipEnd-m_videoClipStart)*UPDATE_INTERVAL_HTRESHOLD)) {
				splitRet = true;
			}
			
			// If there is audio, we should compare size and evaluate the clip end time,
			// both audio and video should end at the time
			if(m_videoEndTimeBySize > 0) {
				// If current size bigger than clip end restart(If less than 200Ms left then don't restart)
				if(m_videoTime >= m_videoEndTimeBySize && m_durationMs-m_videoTime >= m_lastSegDeltaTime) bRestart = true;
			} else if(m_audioBytes+m_videoBytes+predictSize >= m_videoClipEnd) {	// Video has delay frames
				//int64_t curTime = m_audioTime > m_videoTime ? m_audioTime : m_videoTime;
				m_videoEndTimeBySize = m_videoTime+800;	// 200 ms is allowable error
				m_audioEndTimeBySize = m_videoEndTimeBySize;
			}
		}
	}

	if(bRestart) {
		splitRet = false;
		RestartVideo();
		m_videoEndTimeBySize = 0;
		bool bDomux = false;
		if(!m_audioEnc) {	// If there is no audio
			bDomux = true;
		} else if(m_videoClipIndex <= m_audioClipIndex && m_videoClipIndex-1 == m_curMuxingIdx) {
			// If there is audio, should increase "m_curMuxingIdx", ensure muxing is mutually exclusive
			m_curMuxingIdx++;
			bDomux = true;
		}
		if(bDomux) {
			m_vctSplitPointTime.push_back(m_videoTime);
			m_worker->doMux();
		}
	}

	return splitRet;
}

void CMediaSplitter::RestartAudio()
{
	if(!m_audioEnc) return;
	// Stop encoder and reset its output file and restart
	m_audioEnc->Stop();
	std::string tmpFile = m_pFileQueue->GetStreamFileName(ST_AUDIO, m_audioEnc->GetAudioInfo()->format);
	m_audioEnc->SetOutFileName(tmpFile.c_str());
	m_pFileQueue->AddAudioSegFile(tmpFile);
#if PRODUCT_ID == PRODUCT_ID_MEDIACODER_DEVICE
	Sleep(1500);
#endif

	m_audioEnc->Initialize();
	// Update audio clip's start & end
	ResetAudioClip();
}

void CMediaSplitter::RestartVideo()
{
	if(!m_videoEnc) return;
	// Stop encoder and reset its output file and restart
	m_videoEnc->Stop();
	std::string tmpFile = m_pFileQueue->GetStreamFileName(ST_VIDEO, m_videoEnc->GetVideoInfo()->format);
	m_pFileQueue->AddVideoSegFile(tmpFile);
	m_videoEnc->SetOutFileName(tmpFile.c_str());
#if PRODUCT_ID == PRODUCT_ID_MEDIACODER_DEVICE
	Sleep(1500);
#endif
	m_videoEnc->Initialize();
	// Update video clip's start & end
	ResetVideoClip();
}

void CMediaSplitter::ResetAudioClip()
{
	m_audioClipIndex++;

	// Update audio clip's start & end
	if (m_type == SEG_TYPE_AVERAGE) {
		if(m_unitType == SEG_UNIT_BYSIZE) {
			m_audioClipStart = m_audioBytes;
			// Save last audio temp file size
			m_lastAudioSegSize = m_audioBytes;
		} else if(m_unitType == SEG_UNIT_BYTIME) {
			m_lastAudioSegTime = m_audioTime;
			m_audioClipStart = m_audioClipEnd;
		}
		m_audioClipEnd += m_interval;
	} else if(m_type == SEG_TYPE_NORMAL) {
		std::pair<int64_t, int64_t> curPair = m_markSegs.at(m_audioClipIndex);
		if(m_unitType == SEG_UNIT_BYSIZE) {
			m_audioClipStart = m_audioBytes;
			// Save last audio temp file size
			m_lastAudioSegSize = m_audioBytes;
		} else if(m_unitType == SEG_UNIT_BYTIME) {
			m_audioClipStart = curPair.first;
			m_lastAudioSegTime = m_audioTime;
		}
		m_audioClipEnd = curPair.second;
		if(m_audioClipEnd == 0) {
			m_audioClipEnd = m_durationMs;
		}
	}

	// Save last audio temp file size
	if(m_unitType == SEG_UNIT_BYSIZE) {
		m_lastAudioSegSize = m_audioBytes;
	}
}

void CMediaSplitter::ResetVideoClip()
{
	m_videoClipIndex++;

	// Update video clip's start & end
	if (m_type == SEG_TYPE_AVERAGE) {
		if(m_unitType == SEG_UNIT_BYSIZE) {
			m_videoClipStart = m_videoBytes;
			// Save last video temp file size
			m_lastVideoSegSize = m_videoBytes;
		} else if(m_unitType == SEG_UNIT_BYTIME) {
			m_videoClipStart = m_videoClipEnd;
			m_lastVideoSegTime = m_videoTime;
		}
		m_videoClipEnd += m_interval;
	} else if(m_type == SEG_TYPE_NORMAL) {
		std::pair<int64_t, int64_t> curPair = m_markSegs.at(m_videoClipIndex);
		if(m_unitType == SEG_UNIT_BYSIZE) {
			m_videoClipStart = m_videoBytes;
			// Save last video temp file size
			m_lastVideoSegSize = m_videoBytes;
		} else if(m_unitType == SEG_UNIT_BYTIME) {
			m_videoClipStart = curPair.first;
			m_lastVideoSegTime = m_videoTime;
		}
		m_videoClipEnd = curPair.second;
		if(m_videoClipEnd == 0) {
			m_videoClipEnd = m_durationMs;
		}
	}
}

void CMediaSplitter::UpdateAudioSize(int64_t audioBytes)
{
	// Because each clip is a standalone file, so total size should add previous clips files. 
	if(m_videoEnc) audioBytes += (int64_t)(audioBytes*m_muxInfoSizeRatio);
	m_audioBytes = audioBytes + m_lastAudioSegSize;
}

void CMediaSplitter::UpdateVideoSize(int64_t videoBytes)
{
	// Because each clip is a standalone file, so total size should add previous clips files. 
	videoBytes += (int64_t)(videoBytes*m_muxInfoSizeRatio);
	m_videoBytes = videoBytes + m_lastVideoSegSize;
}

void CMediaSplitter::GenSplitConfig()
{
	//if(!m_prefs->EanbleConfigGenerate()) return;

	std::string configXml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?> <root> <version>0x000010</version>";
	// Input url
	configXml += "<input> <url>";
	configXml += m_srcUrl + "</url> </input>";
	
	if(m_vctSplitPointTime.size() < m_vctSegmentFiles.size()) {
		m_vctSplitPointTime.push_back(m_durationMs);
	}

	// Output urls
	configXml += "<output> <files> ";
	for (size_t i=0; i<m_vctSegmentFiles.size(); ++i) {
		configXml += "<file splitTime=\"";
		if(i<m_vctSplitPointTime.size()) {	// Add split time to target file
			std::string timeStr = SecondsToTimeString((int)(m_vctSplitPointTime[i]/1000));
			configXml += timeStr;
		}
		configXml += "\">";
		configXml += m_vctSegmentFiles[i] + "</file>";
	}
	configXml += "</files>";

	// Preference name
	//configXml += "<prefname>";
	//configXml += m_taskPrefName + "</prefname>";

	// Split info string
	//configXml += m_prefs->GetSplitInfoXml();
	configXml += "</output> </root>";

	// Generate configure file name & save file
	if(!m_vctSegmentFiles.empty()) {
		std::string configFile;
		std::string firstDestFile = m_vctSegmentFiles[0];
		std::string dir, title, fileExt;
		if(StrHelper::splitFileName(firstDestFile.c_str(), dir, title, fileExt)) {
			// Remove the subfix of title (index or time)
			size_t splitterIdx = title.find_last_of('-');
			if(splitterIdx != title.npos) title = title.substr(0, splitterIdx);
			configFile = dir+title+".config.xml";
		}

		if(!configFile.empty()) {
			// Remove the subfix of title (index or time)
			FILE* fp = ts_fopen(configFile.c_str(), "w");
			if(!fp) return;

			CCharset strConvert;
			char* utf8Xml = strConvert.ANSItoUTF8(configXml.c_str());
			if(utf8Xml) {
				fwrite(utf8Xml, strlen(utf8Xml), 1, fp);
				free(utf8Xml);
			}
			fclose(fp);
		}
	}
}
