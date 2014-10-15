#include "MediaTools.h"
#include <string.h>
#include <sstream>

#include "util.h"
#include "yuvUtil.h"
#include "tsdef.h"
#include "StrProInc.h"
#include "processwrapper.h"
#include "bit_osdep.h"

using namespace std;

#define MEDIAINFO_INIT_CONTENT "<?xml version=\"1.0\" encoding=\"UTF-8\"?><mediainfo/>"

static int GetBitrate(const char *s)
{
	if(!s || !(*s)) return 0;
	int br = (int)atof(s);	//bit/s
	return br;
}

static int GetSizeKB(const char *s)
{
	if(!s || !(*s)) return 0;
	double br = atof(s);
	br /= 1024;
	return (int)br;
}

static int GetDuration(const char* timestr)
{
	if(!timestr || !(*timestr)) return 0;
	int dur = (int)(atof(timestr) * 1000);	// To ms
	return dur;
}

static bool IsThisUrlSpecial(const char* instr)
{
	if (instr && strstr(instr, "://") != NULL) {
		return true;
	}
	return false;
}

static void removeTagContent(std::string& infoContent, const char* tagName, std::vector<const char*> exceptKeys)
{
	char tagStartName[32] = {0};
	sprintf(tagStartName, "<%s", tagName);
	const char* tagEndName = "/>";

	size_t tagStart = infoContent.find(tagStartName);
	size_t tagEnd = infoContent.find(tagEndName, tagStart+2);
	
	// Remove all content enclosed by tagName
	while (tagStart != std::string::npos && tagEnd != std::string::npos) {
		bool keepCurTag = false;
		for(size_t i=0; i<exceptKeys.size(); ++i) {
			const char* exceptKey = exceptKeys[i];
			if(exceptKey) {		// Skip removing except tag
				std::string exceptPartStart = tagStartName;
				exceptPartStart.append(" key=\"").append(exceptKey).append("\"");
				if(infoContent.find(exceptPartStart, tagStart) == tagStart) {	// Match expcept tag
					keepCurTag = true;
					break;
				}
			}
		}

		if(!keepCurTag) {
			infoContent.replace(tagStart, tagEnd-tagStart+strlen(tagEndName), " ");
		}
		tagStart = infoContent.find(tagStartName, tagStart+2);
		tagEnd = infoContent.find(tagEndName, tagStart+2);
	}
}

static bool nodeHasContent(std::string& infoXml, size_t startPos, const char* nodeName)
{
	if(!nodeName || !(*nodeName)) return false;
	size_t nodeStartPos = infoXml.find(nodeName, startPos);
	if(nodeStartPos == std::string::npos) return false;
	if(infoXml.length() <= nodeStartPos + strlen(nodeName)) return false;	// invalid xml
	if(infoXml.at(nodeStartPos + strlen(nodeName)) == '<') return false;	// empty node value
	return true;
}

static void normalizeTagValue(std::string& infoXml, const char* tagName)
{
	std::string tagStartStr = "<tag key=\"";
	tagStartStr += tagName;
	tagStartStr += "\"";
	size_t tagStartIdx = infoXml.find(tagStartStr);
	while(tagStartIdx != std::string::npos) {
		size_t valueStartIdx = tagStartIdx+tagStartStr.size()+strlen(" value=\"");
		size_t valueEndIdx = infoXml.find('"', valueStartIdx);
		if(valueEndIdx != std::string::npos) {
			bool bInvalidChar = false;
			for(size_t i=valueStartIdx; i<valueEndIdx; ++i) {
				char curChar = infoXml.at(i);
				if(curChar < 'A' || curChar > 'z') {	// beyond A~z
					bInvalidChar = true;
					break;
				}
			}
			if(bInvalidChar) {	// replace invalid lang code to "chi"
				infoXml.replace(valueStartIdx, valueEndIdx-valueStartIdx, "chi");
			}
			tagStartIdx = infoXml.find(tagStartStr, valueStartIdx);
		}
	}
}

static bool isQiyiFlv = false;

