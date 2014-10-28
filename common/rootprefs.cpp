#ifdef WIN32
#include <windows.h>
#include <io.h>
#else
#include <sys/times.h>
#endif
#include <stdio.h>
#include <fcntl.h>	
#include <sys/stat.h>
#include <list>

#include "MediaTools.h"
#include "MEvaluater.h"
#include "logger.h"
#include "bit_osdep.h"
#include "bitconfig.h"
#include "util.h"

using namespace StrPro;
using namespace std;

CRootPrefs::CRootPrefs()
{
	m_pStrUrl = NULL;
	m_pStrTempDir = NULL;
	m_pStrSubTitle = NULL;
	m_pFileTitle = NULL;
	m_pStrDestDir = NULL;

	m_outputs = NULL;
	m_pMainMediaInfo = NULL;
	m_bDeleteMainMediaInfo = false;

	m_prefsDoc = NULL;
	m_taskPriority = 2;		// Normal
	m_mainUrlIndex = 0;
}

CRootPrefs::~CRootPrefs()
{
	if(m_pStrUrl) free(m_pStrUrl);
	m_pStrUrl = NULL;
	if(m_pStrTempDir) free(m_pStrTempDir);
	m_pStrTempDir = NULL;
	if(m_pStrSubTitle) free(m_pStrSubTitle);
	m_pStrSubTitle = NULL;
	
	if(m_pStrDestDir) free(m_pStrDestDir);
	m_pStrDestDir = NULL;
	if(m_pFileTitle) free(m_pFileTitle);
	m_pFileTitle = NULL;
	
	if(m_bDeleteMainMediaInfo && m_pMainMediaInfo) {
		delete m_pMainMediaInfo;
		m_pMainMediaInfo = NULL;
	}
	if (m_outputs) {
		delete m_outputs;
		m_outputs = NULL;
	}
	
	if (m_prefsDoc) {
		delete m_prefsDoc;
		m_prefsDoc = NULL;
	}
	
	clearMediaInfos();
}

#define TEMPCONFIG "<config>\
<node key=\"playlist.enable\">false</node>\
<node key=\"playlist.name\">stream</node>\
<node key=\"playlist.type\">.m3u8</node>\
<node key=\"playlist.top\">true</node>\
<node key=\"playlist.end\">true</node>\
<node key=\"playlist.live\">false</node>\
<node key=\"playlist.live.size\">0</node>\
<node key=\"playlist.live.duration\">10</node>\
</config>"

static bool modifyEpisodeParam(StrPro::CXML2& prefDoc, int videoBr)
{
	// Modify bitrate
	void* videoBrStr = prefDoc.findChildNode("node", "key", "overall.video.bitrate");
	if(videoBrStr) {
		prefDoc.setNodeValue(videoBr-videoBr/10);
	} else {
		void* newNode = prefDoc.addChild("node", videoBr-videoBr/10);
		prefDoc.SetCurrentNode(newNode);
		prefDoc.addAttribute("key", "overall.video.bitrate");
	}
	prefDoc.goParent();

	// Modify psy_trellis
	void* psyTrillis = prefDoc.findChildNode("node", "key", "videoenc.x264.psy_trellis");
	if(psyTrillis) {
		prefDoc.setNodeValue(0);
	} else {
		void* newNode = prefDoc.addChild("node", 0);
		prefDoc.SetCurrentNode(newNode);
		prefDoc.addAttribute("key", "videoenc.x264.psy_trellis");
	}
	prefDoc.goParent();
	return true;
}

struct mbrlevel_t {
    const char* prefix;
    int wMax, wMin, bMax, bMin;	// width and bitrate range
};

