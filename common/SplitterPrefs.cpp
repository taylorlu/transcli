#include "bit_osdep.h"
#include "StrProInc.h"
#include "SplitterPrefs.h"
#include "util.h"

using namespace std;
using namespace StrPro;

CSplitterPrefs::CSplitterPrefs():m_type(SPLIT_TYPE_UNKNOWN), 
								m_subfixType(SPLIT_SUBFIX_TYPE_INDEX), 
								m_unitType(SPLIT_UNIT_BYSIZE),
								m_interval(0), m_bGenConfig(false)
{
}

bool CSplitterPrefs::Parsing(const string &strSpliterPrefs)
{
	CXML2 doc;
	bool ret = true;

	// Save split info string (remove xml header)
	m_strSplitInfoXml = strSpliterPrefs;
	size_t xmlHeaderEndIdx = m_strSplitInfoXml.find("?>");
	if(xmlHeaderEndIdx != m_strSplitInfoXml.npos) {
		m_strSplitInfoXml = m_strSplitInfoXml.substr(xmlHeaderEndIdx+2);
	}

	do {
		if (doc.Read(strSpliterPrefs.c_str()) != 0) break;
		if (!doc.goRoot()) break;

		const char* type = doc.getAttribute("type");

		if (type == NULL) break;
		if (!strcmp(type, "normal")) {
			m_type = SPLIT_TYPE_NORMAL;
		}
		else if (!strcmp(type, "average")) {
			m_type = SPLIT_TYPE_AVERAGE;
		}
		else if (!strcmp(type, "none")) {
			m_type = SPLIT_TYPE_NONE;
			ret = true;
			break;
		}	
		else {
			m_type = SPLIT_TYPE_UNKNOWN;
			break;
		}

		const char* subfix = doc.getAttribute("subfix");
		const char* unitType = doc.getAttribute("unitType");
		const char* prefix = doc.getAttribute("prefix");
		const char* genConfig = doc.getAttribute("isConfig");

		if (subfix && !strcmp(subfix, "time")) {
			m_subfixType = SPLIT_SUBFIX_TYPE_TIME;
		}
		else {
			m_subfixType = SPLIT_SUBFIX_TYPE_INDEX; //Default by index
		}

		if (unitType && !strcmp(unitType, "time")) {
			m_unitType = SPLIT_UNIT_BYTIME;
		}
		else {
			m_unitType = SPLIT_UNIT_BYSIZE; //Default by size
		}

		if(genConfig && !strcmp(genConfig, "true")) {
			m_bGenConfig = true;
		}

		if (prefix) m_prefix = prefix;

		if (!doc.findChildNode("clips")) break;

		const char *clips = doc.getNodeValue();
		if (clips == NULL) break;

		if (m_type == SPLIT_TYPE_AVERAGE) {
			m_interval = atoi(clips);
			if(m_unitType == SPLIT_UNIT_BYTIME) {			// Convert time unit to ms
				m_interval *= 1000;
			} else if(m_unitType == SPLIT_UNIT_BYSIZE) {	// Convert time unit to bytes
				m_interval *= 1024;
			}
		}
		else {
			ret = ParsingClipMarks(clips);
		}
	} while (false);

	return ret;
}

// <clips>-1000|1000-4000|5000-</clips>
bool CSplitterPrefs::ParsingClipMarks(const char *strClipMarks)
{
	if (strClipMarks == NULL || *strClipMarks == '\0') return false;

	const char *delim = "|";
	int64_t last_end = 0;
	char *markStr = _strdup(strClipMarks);
	char *tmpStr = markStr;
	m_clipMarks.clear();
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
			if(m_unitType == SPLIT_UNIT_BYTIME) {			// Convert time unit from s to ms
				start *= 1000;
				end *= 1000;
			} else if(m_unitType == SPLIT_UNIT_BYSIZE) {	// Convert size unit from kb to bytes
				start *= 1024;
				end *= 1024;
			}
		}

		if (start < last_end) start = last_end;
		if (end != 0 && start > end) break;

		m_clipMarks.push_back(std::pair<int64_t, int64_t>(start, end));

		if(end == 0) break;

		last_end = end;
	}
	
	free(markStr);
	return true;
}

std::pair<int64_t, int64_t> CSplitterPrefs::GetClip(int clipIdx)
{
	if (clipIdx >= 0 && clipIdx < (int)m_clipMarks.size()) {
		return m_clipMarks[clipIdx];
	}
	return std::pair<int64_t, int64_t>(-1, -1);
}