static std::string GetMediaInfoXML(const char *mediaPath)
{
	if (mediaPath == NULL || *mediaPath == '\0') return "";
	if (IsThisUrlSpecial(mediaPath)) {
		return "";
	} if(!FileExist(mediaPath)) {
		fprintf(stderr, "%s can not be found!!\n", mediaPath);
		return "";	
	}

	std::string strResult;
	std::string cmdStr = FFPROBE" -analyzeduration 20000000 -i \"";
	cmdStr += mediaPath;
	cmdStr += "\" -of xml -show_streams -show_format -detect_inter_frames 4 -v quiet";
	cmdStr += " -show_entries format=duration,size,bit_rate,format_name,probe_score:format_tags=encoder,metadatacreator:stream_tags=language,rotate";
#ifdef DEBUG_EXTERNAL_CMD
	printf("%s\n", cmdStr.c_str());
#endif

	CProcessWrapper proc;
	proc.flags = SF_REDIRECT_STDOUT | SF_ALLOC;
	bool success = (proc.Create(cmdStr.c_str()) && proc.ReadStdout() > 0);
	if (!success) return "";

	// Get mediainfo part string(xml) from console buffer
	char *p = NULL;
	for (p = proc.conbuf; *p && memcmp(p, "<?xml", 5); p++);
	char* docEndPos = strstr(p, "</ffprobe>");
	if(docEndPos) {
		*(docEndPos+strlen("</ffprobe>")) = 0;
	} else {
		return "";
	}
	strResult = p;

	if(strResult.find("metadatacreator") != std::string::npos &&
		strResult.find("Moyea FLV Lib") != std::string::npos) {
		isQiyiFlv = true;
	}

	// Remove <Movie> and <Preformer> node, to avoid messy code string
	/*const char* uselessTags[] = {"disposition", "tag"};
	for(int i=0; i<sizeof(uselessTags)/sizeof(uselessTags[0]); ++i) {
		// Remove all tags except language tag
		std::vector<const char*> exceptKeys;
		exceptKeys.push_back("language");
		exceptKeys.push_back("title");
		exceptKeys.push_back("rotate");
		removeTagContent(strResult, uselessTags[i], exceptKeys);
	}*/
	// Normalize language tag(remove messy code in language)
	normalizeTagValue(strResult, "language");

	// Under linux, ffprobe generate xml contain file name encoding by gb2132, should convert to utf-8
	#ifndef _WIN32
	StrPro::CCharset charConvert;
	char* utf8Xml = charConvert.ANSItoUTF8(strResult.c_str());
	if(utf8Xml) {
		strResult = utf8Xml;
		free(utf8Xml);
	}	
	#endif 
	return strResult;
}

static void parseGeneralInfo(StrPro::CXML2& xml, StrPro::CXML2 *mediaInfo)
{
	mediaInfo->goRoot();
	mediaInfo->SetCurrentNode(mediaInfo->addChild("general"));
	
	const char* durStr = xml.getAttribute("duration");
	if(durStr && *durStr) {
		mediaInfo->addChild("duration", GetDuration(durStr));
	}
	const char* bitrateStr = xml.getAttribute("bit_rate");
	if(bitrateStr && *bitrateStr) {
		mediaInfo->addChild("bitrate", GetBitrate(bitrateStr));
	}
	const char* sizeStr = xml.getAttribute("size");
	if(sizeStr && *sizeStr) {
		mediaInfo->addChild("filesize", GetSizeKB(sizeStr));
	}

	const char* formatStr = xml.getAttribute("format_name");
	if(formatStr && *formatStr) {
		/*const char* firstDelim = strchr(formatStr, ' ');
		if(!firstDelim) firstDelim = strchr(formatStr, '/');
		if(!firstDelim) firstDelim = strchr(formatStr, '(');
		// Get front part of format long name
		char shortFormat[33] = {0};
		int copyLen = 32;
		if(firstDelim) copyLen = firstDelim - formatStr;
		strncpy(shortFormat, formatStr, copyLen);*/
		mediaInfo->addChild("container", formatStr);
	}
}

static void parseAudioInfo(StrPro::CXML2& xml, StrPro::CXML2 *mediaInfo, int audioIdx)
{
	mediaInfo->goRoot();
	mediaInfo->SetCurrentNode(mediaInfo->addChild("audio"));
	mediaInfo->addChild("index", audioIdx);
	const char* durStr = xml.getAttribute("duration");
	if(durStr && *durStr) {
		mediaInfo->addChild("duration", GetDuration(durStr));
	}
	const char* bitrateStr = xml.getAttribute("bit_rate");
	if(bitrateStr && *bitrateStr) {
		mediaInfo->addChild("bitrate", GetBitrate(bitrateStr));
	}
	const char* idStr = xml.getAttribute("id");
	if(idStr && *idStr) {
		int idNum = (int)strtol(idStr, 0, 16);
		mediaInfo->addChild("id", idNum);
	}
	const char* codecName = xml.getAttribute("codec_name");
	if(codecName && *codecName) {
		mediaInfo->addChild("codec", codecName);
	}
	const char* channelStr = xml.getAttribute("channels");
	if(channelStr && *channelStr) {
		mediaInfo->addChild("channels", channelStr);
	}
	const char* srateStr = xml.getAttribute("sample_rate");
	if(srateStr && *srateStr) {
		mediaInfo->addChild("samplerate", srateStr);
	}

	// Parse audio language
	if(xml.findChildNode("tag", "key", "language")) {
		const char* langStr = xml.getAttribute("value");
		if(langStr && *langStr) {
			mediaInfo->addChild("lang", langStr);
		}
		xml.goParent();
	}
	// Parse title attribute
	if(xml.findChildNode("tag", "key", "title")) {
		const char* titleStr = xml.getAttribute("value");
		if(titleStr && *titleStr) {
			mediaInfo->addChild("title", titleStr);
		}
		xml.goParent();
	}
}