static bool processMBR(StrPro::CXML2* pMediaInfo, StrPro::CXML2& prefDoc)
{
	// Get current input media's width and bitrate
	int inputWidth = 0, inputBr = 0;
	int inputDur = 0;
	if(pMediaInfo) {
		if(pMediaInfo->goToKey("/mediainfo/general/bitrate")) {
			inputBr = pMediaInfo->getNodeValueInt()/1000;
		} else if (pMediaInfo->goToKey("/mediainfo/video/bitrate")) {
			inputBr = pMediaInfo->getNodeValueInt()/1000;
			if(pMediaInfo->goToKey("/mediainfo/audio/bitrate")) {
				inputBr += pMediaInfo->getNodeValueInt()/1000;
			}
		}

		if (pMediaInfo->goToKey("/mediainfo/video/width")) {
			inputWidth = pMediaInfo->getNodeValueInt();
		}

		if (pMediaInfo->goToKey("/mediainfo/general/duration")) {
			inputDur = pMediaInfo->getNodeValueInt();
		}
	}
	
	if(prefDoc.goToKey("/root/output/presets") && prefDoc.getChildCount() > 2) {
		StrPro::CXML2 mbrDoc;
		if(mbrDoc.Open("pptvMbr.xml") == 0 && mbrDoc.goToKey("/pptvmbr/mbrset")) {
			// Parse mbr levels constrain
			std::vector<mbrlevel_t> mbrSet;
			void* levelNode = mbrDoc.findChildNode("level");
			while(levelNode) {
				mbrlevel_t curLevel = {0, 0, 0, 0, 0};
				curLevel.prefix = mbrDoc.getAttribute("prefix");
				curLevel.bMin = mbrDoc.getAttributeInt("brmin");
				curLevel.bMax = mbrDoc.getAttributeInt("brmax");
				curLevel.wMin = mbrDoc.getAttributeInt("widthmin");
				curLevel.wMax = mbrDoc.getAttributeInt("widthmax");
				mbrSet.push_back(curLevel);
				levelNode = mbrDoc.findNextNode("level");
			}

			// Find level of input file 
			int inputLevelIndex = -1;	
			for(size_t mbrIdx = 0; mbrIdx < mbrSet.size(); ++mbrIdx) {
				if(inputBr > mbrSet[mbrIdx].bMin && inputBr <= mbrSet[mbrIdx].bMax) {
					inputLevelIndex = mbrIdx;
					break;
				}
			}

			// input file is under each level, no need to transcode
			if(inputLevelIndex < 0 && inputBr < mbrSet[0].bMax) {	
				logger_err(LOGM_UTIL_ROOTPREFS, "Bitrate level of input file is illegal, no need to transcode.\n");
				return false;
			}

			// Parse mbr presets in streamsPrefs
			void* presetNode = prefDoc.findChildNode("MediaCoderPrefs");
			std::vector<int> removePresetIds;
			while (presetNode) {
				const char* presetId = prefDoc.getAttribute("id");
				if(!presetId) {
					presetNode = prefDoc.findNextNode("MediaCoderPrefs");
					continue;
				}
				int videoBr = prefDoc.getChildNodeValueInt("node", "key", "overall.video.bitrate");
				int audioBr = prefDoc.getChildNodeValueInt("node", "key", "audioenc.nero.bitrate");
				int presetBr = videoBr + audioBr;
				
				// Judge whehther this preset is above or equal to current input bitrate level
				if(presetBr > mbrSet[inputLevelIndex].bMin) {	// the preset is above or equal to input level, no need to transcode, should remove
					removePresetIds.push_back(atoi(presetId));
				} else {	// Modify name rules of the preset
					void* bEnableNameRule = prefDoc.findChildNode("node", "key", "overall.output.formatting");
					if(bEnableNameRule) {
						prefDoc.setNodeValue("true");
						prefDoc.goParent();
					} else {
						bEnableNameRule = prefDoc.addChild("node", "true");
						if(bEnableNameRule) {
							prefDoc.SetCurrentNode(bEnableNameRule);
							prefDoc.addAttribute("key", "overall.output.formatting");
							prefDoc.goParent();
						}
					}
					void* nameFormatting = prefDoc.findChildNode("node", "key", "overall.output.fileNameFormat");
					if(!nameFormatting) {
						void* formatNode = prefDoc.addChild("node", "$(SourceRelativeDir)\\$(SourceFileName)");
						prefDoc.SetCurrentNode(formatNode);
						prefDoc.addAttribute("key", "overall.output.fileNameFormat");
					}

					//logger_err(LOGM_UTIL_ROOTPREFS, "Original name format:%s\n", nameFormatting);
					const char* formatString = prefDoc.getNodeValue();
					if(formatString) {
						std::string modifiedFormat = formatString;
						size_t fileNameIdx = modifiedFormat.find("$(SourceFileName)");
						if(fileNameIdx != std::string::npos) {
							std::string prefixString = "[";
							// Find this preset level
							size_t i = 0;
							for(; i < mbrSet.size(); ++i) {
								if(presetBr > mbrSet[i].bMin && presetBr <= mbrSet[i].bMax) {
									break;
								}
							}
							if(i < mbrSet.size()) {
								prefixString += mbrSet[i].prefix;
								prefixString += "]";
								modifiedFormat.insert(fileNameIdx, prefixString.c_str());
							}
						}
						prefDoc.setNodeValue(modifiedFormat.c_str());
					}
					prefDoc.goParent();

					// Set params about encoding by type
					if(inputDur < 3600000 && videoBr < 700) {	// HD or SD Episode encoding by type
						// Input file duration < 1 hour, will be TV show
						// and preset bitrate < 700, the preset is HD or SD
						modifyEpisodeParam(prefDoc, videoBr);
					}
				}
				presetNode = prefDoc.findNextNode("MediaCoderPrefs");
			}

			//logger_err(LOGM_UTIL_ROOTPREFS, "Remove preset num:%d\n", removePresetIds.size());

			// Remove levels above or equal to input level
			prefDoc.goToKey("/root/output/presets");
			for(size_t idx=0; idx<removePresetIds.size(); ++idx) {
				char presetIdStr[8] = {0};
				_snprintf(presetIdStr, 7, "%d", removePresetIds[idx]);  
				if(prefDoc.findChildNode("MediaCoderPrefs", "id", presetIdStr)) {
					prefDoc.removeNode();
				}
			}
			// Remove streams that use the removed preset
			prefDoc.goToKey("/root/output");
			void* streamNode = prefDoc.findChildNode("stream");
			while(streamNode) {
				const char* pid = prefDoc.getAttribute("pid");
				bool bRemoveCurStream = false;
				if(pid) {
					for(size_t j=0; j<removePresetIds.size(); ++j) {
						if(atoi(pid) == removePresetIds[j]) {
							bRemoveCurStream = true;
							break;
						}
					}
				}
				if(bRemoveCurStream) {
					prefDoc.removeNode();
					streamNode = prefDoc.findChildNode("stream");
				} else {
					streamNode = prefDoc.findNextNode("stream");
				}
			}

			// Modify audio/video stream id of muxers
			prefDoc.goToKey("/root/output");
			std::map<int, int> audioIdAndIndexMap;
			void* audioStreamNode = prefDoc.findChildNode("stream", "type", "audio");
			int streamIdx = 1;
			while(audioStreamNode) {
				const char* originId = prefDoc.getAttribute("id");
				if(originId) {
					audioIdAndIndexMap[atoi(originId)] = streamIdx;
					prefDoc.setAttribute("id", streamIdx);
					streamIdx++;
				}
				audioStreamNode = prefDoc.findNextNode("stream", "type", "audio");
			}

			prefDoc.goToKey("/root/output");
			std::map<int, int> videoIdAndIndexMap;
			streamIdx = 1;
			void* videoStreamNode = prefDoc.findChildNode("stream", "type", "video");
			while(videoStreamNode) {
				const char* originId = prefDoc.getAttribute("id");
				if(originId) {
					videoIdAndIndexMap[atoi(originId)] = streamIdx;
					prefDoc.setAttribute("id", streamIdx);
					streamIdx++;
				}
				videoStreamNode = prefDoc.findNextNode("stream", "type", "video");
			}

			prefDoc.goToKey("/root/output");
			void* muxerStreamNode = prefDoc.findChildNode("stream", "type", "muxer");
			while(muxerStreamNode) {
				const char* aId = prefDoc.getAttribute("aid");
				const char* vId = prefDoc.getAttribute("vid");
				if(aId && audioIdAndIndexMap.count(atoi(aId)) > 0) {
					prefDoc.setAttribute("aid", audioIdAndIndexMap[atoi(aId)]);
				}
				if(vId && videoIdAndIndexMap.count(atoi(vId)) > 0) {
					prefDoc.setAttribute("vid", videoIdAndIndexMap[atoi(vId)]);
				}
				muxerStreamNode = prefDoc.findNextNode("stream", "type", "muxer");
			}
		}
	}
	return true;
}


