#ifndef	MEVALUATER_H
#define MEVALUATER_H
#include <vector>
#include <map>
#include "StrProInc.h"

#define MAX_PATH_LEN 260
#define MCCORE_XML "mcnt.xml"

const char XMLORIGINAL[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><root/>";
const char STAUDIO[] = "audio";
const char STMUXER[] = "muxer";
const char STVIDEO[] = "video";
const char STCONFIG[] = "config";

enum
{
	PREF_TYPE_STRING = 0,
	PREF_TYPE_INT,
	PREF_TYPE_BOOL,
	PREF_TYPE_FLOAT,
};

class CXMLPref;

class  MEvaluater
{
public:
	MEvaluater(void);
	~MEvaluater(void);

	//static int  GetCoreXmlEnumValue(StrPro::CXML2* coreXml, const char* key);
	//static char* GetCoreXmlEnumString(StrPro::CXML2 *prefXml, const char *key);
	static const char* GetCoreXmlFormatValue(const char *key, const char* prefKeyVal);
	static const char* GetDefaultNodeValueByKey(const char* key);
	static bool  UpdatePresets(StrPro::CXML2 *prefxml, StrPro::CXML2& outPrefs);
	static bool  RevertPresets(StrPro::CXML2& outPrefs);
	//static bool	 CreatePresets(StrPro::CXML2& outPrefs);
	static bool	 SetPresetKeyValue(StrPro::CXML2* prefCoreXml, const char* key, const char* value, bool isAdd = false);
	static bool	 SetPresetKeyValue(StrPro::CXML2* prefCoreXml, const char* key, int value, bool isAdd = false);
	static bool	 SetPresetKeyValue(StrPro::CXML2* prefCoreXml, const char* key, float value, bool isAdd = false);
	static const char* GetPresetKeyValue(StrPro::CXML2* prefCoreXml,const char* key);
	static bool  SavePresetFile(StrPro::CXML2* prefs, const char* path);
	static StrPro::CXML2* GetMCcoreDoc();
	//static bool InitStreamsXml(StrPro::CXML2 *streamsPrefs, StrPro::CXML2* mcXml);
	//static bool CreateRootXml(StrPro::CXML2& outPrefs);
	//static CXMLPref* CreateStreamXml(const char* streamType = NULL);
	//static bool SetInputNodeValue(StrPro::CXML2* streamsPrefs, const char* nodeName,  const char* value);
	//static const char* GetInputNodeValue(StrPro::CXML2* streamsPrefs, const char* nodeName);
	//static int	AddStream(StrPro::CXML2* streamsPrefs, const char* type);
	//static int	AddMuxer(StrPro::CXML2* streamsPrefs);
	//static int	GetStreamsCount(StrPro::CXML2* streamsPrefs);
	//static int	GetMuxersCount(StrPro::CXML2* streamsPrefs);
	//static bool SetOutputKeyValue(StrPro::CXML2* streamsPrefs, const char* key,  const char* value, int index = 0, bool isAdd = false);
	//static bool SetOutputKeyValue(StrPro::CXML2* streamsPrefs, const char* key,  int value, int index = 0, bool isAdd = false);
	//static bool SetOutputKeyValue(StrPro::CXML2* streamsPrefs, const char* key,  float value, int index = 0, bool isAdd = false);
	//static const char* GetOutputKeyValue(StrPro::CXML2* streamsPrefs, const char* key, int index = 0);
	//static std::string FillOutputNodeWithPresetFile(const char* rootPref);
private:	
	static bool	 initDefaultPresetFromMcCore(StrPro::CXML2 *mcCorePref, StrPro::CXML2& outPrefs);
	static StrPro::CXML2* getDefaultPrefInstance();
	//static std::string	parseKeyFromMcCore(StrPro::CXML2* coreXml, const char* key, bool isEnumIndex = true, bool fileWithQuots = true, bool isDefault = false);
	static const char* getMcCoreNodeValue(StrPro::CXML2* pCoreXml, const char* nodeType, bool isEnumIndex = true, bool isDefault = false);
};

#define PREF_ERR_NO_INTNODE -1010
#define PREF_ERR_NO_FLOATNODE -1010.f

class CXMLPref
{
public:
	CXMLPref(const char* rootName = NULL);
	bool CreateFromXmlFile(const char* fileName);
	CXMLPref* Clone();

	void* replaceNode(void* newNode);
	void* goRoot();
	void* setAttribute(const char* name, const char* value);
	const char* getAttribute(const char* attrName, const char* defaultValue=NULL, void* node=NULL);
	void setNodeName(const char* nodename);
	const char* getChildNodeValue(const char* name, const char* attrname = NULL, const char* attrvalue = NULL, int index = 0);
	bool isCurrentKeyValueDefault(const char* key);
	int Dump(char** buffer, char* encoding = 0);
	void FreeDump(char** buffer);
	int  GetInt(const char* key, const char* nodeName = NULL);
	float GetFloat(const char* key, const char* nodeName = NULL);
	const char* GetString(const char* key, const char* nodeName = NULL);
	//int  GetEnum(const char* key);
	bool GetBoolean(const char* key, const char* nodeName = NULL);
	bool SetInt(const char* key, int value, const char* nodeName = NULL);
	bool SetFloat(const char* key, float value, const char* nodeName = NULL);
	bool SetString(const char* key, const char* value, bool cdata = true, const char* nodeName = NULL);
	bool SetBoolean(const char* key, bool value, const char* nodeName = NULL);

	bool ExistKey(const char* key);
	//for default int
	~CXMLPref();
private:
	StrPro::CXML2* m_pXml;
};

class CStreamPrefs
{
public:
	CStreamPrefs();
	~CStreamPrefs();

	bool InitStreams(StrPro::CXML2 *streamsPrefs);
	//CXMLPref*  CreateNewStreamAndAppend(const char* style);

	int  GetAudioCount();
	int  GetVideoCount();
	int  GetMuxerCount();

	CXMLPref* GetAudioPrefs(size_t id);
	CXMLPref* GetVideoPrefs(size_t id);
	CXMLPref* GetMuxerPrefs(size_t id);

private:
	std::vector<CXMLPref*> m_audioStreams;
	std::vector<CXMLPref*> m_videoStreams;
	std::vector<CXMLPref*> m_muxerStream;
};

class CRootPrefs
{
public:
	CRootPrefs();
	~CRootPrefs();
	bool			InitRootForMaster(const char *strPrefs);
	bool			InitRoot(const char *strPrefs);
	
	std::string	    DumpXml();
	
	CStreamPrefs*	GetStreamPrefs();
	StrPro::CXML2*	GetSrcMediaInfoDoc();
	
	const char*     GetUrl(){return m_pStrUrl;}
	const char*     GetMainUrl();
	const char*		GetTempDir(){return m_pStrTempDir;}
	const char*		GetSubTitle(){return m_pStrSubTitle;}
	const char*		GetDestDir(){return m_pStrDestDir;}
	const char*		GetSrcFileTitle(){return m_pFileTitle;}
	int				GetPriority() {return m_taskPriority;}

	// Used for decode next(Get other files' mediainfo for concat files) in transnode.
	std::map<std::string, StrPro::CXML2*>& GetMediaInfoNodes() {return m_mediaInfos;}

	// Interfaces for transcli.
	bool SetStreamPref(const char*key, const char *val, const char* type, int index = 0);
	bool SetStreamPref(const char* key, int val, const char* type, int index = 0);
	bool SetStreamPref(const char*key, bool val, const char* type, int index = 0);
	bool SetStreamPref(const char*key, float val, const char* type, int index = 0);
	bool SetStreamPref(const char* key, int keyType, void *val, const char* type, int index = 0);

private:
	CStreamPrefs*	createStreamPrefs();
	bool			initRoot(StrPro::CXML2 *streamsPrefs);
	bool			setStreamPref2(const char* key, int keyType, void *val, const char* type, int index = 0);
	StrPro::CXML2*	createMediaInfo();
	void			clearMediaInfos();
	void			initMediaInfo(StrPro::CXML2& prefDoc);
	void			initFileTitle();

private:
	char*			m_pStrUrl;
	char*			m_pStrTempDir;
	char*			m_pFileTitle;
	char*			m_pStrSubTitle;
	char*			m_pStrDestDir;
	int				m_taskPriority;
	int				m_mainUrlIndex;		// When join multiple files, specify the main url.

	std::vector<std::string> m_srcFiles;
	// Used for tsmaster
	StrPro::CXML2*		m_prefsDoc;  

	// Used for transnode
	CStreamPrefs*		m_outputs;
	StrPro::CXML2*		m_pMainMediaInfo;
	bool				m_bDeleteMainMediaInfo;
	std::map<std::string, StrPro::CXML2*> m_mediaInfos;
};

#endif
