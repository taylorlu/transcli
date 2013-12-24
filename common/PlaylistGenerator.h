#ifndef _PLAYLIST_GENERATOR_H_
#define _PLAYLIST_GENERATOR_H_

#include <string>
#include <deque>
#include <vector>
#include <stdint.h>

#define PLAYLIST_M3U ".m3u"
#define PLAYLIST_PLS ".pls"
#define PLAYLIST_WPL ".wpl"
#define PLAYLIST_ASX ".asx"
#define PLAYLIST_M3U8 ".m3u8"
#define PLAYLIST_SIMPLE ".list"
#define PLAYLIST_CSV ".csv"

#define DEFAULT_SEGMENT_DURATION 10
#define MIN_M3U8_WINDOW_SIZE	10

class CPlaylistGenerator
{
public:
	CPlaylistGenerator(void);
	void SetFilePath(const char* filePath) {if(filePath) m_filePath = filePath;}
	void SetPlaylistName(const char* listName) {if(listName) m_listName = listName;}
	void SetPlaylistType(const char* listType=PLAYLIST_M3U) {if(listType) m_listType=listType;}
	void AddPlaylistItem(const std::string& fileName);
	void FlushPlaylist() {m_segmentQueue.clear(); m_segmentTimes.clear();}
	void SetStartPlaylist(bool bStart) {m_bStartList = bStart;}
	void SetEndPlaylist(bool bEnd) {m_bEndList = bEnd;}
	bool GetPlaylistEnd() {return m_bEndList;}
	void SetbLive(bool bLive) {m_bLive = bLive;}
	void SetSize(int size) { m_size = size; }	// item count of playlist
	void SetSegmentDuration(int second) {if(second>0) m_segmentDuration = second;}
	void GenPlaylist();

	void AddSegmentDuration(int64_t duration) {m_segmentTimes.push_back(duration);}

	~CPlaylistGenerator(void);

private:
	void GenSimpleList(FILE* fp);
	void GenM3uList(FILE* fp);
	void GenM3u8List(FILE* fp);
	void GenPlsList(FILE* fp);
	void GenAsxList(FILE* fp, const char* author="BroadIntel", const char* title="CD Lips",
		const char* copyRight="Broad Intelligence Inc.", const char* strAbstract="Cafe-Music CD by broad intel",
		const char* moreInfo="http://www.broadintel.com");
	void GenWplList(FILE* fp, const char* title="My favorite songs");
	void GenCsvList(FILE* fp);

	bool		m_bEndList;
	bool		m_bStartList;
	std::string m_listName;
	std::string m_listType;
	std::string m_filePath;
	std::deque<std::string> m_segmentQueue;
	std::vector<int64_t> m_segmentTimes;
	bool		m_bLive;
	size_t		m_size;
	int			m_segmentDuration;
	int64_t     m_startTime;
	size_t      m_beginIndex;
};

#endif