void CRootPrefs::initMediaInfo(StrPro::CXML2& prefDoc)
{
	prefDoc.goRoot();
	prefDoc.findChildNode("input");
	void* mediaNode = prefDoc.findChildNode("mediainfo");
	if(!strchr(m_pStrUrl, '|')) {	// Single file name
		m_pMainMediaInfo = createMediaInfo();
		std::string strSrcFile(m_pStrUrl);
		m_srcFiles.push_back(strSrcFile);
		if(mediaNode && prefDoc.findChildNode("general")) {
			m_pMainMediaInfo->replaceNode(mediaNode);
		} else {
			if(!GetMediaInfo(m_pStrUrl, m_pMainMediaInfo)) {
				delete m_pMainMediaInfo;
				m_pMainMediaInfo = NULL;
				logger_err(LOGM_UTIL_ROOTPREFS, "Get Media Info failed.\n");
			}
		}
		if(m_pMainMediaInfo) {
			m_mediaInfos[strSrcFile] = m_pMainMediaInfo;
		}
	} else {
		int totaltime = 0;
		char* tmpUrl = m_pStrUrl;
		char* curUrl = NULL;
		// Consider situation that only the first file contain full folder, others only have title.
		std::string fileFolder;
		int urlIndex = 0;
		while(curUrl = Strsep(&tmpUrl, "|")) {
			if(fileFolder.empty()) {
				fileFolder = curUrl;
				size_t lastSlashIdx = fileFolder.find_last_of(PATH_DELIMITER);
				if(lastSlashIdx != std::string::npos) {
					fileFolder = fileFolder.substr(0, lastSlashIdx+1);
				} else {
					fileFolder.clear();
				}
			}

			std::string curFilePath = curUrl;
			if(curFilePath.find(PATH_DELIMITER) == std::string::npos) {
				curFilePath = fileFolder+curUrl;
			}

			CXML2* pXml = createMediaInfo();
			if (mediaNode && prefDoc.findChildNode("general")) {
				pXml->replaceNode(mediaNode);
				mediaNode = prefDoc.findNextNode("mediainfo");
			} else {
				if(!GetMediaInfo(curFilePath.c_str(), pXml)) {
					delete pXml;
					pXml = NULL;
					logger_err(LOGM_UTIL_ROOTPREFS, "Get Media Info failed.\n");
				}
			} 

			if(m_mainUrlIndex == urlIndex) {		// Main source file
				m_pMainMediaInfo = pXml;
			}

			m_srcFiles.push_back(curFilePath);
			m_mediaInfos[curFilePath] = pXml;

			urlIndex++;

			if(pXml && pXml->goRoot() && pXml->findChildNode("general")) {
				totaltime += pXml->getChildNodeValueInt("duration");
			}
		}
		
		// reset duration for main mediainfo node
		if (m_pMainMediaInfo && m_pMainMediaInfo->goRoot() && m_pMainMediaInfo->findChildNode("general")) {
			if(m_pMainMediaInfo->findChildNode("duration")) {
				m_pMainMediaInfo->setNodeValue(totaltime);
			} else {
				m_pMainMediaInfo->addChild("duration", totaltime);
			}
		}
	}	
}

