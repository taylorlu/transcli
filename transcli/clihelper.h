#ifndef __CLI_HELPER_H__
#define __CLI_HELPER_H__

#include <string>

using namespace std;

typedef enum {
	CLI_TYPE_CISCO,
	CLI_TYPE_PPLIVE,
} cli_type_t;

class CCliHelper
{
public:
	CCliHelper() {}
	virtual ~CCliHelper() {}

	int GetStdPrefString(const char *inMediaFile, const char *outDir, const char *outMediaFile, const char *XmlConfigFile, 
		const char* templateFile, std::string &outPrefString /*OUT*/, const char *tmpDir = NULL, bool noThumb=false, int mainUrlIndex = 0);
	bool GenMetadataFile(const char *mediaFile, const char *thumbnailFile, const char *metaFile);

	virtual std::string GetMetaInfo(const char *mediaFile, const char *thumbnailFile = NULL) = 0;

	bool LoadXMLConfig(const char *configFile);

	virtual bool InitOutStdPrefs(const char* templateFile) = 0;
	virtual bool AdjustPreset(const char *inMediaFile, const char *outDir, const char *tmpDir, int mainUrlIndex = 0, const char *outMediaFile= NULL) = 0;
	virtual cli_type_t GetCliType() = 0;

protected:
	std::string m_inConfig;
	std::string m_outStdPrefs;
};

//class CCliHelperCisco: public CCliHelper
//{
//public:
//	virtual std::string GetMetaInfo(const char *mediaFile, const char *thumbnailFile);
//	virtual cli_type_t GetCliType() { return CLI_TYPE_CISCO; }
//
//private:
//	virtual bool InitOutStdPrefs(const char* templateFile);
//	virtual bool AdjustPreset(const char *inMediaFile, const char *outDir, const char *outMediaFile);
//};

class CCliHelperPPLive: public CCliHelper
{
public:
	virtual std::string GetMetaInfo(const char *mediaFile, const char *thumbnailFile);
	virtual cli_type_t GetCliType() { return CLI_TYPE_PPLIVE; }

private:
	virtual bool InitOutStdPrefs(const char* templateFile);
	virtual bool AdjustPreset(const char *inMediaFile, const char *outDir, const char *tmpDir, int mainUrlIndex = 0, const char *outMediaFile = NULL);
};

//////////////////////////////////////////////////////////////////////////////////////////
int QuickScreenshot(const char *video_file, const char *out_dir, long start_pos, long end_pos, int number);

#endif

