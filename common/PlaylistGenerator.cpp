#include <sstream>
#include <time.h>
#include "bit_osdep.h"
#include "bitconfig.h"
#include "PlaylistGenerator.h"
#include "StrProInc.h"
#include "util.h"
#include "logger.h"

using namespace StrPro;


CPlaylistGenerator::CPlaylistGenerator(void) : m_bEndList(false), m_bStartList(false),
											   m_bLive(false), m_size(0),
											   m_segmentDuration(DEFAULT_SEGMENT_DURATION),
											   m_beginIndex(0)
{
    m_startTime = GetTickCount();
}

CPlaylistGenerator::~CPlaylistGenerator(void)
{
	m_segmentQueue.clear();
}

void CPlaylistGenerator::GenPlaylist()
{
	if(m_segmentQueue.empty()) return;
	if(m_filePath.empty() || m_listName.empty()) return;

	const char* listType = m_listType.c_str();
	std::string listFile = m_filePath;
	if(listFile.back() != PATH_DELIMITER) listFile.append(1, PATH_DELIMITER);
	listFile += m_listName + m_listType;
	FILE* fp = fopen(listFile.c_str(), "w");
	if(!fp) return;

	if(_stricmp(listType, PLAYLIST_M3U) == 0) {
		GenM3uList(fp);
	} else if(_stricmp(listType, PLAYLIST_PLS) == 0) {
		GenPlsList(fp);
	} else if(_stricmp(listType, PLAYLIST_WPL) == 0) {
		GenWplList(fp);
	} else if(_stricmp(listType, PLAYLIST_ASX) == 0) {
		GenAsxList(fp);
	} else if (_stricmp(listType, PLAYLIST_M3U8) == 0) {
		GenM3u8List(fp);
	} else if (_stricmp(listType,PLAYLIST_CSV) == 0) {
		GenCsvList(fp);
	}


	fclose(fp);
	if (!m_bLive) {
		m_bEndList = false;
		m_bStartList = false;
		m_segmentQueue.clear();
	}

}

void CPlaylistGenerator::GenM3uList(FILE* fp)
{
	std::ostringstream m3uContent;
	int duration = DEFAULT_SEGMENT_DURATION;
	if(m_segmentTimes.size() > 0) {
		duration = (int)(m_segmentTimes.at(0)/1000);
	}
	m3uContent << "#EXTM3U\n#EXT-X-MEDIA-SEQUENCE:1\n"
		<< "#EXT-X-TARGETDURATION:"
		<< duration << "\n";
	for (size_t i=0; i<m_segmentQueue.size(); ++i) {
		std::string segFile = m_segmentQueue[i];
		// Get relative path of segment file
		size_t slashIdx = segFile.find_last_of('\\');
		if(slashIdx != segFile.npos) segFile = segFile.substr(slashIdx+1);
		m3uContent << "#EXTINF:"
			<< duration << ",\n"
			<< segFile + "\n";
	}

	m3uContent << "#EXT-X-ENDLIST";
	fwrite(m3uContent.str().c_str(), m3uContent.str().size(), 1, fp);
	
}

//http://developer.apple.com/iphone/library/documentation/NetworkingInternet/Conceptual/HTTPLiveStreaming/

void CPlaylistGenerator::GenM3u8List(FILE* fp)
{
	
	int duration = m_segmentDuration > 0? m_segmentDuration:DEFAULT_SEGMENT_DURATION;
	std::ostringstream content;

	content << "#EXTM3U\n";
	if (m_size > 0 && m_segmentQueue.size() > m_size) {	
		// Live streaming, increase begin index
		++m_beginIndex;
		// Delete expired file when it's live streaming
		std::string expiredFile = m_segmentQueue.front();
		if(FileExist(expiredFile.c_str())) {
			if(::remove(expiredFile.c_str()) != 0) {
				logger_warn(LOGM_UTIL_PLAYLIST_GEN, "Failed to delete expired ts file %s \n", expiredFile.c_str());
			}
		}
		m_segmentQueue.pop_front();
	}

	content << "#EXT-X-MEDIA-SEQUENCE:"
			<< m_beginIndex + 1 << "\n"
			<< "#EXT-X-TARGETDURATION:"
			<< duration << "\n";

	for (size_t i = 0; i < m_segmentQueue.size(); ++i) {
		std::string segFile = m_segmentQueue[i];
		// Get relative path of segment file
		size_t slashIdx = segFile.find_last_of(PATH_DELIMITER);
		if(slashIdx != segFile.npos) segFile = segFile.substr(slashIdx+1);

		content << "#EXTINF:"
				<< duration << ",\n"
				<< segFile + "\n";
	}

	if (!m_bLive) {		// live streaming should not add end list property
		content << "#EXT-X-ENDLIST";
	}

	// content of m3u8 must be utf-8 format
	CCharset charConvert;
	char* utf8Content = charConvert.ANSItoUTF8(content.str().c_str());
	if(utf8Content) {
		fwrite(utf8Content, strlen(utf8Content), 1, fp);
		free(utf8Content);
	}
}