bool CRootPrefs::InitRoot(const char *strPrefs)
{
	if (strPrefs == NULL || *strPrefs == '\0') {
		logger_err(LOGM_UTIL_ROOTPREFS, "Invalid parameter: InitRoot().\n");
		return false;
	}
	CXML2 tempXml;
	if(tempXml.Read(strPrefs) != 0 || !tempXml.goRoot() || 
		!tempXml.findChildNode("input") || !tempXml.findNode("output")) {
		logger_err(LOGM_UTIL_ROOTPREFS, "Invalid preset.\n");
		return false;
	}

	// ===================Format enum(convert from string to index)===================
	// New version preset file (If find 'presets' node, use new else old)
	if(MEvaluater::GetMCcoreDoc()) {
		tempXml.findChildNode("presets");	
		void* childNode = tempXml.goChild();
		while(childNode) {			
			void* node = tempXml.findChildNode("node");
			while(node) {
				const char* key = tempXml.getAttribute("key");
				// If key type is enum, then convert enum string to index, else no change
				const char* nodeVal = tempXml.getNodeValue();
				const char* val = MEvaluater::GetCoreXmlFormatValue(key, nodeVal); 
				if (val) {
					tempXml.setNodeValue(val);
				}
				node = tempXml.findNextNode("node");
			}

			tempXml.SetCurrentNode(childNode);
			childNode = tempXml.goNext();
		}
	}

	// ===============Initialize other basic info===============
	CCharset set;
	tempXml.goRoot();
	tempXml.findChildNode("input");
	const char* val = tempXml.getChildNodeValue("url");
	m_pStrUrl = set.UTF8toANSI(val);
	val = tempXml.getChildNodeValue("tempdir");
	m_pStrTempDir = set.UTF8toANSI(val);
	val = tempXml.getChildNodeValue("subtitle");
	m_pStrSubTitle = set.UTF8toANSI(val);
	m_mainUrlIndex = tempXml.getChildNodeValueInt("index");
	// Init mediainfo node
	initMediaInfo(tempXml);
	initFileTitle();	// Get file title or DVD/VCD track name

	tempXml.goRoot();

	//==================== Apply bitrate and resolution restrict here(PPTV MBR) ========================
	//if(!processMBR(m_pSrcMediaInfo, tempXml)) return false;
	//========================================================================================

	// Init output stream prefs.
	m_outputs = new CStreamPrefs;
	tempXml.goRoot();
	tempXml.findChildNode("output");
	return m_outputs->InitStreams(&tempXml);
}

