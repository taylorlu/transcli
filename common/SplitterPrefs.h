#ifndef __COMMON_SPLITTER_PREFS__
#define __COMMON_SPLITTER_PREFS__
#include <string>
#include <vector>
#include <stdint.h>

enum SplitType_t {
	SPLIT_TYPE_UNKNOWN = -1,
	SPLIT_TYPE_NONE = 0,
	SPLIT_TYPE_AVERAGE,
	SPLIT_TYPE_NORMAL
};

enum SplitSubfixType_t {
	SPLIT_SUBFIX_TYPE_INDEX = 0, // default value
	SPLIT_SUBFIX_TYPE_TIME
};

enum SplitUnitType_t {
	SPLIT_UNIT_BYSIZE = 0,  // default value
	SPLIT_UNIT_BYTIME,
};

/*
	A example of split node: 
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<split type=\"normal\" prefix="video" subfix=\"index\" unitType=\"size\">"
		"  <clips>-1000|1000-4000|5000-</clips>"
		"</split>";
*/

class CSplitterPrefs
{
public:
	CSplitterPrefs();
	bool Parsing(const std::string &strSpliterPrefs);
	std::pair<int64_t, int64_t> GetClip(int clipIdx);
	std::vector<std::pair<int64_t, int64_t> > & GetClipsMark() { return m_clipMarks; }
	int GetInterval() const { return m_interval; }
	void SetInterval(int interval) {m_interval = interval;}
	SplitType_t GetType() const { return m_type; }
	
	SplitSubfixType_t GetSubfixType() const { return m_subfixType; }
	SplitUnitType_t GetUnitType() const { return m_unitType; }
	void SetUnitType(SplitUnitType_t unitType) {m_unitType = unitType;}

	std::string GetSplitInfoXml() {return m_strSplitInfoXml;}
	std::string GetPrefix() {return m_prefix;}
	bool EanbleConfigGenerate() {return m_bGenConfig;}

private:
	bool ParsingClipMarks(const char *strClipMarks);

private:
	SplitType_t m_type;
	SplitSubfixType_t m_subfixType;
	SplitUnitType_t m_unitType;
	int m_interval;
	std::vector<std::pair<int64_t, int64_t> > m_clipMarks;

	std::string m_strSplitInfoXml;
	std::string m_prefix;
	bool m_bGenConfig;
};

#endif