void CPlaylistGenerator::GenPlsList(FILE* fp)
{
	std::string strPls = "[playlist]\n";
	for (size_t i=0; i<m_segmentQueue.size(); ++i) {
		std::string segFile = m_segmentQueue[i];
		// Get relative path of segment file
		size_t slashIdx = segFile.find_last_of('\\');
		if(slashIdx != segFile.npos) segFile = segFile.substr(slashIdx+1);

		std::string segTitle = segFile;
		size_t dotIdx = segFile.find_last_of('.');
		if(dotIdx) segTitle = segTitle.substr(0, dotIdx);

		char plsContent[1024] = {0};
		int fileTime = -1;
		sprintf(plsContent, "File%ld=%s\nTitle%ld=%s\nLength%ld=%d\n", i+1,segFile.c_str(), i+1,segTitle.c_str(), i+1,fileTime);

		strPls += plsContent;
	}
	char totalFiles[64] = {0};
	sprintf(totalFiles, "NumberOfEntries=%lu\n", m_segmentQueue.size());
	strPls += totalFiles;
	strPls += "Version=2";
	CCharset strConvert;
	char* utf8Pls = strConvert.ANSItoUTF8(strPls.c_str());
	if(utf8Pls) {
		fwrite(utf8Pls, strlen(utf8Pls), 1, fp);
		free(utf8Pls);
	}
}

void CPlaylistGenerator::GenAsxList(FILE* fp, const char* author, const char* title, const char* copyRight,
								const char* strAbstract, const char* moreInfo)
{
	// Header info
	std::string asxContent = "<ASX VERSION=\"3.0\">\n";
	if(author) {
		asxContent += "<Author>";
		asxContent += author;
		asxContent += "</Author>\n";
	}
	if(title) {
		asxContent += "<Title>";
		asxContent += title;
		asxContent += "</Title>\n";
	}
	if(copyRight) {
		asxContent += "<Copyright>";
		asxContent += copyRight;
		asxContent += "</Copyright>\n";
	}
	if(strAbstract) {
		asxContent += "<Abstract>";
		asxContent += strAbstract;
		asxContent += "</Abstract>\n";
	}
	if(moreInfo) {
		asxContent += "<Moreinfo href=\"";
		asxContent += moreInfo;
		asxContent += "\" />\n";
	}

	// Main content
	for (size_t i=0; i<m_segmentQueue.size(); ++i) {
		std::string segFile = m_segmentQueue[i];
		// Get relative path of segment file
		size_t slashIdx = segFile.find_last_of('\\');
		if(slashIdx != segFile.npos) segFile = segFile.substr(slashIdx+1);

		std::string segTitle = segFile;
		size_t dotIdx = segFile.find_last_of('.');
		if(dotIdx) segTitle = segTitle.substr(0, dotIdx);

		asxContent += "<Entry>\n";
		if(author) {
			asxContent += "<Author>";
			asxContent += author;
			asxContent += "</Author>\n";
		}
		if(!segTitle.empty()) {
			asxContent += "<Title>";
			asxContent += segTitle;
			asxContent += "</Title>\n";
		}
		if(moreInfo) {
			asxContent += "<Moreinfo href=\"";
			asxContent += moreInfo;
			asxContent += "\" />\n";
		}
		asxContent += "<ref href=\"";
		asxContent += segFile;
		asxContent += "\" />\n</Entry>\n";
	}
	asxContent += "</ASX>";
	fwrite(asxContent.c_str(), asxContent.length(), 1, fp);
}