CXML2* CRootPrefs::createMediaInfo()
{
	CXML2* pInfo = new CXML2;
	pInfo->New("1.0", "mediainfo");
	return pInfo;
}

bool CRootPrefs::setStreamPref2(const char* key, int keyType, void *pVal, const char* type, int index /* = 0 */)
{
	//first find the node parent node
	if(!m_prefsDoc->goRoot() || !m_prefsDoc->findChildNode("output")) {
		logger_err(LOGM_UTIL_ROOTPREFS, "Invalid preset doc in setStreamPref2.\n");
		return false;
	}

	int begIndex = 0;
	void* cnode = NULL;
	if(m_prefsDoc->findChildNode("presets")) {	// New version preset file
		m_prefsDoc->goParent();
		if(!strcmp(type, STAUDIO) || !strcmp(type, STVIDEO) || !strcmp(type, STMUXER)) {
			// Find preset id first from stream node
			const char* presetId = NULL;
			void* streamNode = m_prefsDoc->findChildNode("stream", "type", type);
			while(streamNode) {
				presetId = m_prefsDoc->getAttribute("pid");
				if(presetId && (atoi(presetId)-1 == index)) {	// preset id base index is 1 (in preset)
					break;
				}
				streamNode = m_prefsDoc->findNextNode("stream", "type", type);
			}

			if(presetId) {
				m_prefsDoc->goParent();
				if(m_prefsDoc->findChildNode("presets")) {
					cnode = m_prefsDoc->findChildNode("MediaCoderPrefs", "id", presetId);
				}
			}
		} else {	// Config or Split node
			cnode = m_prefsDoc->findChildNode(type);
			while(cnode && begIndex++ < index) {
				cnode = m_prefsDoc->findNextNode(type);
			}
		}
	} else {
		if( strcmp(type, STAUDIO) == 0 || strcmp(type, STVIDEO) == 0) {
			cnode = m_prefsDoc->findChildNode("stream", "type", type);
			while(cnode && begIndex++ < index) {
				cnode = m_prefsDoc->findNextNode("stream", "type", type);
			}
		} else {		
			cnode = m_prefsDoc->findChildNode(type);
			while(cnode && begIndex++ < index) {
				cnode = m_prefsDoc->findNextNode(type);
			}
		}
	}
		

	if (cnode) {
		if(!m_prefsDoc->findChildNode("node", "key", key)) {
			void* newnode = m_prefsDoc->addChild("node");
			m_prefsDoc->SetCurrentNode(newnode);
			m_prefsDoc->setAttribute("key", key);
		}
		switch (keyType) {
		case PREF_TYPE_STRING: //string
			m_prefsDoc->setNodeValue((const char*)pVal);
			break;
		case PREF_TYPE_INT: //int
			m_prefsDoc->setNodeValue(*(int*)pVal);
			break;
		case PREF_TYPE_BOOL: //bool
			m_prefsDoc->setNodeValue(*(bool*)pVal ? "true" : "false");
			break;
		case PREF_TYPE_FLOAT: //float
			m_prefsDoc->setNodeValue(*(float*)pVal);
			break;
		default:
			break;
		}
		return true;
	}

	return false;
}

