#include <stdio.h>
#include <fcntl.h>	
#include <sys/stat.h>
#include <list>

#include "MEvaluater.h"
#include "logger.h"

#include "bit_osdep.h"

using namespace StrPro;
using namespace std;

//==================================================================================
CStreamPrefs::CStreamPrefs()
{
}

CStreamPrefs::~CStreamPrefs()
{
	for(size_t i=0; i<m_muxerStream.size(); ++i) {
		CXMLPref *prefs = m_muxerStream.at(i);
		delete prefs;
	}
	m_muxerStream.clear();

	for(size_t i=0; i<m_audioStreams.size(); ++i) {
		CXMLPref *prefs = m_audioStreams.at(i);
		delete prefs;
	}
	m_audioStreams.clear();

	for(size_t i=0; i<m_videoStreams.size(); ++i) {
		CXMLPref *prefs = m_videoStreams.at(i);
		delete prefs;
	}
	m_videoStreams.clear();
}

int CStreamPrefs::GetAudioCount() 
{
	return m_audioStreams.size();
}

int CStreamPrefs::GetVideoCount()
{
	return m_videoStreams.size();
}

int CStreamPrefs::GetMuxerCount()
{
	return m_muxerStream.size();
}

CXMLPref* CStreamPrefs::GetAudioPrefs(size_t id)
{
	if (id < m_audioStreams.size() && id >= 0) {
		return m_audioStreams.at(id);
	} 
	return NULL;
}

CXMLPref*  CStreamPrefs::GetVideoPrefs(size_t id)
{
	if (id < m_videoStreams.size() && id >= 0) {
		return m_videoStreams.at(id);
	}
	return NULL;
}

CXMLPref* CStreamPrefs::GetMuxerPrefs(size_t id)
{
	if (id < m_muxerStream.size() && id >= 0) {
		return m_muxerStream.at(id);
	}
	return NULL;
}

bool CStreamPrefs::InitStreams(StrPro::CXML2 *streamsPrefs)
{
	if (!streamsPrefs || strcmp(streamsPrefs->getNodeName(), "output") != 0){
		return false;
	}

	void* presetsNode = streamsPrefs->findChildNode("presets");
	if(presetsNode) {	// New version preset file
		// Cache preset node for follow-up use
		std::map<int, void*> presetsMap;
		void* presetNode = streamsPrefs->findChildNode("MediaCoderPrefs");
		while(presetNode) {
			const char* presetId = streamsPrefs->getAttribute("id");
			if(presetId) {
				presetsMap[atoi(presetId)] = presetNode;
			}
			presetNode = streamsPrefs->findNextNode("MediaCoderPrefs");
		}
		streamsPrefs->goParent();

		streamsPrefs->goParent();
		// Create audio/video/muxer stream prefs
		void* stream = streamsPrefs->findChildNode("stream");
		while (stream) {			
			const char* type = streamsPrefs->getAttribute("type");
			const char* presetId = streamsPrefs->getAttribute("pid");
			if(type && presetId) {
				CXMLPref* pNewPref = new CXMLPref("stream");
				if (pNewPref) {
					void* newPresetNode = presetsMap[atoi(presetId)];
					if(!newPresetNode) {
						logger_err(LOGM_UTIL_STREAMPREFS, "No node relate to preset id:%s.\n", presetId);
						return false;
					}
					pNewPref->replaceNode(newPresetNode);
				} else if(pNewPref) {
					logger_err(LOGM_UTIL_STREAMPREFS, "Create pref of stream [%s] failed.\n", type);
					return false;
				}

				pNewPref->goRoot();
				if (strcmp(type, STAUDIO) == 0) {
					m_audioStreams.push_back(pNewPref);			
				} else if (strcmp(type, STVIDEO) == 0) {
					m_videoStreams.push_back(pNewPref);
				} else if (strcmp(type, STMUXER) == 0) {
					const char* aIdStr = streamsPrefs->getAttribute("aid");
					const char* vIdStr = streamsPrefs->getAttribute("vid");
					if(aIdStr) pNewPref->setAttribute("aid", aIdStr);
					if(vIdStr) pNewPref->setAttribute("vid", vIdStr);
					m_muxerStream.push_back(pNewPref);
				}
			}
			stream = streamsPrefs->findNextNode("stream");
		}
	} else {	// Old version preset file
		// Create audio/video stream prefs
		void* stream = streamsPrefs->findChildNode("stream");
		while (stream) {			
			const char* type = streamsPrefs->getAttribute("type");
			if (type) {
				if (strcmp(type, STAUDIO) == 0) {
					CXMLPref* pNewPref = new CXMLPref("stream");
					if (pNewPref) {
						pNewPref->replaceNode(stream);
						m_audioStreams.push_back(pNewPref);
					}					
				} else if (strcmp(type, STVIDEO) == 0) {
					CXMLPref* pNewPref = new CXMLPref("stream");
					if (pNewPref) {
						pNewPref->replaceNode(stream);
						m_videoStreams.push_back(pNewPref);
					}				
				}
			}
			stream = streamsPrefs->findNextNode("stream");
		}

		// Create muxer prefs
		streamsPrefs->goParent();
		void* muxer = streamsPrefs->findChildNode("muxer");
		while(muxer) {
			CXMLPref* pNewPref = new CXMLPref("muxer");
			if(pNewPref) {
				pNewPref->replaceNode(muxer);
				pNewPref->setNodeName("muxer");
				m_muxerStream.push_back(pNewPref);
			}	
			muxer = streamsPrefs->findNextNode("muxer");
		}
	}
	
	return true;
}