static void parseVideoInfo(StrPro::CXML2& xml, StrPro::CXML2 *mediaInfo, int videoIdx)
{
	const char* codecName = xml.getAttribute("codec_name");
	if(codecName && *codecName) {
		const char* frameNumStr = xml.getAttribute("nb_read_frames");
		// This may be mp3 file with jpeg cover, so actually it's not a video stream
		if(frameNumStr && atoi(frameNumStr) == 1 && !_stricmp(codecName, "mjpeg")) {
			return;
		}
	}

	int interlaced = VID_PROGRESSIVE;	//Default
	bool isVfr = false;
	int width = 0, height = 0;
	mediaInfo->goRoot();
	mediaInfo->SetCurrentNode(mediaInfo->addChild("video"));

	mediaInfo->addChild("index", videoIdx);
	if(codecName && *codecName) {
		mediaInfo->addChild("codec", codecName);
	}
	const char* durStr = xml.getAttribute("duration");
	if(durStr && *durStr) {
		mediaInfo->addChild("duration", GetDuration(durStr));
	}
	const char* bitrateStr = xml.getAttribute("bit_rate");
	if(bitrateStr && *bitrateStr) {
		mediaInfo->addChild("bitrate", GetBitrate(bitrateStr));
	}
	const char* idStr = xml.getAttribute("id");
	if(idStr && *idStr) {
		mediaInfo->addChild("id", atoi(idStr));
	}
	const char* profileName = xml.getAttribute("profile");
	if(profileName && *profileName) {
		mediaInfo->addChild("profile", profileName);
	}
	const char* widthStr = xml.getAttribute("width");
	if(widthStr && *widthStr) {
		width = atoi(widthStr);
		mediaInfo->addChild("width", widthStr);
	}
	const char* heightStr = xml.getAttribute("height");
	if(heightStr && *heightStr) {
		height = atoi(heightStr);
		mediaInfo->addChild("height", heightStr);
	}
	const char* interlaceStr = xml.getAttribute("interlaced_frame");
	if(interlaceStr) {
		const char* topFirstStr = xml.getAttribute("top_field_first");
		bool bInterlaced = (atoi(interlaceStr) == 1);
		bool bTopFirst = (topFirstStr && atoi(topFirstStr) == 1);
		if(bTopFirst) bInterlaced = true;	// If topFirst is true then it's interlaced
		if(bInterlaced) {
			if(bTopFirst) {
				interlaced = VID_INTERLACE_TFF;
			} else {
				interlaced = VID_INTERLACE_BFF;
			}
		}
	}
	mediaInfo->addChild("interlaced", interlaced);

	const char* realFpsStr = xml.getAttribute("r_frame_rate");
	const char* avgFpsStr = xml.getAttribute("avg_frame_rate");
	if(realFpsStr && avgFpsStr) {
		int realNum = atoi(realFpsStr);
		int realDen = 0;
		const char* delim = strchr(realFpsStr, '/');
		if(delim) {
			realDen = atoi(delim+1);
		}

		int avgNum = atoi(avgFpsStr);
		int avgDen = 0;
		delim = strchr(avgFpsStr, '/');
		if(delim) {
			avgDen = atoi(delim+1);
		}

		if(realNum != avgNum || realDen != avgDen) {
			// If real fps is equal to avg fps and it's progressive, it's vfr
			if(interlaced == VID_PROGRESSIVE || realNum != avgNum*2) {
				if(!isQiyiFlv) {
					isVfr = true;
				}
			}
			if(avgNum == 0 || avgDen == 0) {
				avgNum = realNum;
				avgDen = realDen;
				//isVfr = false;
			}
		}

		double fps = double(avgNum)/avgDen;
		if(isQiyiFlv && realDen > 0 && realNum > 0) {
			fps = double(realNum)/realDen;
		}
		GetFraction(fps, &avgNum, &avgDen);
		if(fps > 32 && fps < 60) {
			GetFraction(fps/2, &avgNum, &avgDen);
			isVfr = true;
		} else if(fps > 60) {
			avgNum = 24;
			avgDen = 1;
			isVfr = true;
		} /*else if(avgNum > 60000 || avgDen > 60000) {
		}*/
		
		mediaInfo->addChild("fps_num", avgNum); 
		mediaInfo->addChild("fps_den", avgDen);
		mediaInfo->addChild("is_vfr", isVfr ? 1 : 0);
	}

	// Video sync method: use pass through
	//if(isQiyiFlv) {
	//	mediaInfo->addChild("passthrough", 1);
	//}

	const char* darStr = xml.getAttribute("display_aspect_ratio");
	if(darStr && *darStr) {
		int darNum = atoi(darStr);
		int darDen = 0;
		const char* delim = strchr(darStr, ':');
		if(delim) {
			darDen = atoi(delim+1);
		}
		// Check dar later in CTransWorkerSeperate(initAVSrcVideo)
		/*if(darNum == 0 || darDen == 0) {
			if(width > 0 && height > 0) {
				GetFraction(double(width)/height, &darNum, &darDen);
			}
		}*/
		mediaInfo->addChild("dar_num", darNum); 
		mediaInfo->addChild("dar_den", darDen);
	}
	
	// Parse video rotation attribute
	if(xml.findChildNode("tag", "key", "rotate")) {
		const char* rotateStr = xml.getAttribute("value");
		if(rotateStr && *rotateStr) {
			mediaInfo->addChild("rotate", rotateStr);
		}
		xml.goParent();
	}
	// Parse title attribute
	if(xml.findChildNode("tag", "key", "title")) {
		const char* titleStr = xml.getAttribute("value");
		if(titleStr && *titleStr) {
			mediaInfo->addChild("title", titleStr);
		}
		xml.goParent();
	}
}