bool CRootPrefs::SetStreamPref(const char* key, int keyType, void *pVal, const char* type, int index /* = 0 */)
{
	bool ret;
	CXMLPref* pref = NULL;
	
	if (key == NULL || pVal == NULL) {
		logger_err(LOGM_UTIL_ROOTPREFS, "Invalid parameter in SetStreanPref.\n");
		return false;
	}
	//we set the function here to do changes 
	//while the root is created in master
	if (m_prefsDoc) {
		return setStreamPref2(key, keyType, pVal, type, index);
	}

	if (!m_outputs) {
		logger_err(LOGM_UTIL_ROOTPREFS, "Output stream preset object hasn't been created.\n");
		return false;
	}

	if (strcmp(type, STVIDEO) == 0) {
		pref = m_outputs->GetVideoPrefs(index);
	} else if (strcmp(type, STAUDIO) == 0) {
		pref = m_outputs->GetAudioPrefs(index);
	} else if (strcmp(type, STMUXER) == 0) {
		pref = m_outputs->GetMuxerPrefs(index);
	} 

	if ( pref == NULL) {
		logger_err(LOGM_UTIL_ROOTPREFS, "Can not find specified stream pref.\n");
		return false;
	}

	switch (keyType) {
		case PREF_TYPE_STRING: //string
			ret = pref->SetString(key, (const char*)pVal, false);
			break;
		case PREF_TYPE_INT: //int
			ret = pref->SetInt(key, *(int*)pVal);
			break;
		case PREF_TYPE_BOOL: //bool
			ret = pref->SetBoolean(key, *(bool*)pVal);
			break;
		case PREF_TYPE_FLOAT: //float
			ret = pref->SetFloat(key, *(float*)pVal);
			break;
		default:
			ret = false;
			break;
	}

	if (!ret) {
		logger_err(LOGM_UTIL_ROOTPREFS, "SetStreamPref failed\n");
		return false;
	}

	return true;
}

bool CRootPrefs::SetStreamPref(const char*key, const char *val, const char* type, int index /* = 0 */)
{
	return SetStreamPref(key, PREF_TYPE_STRING, (void*)val, type, index);
}

