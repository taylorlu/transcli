#include <stdio.h>

#include "clihelper.h"
#include "logger.h"
#include "StrPro/StrProInc.h"
#include "util.h"
#include "bit_osdep.h"
#define BUFFER_SIZE 1024

static void changeSetting(std::string& stdPref, const char* changeStart, const char* newVal)
{
	if(!changeStart || !newVal) return;
	size_t begPartLen = strlen(changeStart);
	size_t videoModeBegIdx = stdPref.find(changeStart);
	if(videoModeBegIdx != std::string::npos) {
		size_t videoModeEndIdx = stdPref.find("</node>", videoModeBegIdx);
		stdPref.replace(videoModeBegIdx+begPartLen, 
			videoModeEndIdx-videoModeBegIdx-begPartLen, newVal);
	}
}

int CCliHelper::GetStdPrefString(const char *inMediaFile, const char *outDir, const char *outMediaFile,
				const char *XmlConfigFile, const char* templateFile, std::string &outPrefString, const char *tmpDir, bool noThumb)
{
	if (inMediaFile == NULL || outMediaFile == NULL || XmlConfigFile == NULL) {
		logger_err( LOGM_GLOBAL, "Invaild paramters\n");
		return -1;
	}

	bool bPreview = false;
	if(_stricmp(XmlConfigFile, "preview") == 0) {
		XmlConfigFile = "./preset/preview.xml";
		bPreview = true;
	}

	if (!LoadXMLConfig(XmlConfigFile)) {
		logger_err( LOGM_GLOBAL, "Failed to load config file %s\n", XmlConfigFile);
		return -2;
	}

	if (!InitOutStdPrefs(templateFile)) {
		logger_err( LOGM_GLOBAL, "Failed to init out prefstring\n");
		return -3;
	}

	// If it's preview, change some settings
	if(bPreview) {
		changeSetting(m_outStdPrefs, "<node key=\"overall.video.mode\">", "0");
		changeSetting(m_outStdPrefs, "<node key=\"videoenc.x264.showInfo\">", "true");
		changeSetting(m_outStdPrefs, "<node key=\"videoenc.x264.keyint\">", "24");
	}
	
	if (!AdjustPreset(inMediaFile, outDir, tmpDir, outMediaFile)) {
		logger_err( LOGM_GLOBAL, "Failed to adjust preset\n");
		return -4;
	}

	if(noThumb) {
		changeSetting(m_outStdPrefs, "<node key=\"videofilter.thumb.enabled\">", "false");
	}

	outPrefString = m_outStdPrefs;
	return 0;
}

bool CCliHelper::GenMetadataFile(const char *mediaFile, const char *thumbnailFile, const char *metaFile)
{
	if (metaFile == NULL) {
		logger_err( LOGM_GLOBAL, "Invalid meda file name\n");
		return false;
	}

	std::string metaInfo = GetMetaInfo(mediaFile, thumbnailFile);
	if (metaInfo.empty()) {
		logger_err( LOGM_GLOBAL, "Failed to get Meta Info\n");
		return false;
	}

	FILE *fp = fopen(metaFile, "w");
	if (fp == NULL) {
		logger_err( LOGM_GLOBAL, "Failed to open meta file %s to write\n", metaFile);
		return false;
	}

	if (fwrite(metaInfo.c_str(), metaInfo.length(), 1, fp) != metaInfo.length()) {
		logger_err( LOGM_GLOBAL, "Failed to write the meta info\n");
		fclose(fp);
		return false;
	}

	fclose(fp);
	return true;
}

bool CCliHelper::LoadXMLConfig(const char *configFile)
{
	char* fileContent = ReadTextFile(configFile);
	if(!fileContent) return false;

	m_inConfig.clear();
	m_inConfig = fileContent;
	
	// Convert to UTF-8
// 	StrPro::CCharset charConvert;
// 	m_inConfig = charConvert.ANSItoUTF8(buf);
 	free(fileContent);

	return true;
}

CCliHelper* CliHelperFactory(cli_type_t t)
{
	switch (t) {
	//case CLI_TYPE_CISCO:
		//return new CCliHelperCisco;
	case CLI_TYPE_PPLIVE:
	default:
		return new CCliHelperPPLive;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////
#include <sstream>
#include "processwrapper.h"
#include "MEvaluater.h"
#include "util.h"
#include "MediaTools.h"
#include "bit_osdep.h"

int QuickScreenshot(const char *video_file, const char *out_dir, long start_pos, long end_pos, int number)
{
	if (video_file == NULL || end_pos < 0 || start_pos < 0 || number <= 0) {
		logger_err(LOGM_GLOBAL, "QuickScreenshot(): Invalid parameter\n");
		return -1;
	}

	if (!FileExist(video_file)) {
		logger_err(LOGM_GLOBAL, "QuickScreenshot(): The video file (%s) is not existed\n", video_file);
		return -2;
	}

	StrPro::CXML2 mediaInfo;
	double fps = 25.0;
	long duration = -1;

	int frame_interval;

	mediaInfo.New("1.0", "mediainfo");

	if (!GetMediaInfo(video_file, &mediaInfo)) {
		logger_warn(LOGM_GLOBAL, "Failed to parsing Meta information\n");
		fps = 25.0;
	}
	else {
		int fps_num = 0, fps_den = 0;

		if (mediaInfo.goRoot() != NULL && mediaInfo.findChildNode("video") != NULL) {
			if (mediaInfo.findChildNode("fps_num") != NULL) {
				fps_num = atoi(mediaInfo.getNodeValue());
				mediaInfo.goParent();
			}
			if (mediaInfo.findChildNode("fps_den") != NULL) {
				fps_den = atoi(mediaInfo.getNodeValue());
				mediaInfo.goParent();
			}
			if (mediaInfo.findChildNode("duration") != NULL) {
				duration = atol(mediaInfo.getNodeValue()) / 1000;
				mediaInfo.goParent();
			}
		}

		if (fps_num > 0 || fps_den > 0) {
			fps = fps_num * 1.0 / fps_den;
		}
	}

	if (duration > 0 && start_pos > duration ) {
		logger_err(LOGM_GLOBAL, "QuickScreenshot(): the pos is out of \n", video_file);
		return -3;
	}

	if (number == 1) {
		end_pos = 0;
		frame_interval = 1;
	}
	else {
		frame_interval = (int) (end_pos * fps / number);
	}

	for (int i = 0; i < number; i++) {
		char buf[15];

		sprintf(buf, "shot%04d.png", i+1);
		::remove(buf);
	}

	std::ostringstream sstr;

	sstr << MPLAYER << " \"" << video_file << "\""
					<< " -benchmark -vo null"
					<< " -ss " << start_pos
					<< " -endpos " << end_pos
					<< " -vf screenshot=" << frame_interval;
					
	CProcessWrapper mplayer_proc;

	int ret = mplayer_proc.Run(sstr.str().c_str());

	if (ret != 0) {
		logger_err(LOGM_GLOBAL, "QuickScreenshot(): the pos is out of \n", video_file);
		return -3;
	}

	if (out_dir != NULL && *out_dir != '\0') {
		char in_file[MAX_PATH], out_file[MAX_PATH];

		MakeDirRecursively(out_dir);

		for (int i = 0; i < number; i++) {
			_snprintf(in_file, MAX_PATH, "shot%04d.png", i+1);
			_snprintf(out_file, MAX_PATH, "%s%c%s", out_dir, PATH_DELIMITER, in_file);
			::remove(out_file);
            TsMoveFile(in_file, out_file);
		}
	}

	return ret;
}