void CPlaylistGenerator::GenWplList(FILE* fp, const char* title)
{
	std::string wplContent = "<?wpl version=\"1.0\"?>\n<smil>\n";
	// Header
	wplContent += "<head>\n <meta name=\"Generator\" content=\"BroadIntel MediacoderNT-1.0\" />\n";
	if(title) {
		wplContent += "<title>";
		wplContent += title;
		wplContent += "</title>\n</head>\n";
	}

	// Body
	wplContent += "<body>\n<seq>\n";
	for (size_t i=0; i<m_segmentQueue.size(); ++i) {
		std::string segFile = m_segmentQueue[i];
		// Get relative path of segment file
		size_t slashIdx = segFile.find_last_of('\\');
		if(slashIdx != segFile.npos) segFile = segFile.substr(slashIdx+1);
		wplContent += "<media src=\"";
		wplContent += segFile;
		wplContent += "\" />\n";
	}
	wplContent += "</seq>\n</body>\n</smil>";

	CCharset strConvert;
	char* utf8Wpl = strConvert.ANSItoUTF8(wplContent.c_str());
	if(utf8Wpl) {
		fwrite(utf8Wpl, strlen(utf8Wpl), 1, fp);
		free(utf8Wpl);
	}
}

void CPlaylistGenerator::GenCsvList(FILE* fp)
{
	std::string strCsv;
	for (size_t i=0; i<m_segmentQueue.size(); ++i) {
		std::string segFile = m_segmentQueue[i];
		// Get relative path of segment file
		size_t slashIdx = segFile.find_last_of('\\');
		if(slashIdx != segFile.npos) segFile = segFile.substr(slashIdx+1);
		// File number
		time_t t = time(0);
		struct tm *btm = localtime(&t);
		char fileNo[128] = {0};
		sprintf(fileNo, "%d%2d%2d%03d,", btm->tm_year+1900, btm->tm_mon+1, btm->tm_mday, i+1);
		strCsv += fileNo;

		// File duration
		if(m_segmentTimes.size() == m_segmentQueue.size()) {
			strCsv += SecondsToTimeString((int)(m_segmentTimes[i]/1000));
		}
		strCsv += ",";
		// File name
		strCsv += segFile + ",";
		// File Size
		int64_t filesize = StatFileSize(m_segmentQueue[i].c_str());
		char sizestr[24] = {0};
		sprintf(sizestr, "%ld\n", filesize/1024);
		strCsv += sizestr;
	}
	fwrite(strCsv.c_str(), strCsv.size(), 1, fp);
}

void CPlaylistGenerator::GenSimpleList(FILE* fp)
{
	// Simple list is used for ui to upload and validate target files

	std::string simpleXml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?> <root>";
		
	for (size_t i=0; i<m_segmentQueue.size(); ++i) {
		simpleXml += "<file duration=\"";
		std::string strDur;
		// File duration
		if(m_segmentTimes.size() == m_segmentQueue.size()) {
			simpleXml += SecondsToTimeString((int)(m_segmentTimes[i]/1000));
		}
		// File Size
		int64_t filesize = StatFileSize(m_segmentQueue[i].c_str());
		char sizestr[24] = {0};
		sprintf(sizestr, "%ld", filesize/1024);
			
		simpleXml += "\" size=\"";
		simpleXml += sizestr;
		simpleXml += "\">";
		simpleXml += m_segmentQueue[i];
		simpleXml += "</file>";
	}

	simpleXml += "</root>";

		
	CCharset strConvert;
	char* utf8Xml = strConvert.ANSItoUTF8(simpleXml.c_str());
	if(utf8Xml) {
		fwrite(utf8Xml, strlen(utf8Xml), 1, fp);
		free(utf8Xml);
	}
}

void CPlaylistGenerator::AddPlaylistItem(const std::string& fileName)
{
	//if(!m_bStartList) return;
	
	m_segmentQueue.push_back(fileName);

	if (m_bLive) GenPlaylist();
}