bool CRootPrefs::SetStreamPref(const char* key, int val, const char* type, int index /* = 0 */)
{
	return SetStreamPref(key, PREF_TYPE_INT, (void*)&val, type, index);
}

bool CRootPrefs::SetStreamPref(const char*key, bool val, const char* type, int index /* = 0 */)
{
	return SetStreamPref(key, PREF_TYPE_BOOL, (void*)&val, type, index);
}

bool CRootPrefs::SetStreamPref(const char*key, float val, const char* type, int index /* = 0 */)
{
	return SetStreamPref(key, PREF_TYPE_FLOAT, (void*)&val, type, index);
}

std::string CRootPrefs::DumpXml()
{
	std::string xmlStr;
	char* res = NULL;
	if (m_prefsDoc && m_prefsDoc->goRoot() && m_prefsDoc->Dump(&res) > 0) {
		xmlStr = res;
		m_prefsDoc->FreeDump(&res);
		return xmlStr;
	}
	return "";
}

const char* CRootPrefs::GetMainUrl()
{
	std::string strMainUrl;
	if(m_srcFiles.size() > m_mainUrlIndex) {
		strMainUrl = m_srcFiles[m_mainUrlIndex];
	}
	if(strMainUrl.empty()) {
		return NULL;
	}
	return strMainUrl.c_str();
}

void CRootPrefs::initFileTitle()
{
	std::string strMainUrl;
	if(m_srcFiles.size() > m_mainUrlIndex) {
		strMainUrl = m_srcFiles[m_mainUrlIndex];
	}
	if(strMainUrl.empty()) return;

	if (strMainUrl.find("dvd://") != std::string::npos) {		// DVD playback 
		std::vector<std::string> dvdParams;
		StrPro::StrHelper::splitString(dvdParams, strMainUrl.c_str());
		std::string dvdTitle;
		std::string ext;
		// If Url contains extension, parse file title
		if(StrPro::StrHelper::getFileExt(strMainUrl.c_str(), ext)) {
			StrPro::StrHelper::getFileTitle(strMainUrl.c_str(), dvdTitle);
		} else {
			dvdTitle = "DVD";
		}

		if(!dvdParams[1].empty() && dvdParams[1] != "0") {			// Track id or title
			dvdTitle += "_Track_";
			dvdTitle += dvdParams[1];								
		}
		if(!dvdParams[2].empty() && dvdParams[2] != "-1") {
			dvdTitle += "_Chapter_";
			dvdTitle += dvdParams[2];
		}
		m_pFileTitle = _strdup(dvdTitle.c_str());
	} else if (strMainUrl.find("vcd://") != std::string::npos) {	// VCD playback
		std::vector<std::string> vcdParams;
		StrPro::StrHelper::splitString(vcdParams, strMainUrl.c_str());
		std::string vcdTitle = "VCD";
		if(!vcdParams[1].empty() && vcdParams[1] != "0") {			// Track id or title
			vcdTitle += "_Track_";
			vcdTitle += vcdParams[1];								
		}
		m_pFileTitle = _strdup(vcdTitle.c_str());
	} else {
		// Get file title from main url
		const char* lastSlash = strrchr(strMainUrl.c_str(), '/');
		const char* lastBackSlash = strrchr(strMainUrl.c_str(), '\\');
		if(lastSlash < lastBackSlash) {
			lastSlash = lastBackSlash;
		}

		if(lastSlash) {
			lastSlash += 1;
		} else {
			lastSlash = strMainUrl.c_str();
		}
		const char* lastDot = strrchr(strMainUrl.c_str(), '.');
		size_t copyLen = strlen(lastSlash);
		if(lastDot && lastDot > lastSlash) copyLen = lastDot-lastSlash;

		char strTitle[256] = {0};
		strncpy(strTitle, lastSlash, copyLen);
		m_pFileTitle = _strdup(strTitle);
	}
}