static void parseSubtitleInfo(StrPro::CXML2& xml, StrPro::CXML2 *mediaInfo, int subIdx)
{
	mediaInfo->goRoot();
	mediaInfo->SetCurrentNode(mediaInfo->addChild("subtitle"));
	mediaInfo->addChild("index", subIdx);
	const char* durStr = xml.getAttribute("duration");
	if(durStr && *durStr) {
		mediaInfo->addChild("duration", GetDuration(durStr));
	}
	const char* codecName = xml.getAttribute("codec_name");
	if(codecName && *codecName) {
		mediaInfo->addChild("codec", codecName);
	}
	
	const char* streamIdx = xml.getAttribute("index");
	if(streamIdx && *streamIdx) {
		mediaInfo->addChild("streamid", streamIdx);
	}
	// Parse subtitle language
	if(xml.findChildNode("tag", "key", "language")) {
		const char* langStr = xml.getAttribute("value");
		if(langStr && *langStr) {
			if(_stricmp(langStr, "zho") == 0) {
				langStr = "cht";
			}
			mediaInfo->addChild("lang", langStr);
		}
		xml.goParent();
	}
	// Parse title attribute
	if(xml.findChildNode("tag", "key", "title")) {
		const char* titleStr = xml.getAttribute("value");
		if(titleStr && *titleStr) {
			mediaInfo->addChild("title", titleStr);
		}
		xml.goParent();
	}
}

bool GetMediaInfo(const char *mediaPath /*IN*/, StrPro::CXML2 *mediaInfo/*OUT*/)
{
	std::string xmlText = GetMediaInfoXML(mediaPath);
	if(xmlText.empty()) return false;
	StrPro::CXML2 probeDoc;
	if (probeDoc.Read(xmlText.c_str(), xmlText.length()) != 0) return false;

	// Check if audio parameter is normal(Mediainfo detect AAC-LTP/Main has problem)
	if(probeDoc.goRoot()) {
		if(probeDoc.findChildNode("format")) {
			if(probeDoc.getAttributeInt("probe_score") < 20) {
				printf("Probe score is low, unknown format.\n");
				return false;
			}
			parseGeneralInfo(probeDoc, mediaInfo);
		}
		probeDoc.goRoot();

		if(probeDoc.findChildNode("streams") && probeDoc.goChild()) {
			int audioIdx = 0, videoIdx = 0, subIdx = 0;
			do {
				const char* codecName = probeDoc.getAttribute("codec_name");
				if(codecName && !_stricmp(codecName, "ansi")) continue;	// Skip text file
				const char* streamType = probeDoc.getAttribute("codec_type");
				if(streamType && *streamType) {
					if(!_stricmp(streamType, "video")) {
						parseVideoInfo(probeDoc, mediaInfo, videoIdx);
						videoIdx++;
					} else if (!_stricmp(streamType, "audio")) {
						parseAudioInfo(probeDoc, mediaInfo, audioIdx);
						audioIdx++;
					} else if (!_stricmp(streamType, "subtitle")) {
						parseSubtitleInfo(probeDoc, mediaInfo, subIdx);
						subIdx++;
					}
				}
			} while(probeDoc.goNext());
		}
	}

	//char* tmp=NULL;
	//mediaInfo->Dump(&tmp);

	return true;
}