void CRootPrefs::clearMediaInfos()
{
	std::map<std::string, StrPro::CXML2*>::iterator it;
	for(it=m_mediaInfos.begin(); it != m_mediaInfos.end(); ++it) {
		StrPro::CXML2* pXml = it->second;
		if(pXml) delete pXml;
	}
	m_mediaInfos.clear();
}

CXML2* CRootPrefs::GetSrcMediaInfoDoc()
{
	// For transnode, it's initialized in InitRoot
	// For tsmaster, when mediainfo is in input part of preset, it's already inited.
	if(m_pMainMediaInfo) return  m_pMainMediaInfo;
	
	// For tsmaster, it's not  initialized in InitRootForMaster, init here
	// Because mediainfo parsing in tsmaster is delay to user select one task in GUI
	// For tsmaster, only parse mediainfo of first file(m_pMainUrl)
	const char* strMainUrl = GetMainUrl();
	if(!strMainUrl) return NULL;

	m_pMainMediaInfo = createMediaInfo();
	if(!GetMediaInfo(strMainUrl, m_pMainMediaInfo)) {
		delete m_pMainMediaInfo;
		m_pMainMediaInfo = NULL;
	}
	return m_pMainMediaInfo;
}

bool CRootPrefs::InitRootForMaster(const char *strPrefs)
{
	if (!strPrefs || !*strPrefs) {
		return false;
	}

	m_prefsDoc = new CXML2;
	if(!m_prefsDoc || m_prefsDoc->Read(strPrefs) != 0 || !m_prefsDoc->goRoot() || 
		!m_prefsDoc->findChildNode("input")) {
		logger_err(LOGM_UTIL_ROOTPREFS, "Invalid preset.\n");
		return false;
	}

	// Find priority node
	m_taskPriority = m_prefsDoc->getChildNodeValueInt("priority");
	if(m_taskPriority < 0) m_taskPriority = 2;	// Normal

	CCharset set;
	const char* val = m_prefsDoc->getChildNodeValue("url");
	m_pStrUrl = set.UTF8toANSI(val);
	char* delimChr = strchr(m_pStrUrl, '|');
	if(delimChr) {
		*delimChr = 0;
	}
	m_srcFiles.push_back(std::string(m_pStrUrl));

	initFileTitle();	// Get file title or DVD/VCD track name

	// Create mediainfo doc if there is mediainfo in input
	m_bDeleteMainMediaInfo = true;
	void* mediainfoNode = m_prefsDoc->findChildNode("mediainfo");
	if(mediainfoNode) {
		m_pMainMediaInfo = createMediaInfo();
		m_pMainMediaInfo->replaceNode(mediainfoNode);
	}

	//================= Parse output part ======================
	//find dest dir
	m_prefsDoc->goRoot();
	m_prefsDoc->findChildNode("output");
	const char* outDirNodeVal = NULL;
	//find a stream and get its destdir
	if(m_prefsDoc->findChildNode("presets")) {	// New version preset file
		if(m_prefsDoc->findChildNode("MediaCoderPrefs")) {
			outDirNodeVal = m_prefsDoc->getChildNodeValue("node", "key", "overall.task.destdir");
		}
	} else if(m_prefsDoc->findChildNode("stream")) {	// Old version preset file
		outDirNodeVal = m_prefsDoc->getChildNodeValue("node", "key", "overall.task.destdir");
	}
	m_pStrDestDir = set.UTF8toANSI(outDirNodeVal);
	//m_prefsDoc->Save("e:\\simpleTask.xml");
	return true;
}

CStreamPrefs* CRootPrefs::GetStreamPrefs()
{
	// For transnode, it's is initialized in InitRoot
	if(m_outputs) return  m_outputs;
	
	// For tsmaster, it's not  initialized in InitRootForMaster, init here
	// Because mediainfo parsing in tsmaster is delay to user select one task in GUI
	if(m_prefsDoc) {
		m_prefsDoc->goRoot();
		m_prefsDoc->findChildNode("output");
		m_outputs = new CStreamPrefs;
		m_outputs->InitStreams(m_prefsDoc);
	}

	return m_outputs;
}
