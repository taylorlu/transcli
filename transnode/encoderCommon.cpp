#include <time.h>
#ifdef WIN32
#include <direct.h>
#endif
#include "encoderCommon.h"
#include "StreamOutput.h"
#include "vfhead.h"
#include "MEvaluater.h"
#include "WaterMarkFilter.h"
#include "ThumbnailFilter.h"
#include "util.h"

const char* GetFFMpegAudioCodecName(int id)
{
	switch (id) {
	case AC_MP3:		return "libmp3lame";
	case AC_AAC_LC:		return "libfaac";
	case AC_AAC_HE:
	case AC_AAC_HEV2:	return "libfdk_aac";
	case AC_WMA7:		return "wmav1";
	case AC_WMA8:		return "wmav2";
	case AC_AC3:		return "ac3";
	case AC_EAC3:		return "eac3";
	case AC_MP2:		return "mp2";
	case AC_AMR:		return "libopencore_amrnb";
	case AC_PCM:		return "pcm_s16le";
	case AC_FLAC:		return "flac";
	case AC_APE:		return "ape";
	default:			return 0;
	}
}

const char* GetFFMpegVideoCodecName(int id)
{
	switch (id) {
	case VC_XVID:		return "mpeg4";
	case VC_H264:		return "libx264";
	case VC_MPEG1:		return "mpeg1video";
	case VC_MPEG2:		return "mpeg2video";
	case VC_MPEG4:		return "mpeg4";
	case VC_FLV:		return "flv";
	case VC_MSMPEG4V2:	return "msmpeg4v2";
	case VC_H261:		return "h261";
	case VC_H263:		return "h263";
	case VC_H263P:		return "h263p";
	case VC_HUFFYUV:	return "huffyuv";
	case VC_SNOW:		return "snow";
	case VC_DV:			return "dvvideo";
	case VC_MJPEG:		return "mjpeg";
	case VC_LJPEG:		return "ljpeg";
	case VC_RM:			return "rv20";
	case VC_DIRAC:		return "libdirac";
	case VC_THEORA:		return "libtheora";
	case VC_WMV7:		return "wmv1";
	case VC_WMV8:		return "wmv2";
	default:			return 0;
	}
}

const char* GetFFMpegFormatName(int id)
{
	switch (id) {
	case CF_AVI:	return "avi";
	case CF_MP4:	return "mp4";
	case CF_MOV:	return "mov";
	case CF_MKV:	return "matroska";
	case CF_3GP:	return "3gp";
	case CF_3GP2:	return "3g2";
	case CF_MPEG1:	return "vcd";
	case CF_MPEG2:	return "vob";
	case CF_MPEG2TS:	return "mpegts";
	case CF_FLV:	return "flv";
	case CF_ASF:    return "asf";
	case CF_MJPEG:	return "mjpeg";
	case CF_RM:		return "rm";
	case CF_RAW:	return "rawvideo";
	case CF_OGG:	return "ogg";
	case CF_NUT:	return "nut";
	case CF_DV:		return "dv";
	default:		return "out";
	}
}

const char* GetFFMpegOutFileExt(int id, bool ifAudioOnly)
{
	switch (id) {
	case CF_MPEG1:
	case CF_MPEG2:
		return "mpg";
	case CF_MPEG2TS:
		return "ts";
	case CF_FLV:
		return "flv";
	case CF_ASF:
		return ifAudioOnly ? "wma" : "wmv";
	case CF_AVI:
		return "avi";
	case CF_MP4:
		return ifAudioOnly ? "m4a" : "mp4";
	case CF_MOV:
		return "mov";
	case CF_3GP:
		return "3gp";
	case CF_3GP2:
		return "3g2";
	case CF_MKV:
		return ifAudioOnly ? "mka" : "mkv";
	case CF_OGG:	
		return "ogm";
	}
	return "stream";
}

int GetAudioBitrateForAVEnc(CXMLPref* pAudioPref)
{
	int bitRate = pAudioPref->GetInt("audioenc.ffmpeg.bitrate");
	switch(pAudioPref->GetInt("overall.audio.format")) {
	case AC_MP3:
		bitRate = pAudioPref->GetInt("audioenc.lame.bitrate");
		break;
	case AC_AAC_LC:
		bitRate = pAudioPref->GetInt("audioenc.faac.bitrate");
		break;
	}
	if(bitRate <= 0) bitRate = 128;
	return bitRate;
}

bool parseWaterMarkInfo(CWaterMarkManager*& pWatermarkMan,CXMLPref* videoPref,video_info_t* pVinfo)
{
	bool enableWaterMark = videoPref->GetBoolean("videofilter.overlay.enabled");

	std::string imgFilePath;
	std::string imgFolderPath;
	StrPro::CCharset encodeConvert;

	const char* imageFile = videoPref->GetString("videofilter.overlay.image");
	char* ansiFile = encodeConvert.UTF8toANSI(imageFile);
	if(ansiFile) {
		imgFilePath = ansiFile;
		free(ansiFile);
	}

	const char* imageFloder = videoPref->GetString("videofilter.overlay.imageFolder");
	char* ansiFolder = encodeConvert.UTF8toANSI(imageFloder);
	if(ansiFolder) {
		imgFolderPath = ansiFolder;
		free(ansiFolder);
	}

	if(enableWaterMark && (imageFile || imageFloder)) {
		bool enableColor = videoPref->GetBoolean("videofilter.overlay.colored");
		int wkPos = videoPref->GetInt("videofilter.overlay.pos");		// Relative pos
		int offsetX = videoPref->GetInt("videofilter.overlay.offsetX");
		int offsetY = videoPref->GetInt("videofilter.overlay.offsetY");
		int logoW = videoPref->GetInt("videofilter.overlay.width");
		int logoH = videoPref->GetInt("videofilter.overlay.height");
		int speed = videoPref->GetInt("videofilter.overlay.speed");
		int waitTime = 0;
		const char* routeStr = NULL;
		if(speed > 0) {
			waitTime = videoPref->GetInt("videofilter.overlay.waitTime");
			routeStr = videoPref->GetString("videofilter.overlay.route");
		}

		int transColorR = videoPref->GetInt("videofilter.overlay.transparentR");
		int transColorG = videoPref->GetInt("videofilter.overlay.transparentG");
		int transColorB = videoPref->GetInt("videofilter.overlay.transparentB");
		float opaqueFactor = 1.f - videoPref->GetFloat("videofilter.overlay.opaque");
		if(opaqueFactor < 0) opaqueFactor = 0.f;
		else if(opaqueFactor > 1.f) opaqueFactor = 1.f;

		int intervalMs = videoPref->GetInt("videofilter.overlay.interval");
		// pos number(different time interval different position)
		// int posNum = videoPref->GetInt("videofilter.overlay.posNum");	

		pWatermarkMan = new CWaterMarkManager();
		CWaterMarkFilter* pWaterMark = NULL;
		if(!imgFilePath.empty()) {
			if(imgFilePath.compare(0, 5, "$TIME") == 0) {
				std::string timeFormat = GetStringBetweenDelims(imgFilePath.c_str(), "(", ")");
				if(timeFormat.empty()) timeFormat = "%X";
				pWaterMark = pWatermarkMan->AddTimeStringMark(timeFormat.c_str(), 24);
			} else if (imgFilePath.compare(0, 5, "$TEXT") == 0) {
				std::string textStr = GetStringBetweenDelims(imgFilePath.c_str(), "(", ")");
				if(!textStr.empty()) {
					pWaterMark = pWatermarkMan->AddTextMark(textStr.c_str());
				}
			} else {
				std::vector<std::string> vctFiles;
				StrPro::StrHelper::splitString(vctFiles, imgFilePath.c_str(), "|");
				std::vector<const char*> vctImgFiles;
				for (size_t i=0; i<vctFiles.size(); ++i) {
					vctImgFiles.push_back(vctFiles[i].c_str());
				}
				pWaterMark = pWatermarkMan->AddWaterMark(vctImgFiles, false);
			}
		} else if(!imgFolderPath.empty()) {
			pWaterMark = pWatermarkMan->AddWaterMark(imgFolderPath.c_str());
		}

		if(pWaterMark) {
			if(intervalMs > 0) pWaterMark->SetInterval(intervalMs);
			pWaterMark->EnableColor(enableColor);
			pWaterMark->SetAlpha(opaqueFactor);
			pWaterMark->SetYUVFrameSize(pVinfo->res_out.width, pVinfo->res_out.height);
			pWaterMark->SetYUVFps((float)pVinfo->fps_in.num/pVinfo->fps_in.den);
			if(transColorR != 255 || transColorG != 0 || transColorB != 255) {
				pWaterMark->SetTransparentColor(Vector3ub(transColorR, transColorG, transColorB));
			}

			// logo position
			const char* absolutePos = videoPref->GetString("videofilter.overlay.pos1");
			if(!absolutePos || !(*absolutePos)) {	// Relative position
				pWaterMark->AddShowTimeAndPosition(0, -1, (water_mark_pos_t)wkPos, offsetX, offsetY,
					logoW, logoH, speed, routeStr, waitTime);
			} else {	// Add absolute position
				const int maxLogoNum = 4;
				for (int i=1; i<=maxLogoNum; ++i) {
					char posKey[32] = {0};
					sprintf(posKey, "videofilter.overlay.pos%d", i);
					const char* timePosStr = videoPref->GetString(posKey);
					if(!timePosStr || !(*timePosStr)) continue;
					std::vector<int> vctVal;
					if(StrPro::StrHelper::parseStringToNumArray(vctVal, timePosStr)
								&& vctVal.size() == 6) {
						if(vctVal.at(4) <= 0 || vctVal[5] <= 0) continue;
						pWaterMark->AddShowTimeAndPosition(vctVal.at(0), vctVal.at(1),
							vctVal.at(2), vctVal.at(3), vctVal[4], vctVal[5]);
					}
				}
			} 
		} 
	}
	return true;
}

bool parseThumbnailInfo(CThumbnailFilter*& pThumbnail, CXMLPref* videoPref,
	                    video_info_t* pVinfo, std::string destFileName)
{
	bool enableThumb = videoPref->GetBoolean("videofilter.thumb.enabled");
	if(!enableThumb) return true;
	const char* folderPath = videoPref->GetString("videofilter.thumb.folder");
	StrPro::CCharset encodeConvert;
	std::string thumbFolder;
	char* ansiFolder = encodeConvert.UTF8toANSI(folderPath);
	if(ansiFolder) {
		thumbFolder = ansiFolder;
		free(ansiFolder);
	}
	
	const char* prefixName = videoPref->GetString("videofilter.thumb.name");
	std::string strPrefix;
	char* ansiName = encodeConvert.UTF8toANSI(prefixName);
	if(ansiName) {
		strPrefix = ansiName;
		free(ansiName);
	}

	int thumbFormat = videoPref->GetInt("videofilter.thumb.format");
	int thumbW = videoPref->GetInt("videofilter.thumb.width");
	int thumbH = videoPref->GetInt("videofilter.thumb.height");
	int startTime = videoPref->GetInt("videofilter.thumb.start");
	int endTime = videoPref->GetInt("videofilter.thumb.end");
	int thumbCount = videoPref->GetInt("videofilter.thumb.count");
	bool bStitching = videoPref->GetBoolean("videofilter.thumb.stitching");
	int thumbAlign = videoPref->GetInt("videofilter.thumb.align");
	int imageQuality = videoPref->GetInt("videofilter.thumb.quality");
	int cropMode = videoPref->GetInt("videofilter.thumb.crop");
	bool enableMultiSize = videoPref->GetBoolean("videofilter.thumb.multiSize");
	bool enablePack = videoPref->GetBoolean("videofilter.thumb.pack");
	bool bOptimize = videoPref->GetBoolean("videofilter.thumb.optimize");
	int thumbInterval = videoPref->GetInt("videofilter.thumb.interval");
	int thumbRow = videoPref->GetInt("videofilter.thumb.row");
	int thumbCol = videoPref->GetInt("videofilter.thumb.col");
	pThumbnail = new CThumbnailFilter();
		
	if(thumbFolder.empty()) {
		size_t startIdx = destFileName.find_last_of('\\');
		if(startIdx == std::string::npos) {
			startIdx = destFileName.find_last_of('/');
		}
		thumbFolder = destFileName.substr(0, startIdx);
	}
	pThumbnail->SetFolder(thumbFolder.c_str());

	if(!strPrefix.empty()) {
		pThumbnail->SetPrefixName(strPrefix.c_str());
	} else {
		std::string fileTitle;
		StrPro::StrHelper::getFileTitle(destFileName.c_str(), fileTitle);
		pThumbnail->SetPrefixName(fileTitle.c_str());
	}

	const char* postfix = videoPref->GetString("videofilter.thumb.postfix");
	if(postfix && *postfix) {
		pThumbnail->SetPostfixName(postfix);
	}

	fraction_t dar = pVinfo->dest_dar;
	if(dar.den > 0) {
		pThumbnail->SetVideoDar((float)dar.num/dar.den);
		NormalizeResolution(thumbW, thumbH, dar.num, dar.den);
	}
	pThumbnail->SetThumbnailSize(thumbW, thumbH);
	pThumbnail->SetImageFormat(thumbFormat);
	pThumbnail->SetYUVFrameSize(pVinfo->res_out.width, pVinfo->res_out.height);
	pThumbnail->SetFps((float)pVinfo->fps_out.num/pVinfo->fps_out.den);
	float vdur = pVinfo->duration/1000.f;
	if(vdur <= 0) {
		vdur = 3600.f*6;		// 6 hour
	} else if(vdur < 1) {
		vdur = 1;
	} else if(vdur > 3600*12) {
		vdur = 3600.f*12;		// 12 hour
	}
	
	if(vdur < 20) {
		startTime = 1;
	}
	pThumbnail->SetDuration((int)vdur);
	pThumbnail->SetStartTime(startTime);
	pThumbnail->SetEndTime(endTime);
	if(imageQuality > 0 && imageQuality < 100) {
		pThumbnail->SetImageQuality((unsigned int)imageQuality);
	}
	pThumbnail->SetCropMode(cropMode);
	if(thumbCount > 0) pThumbnail->SetThumbnailCount(thumbCount);
	if(bStitching) {
		pThumbnail->SetEanbleStitching(bStitching);
		pThumbnail->SetStitchAlign(thumbAlign);
	}
	if(thumbInterval > 0) {
		pThumbnail->SetThumbnailInterval(thumbInterval);
	}
	if(thumbRow > 0) {
		pThumbnail->SetStitchGrid(thumbRow, thumbCol);
	}
	// Pack image 
	pThumbnail->SetEnablePackImage(enablePack);
	pThumbnail->SetEnableOptimize(bOptimize);

	// Multi size output support
	if(enableMultiSize) {
		pThumbnail->SetEnableMultiSize(true);
		if(thumbW > 0 && thumbH > 0) {
			pThumbnail->AddThumbSize(Vector2i(thumbW, thumbH));
		}
		const char* sizePostfix = videoPref->GetString("videofilter.thumb.sizePostfix");
		if(sizePostfix) pThumbnail->SetSizePostfixFormat(sizePostfix);

		// Support another 4 different size output
		for(int i=1; i<=4; ++i) {
			char multiW[32] = {0};
			char multiH[32] = {0};
			sprintf(multiW, "videofilter.thumb.width%d", i);
			sprintf(multiH, "videofilter.thumb.height%d", i);
			int tempW = videoPref->GetInt(multiW);
			int tempH = videoPref->GetInt(multiH);
			if(dar.den > 0) {
				NormalizeResolution(tempW, tempH, dar.num, dar.den);
			}
			if(tempW > 0 && tempH > 0) {
				pThumbnail->AddThumbSize(Vector2i(tempW, tempH));
			}
		}
	}

	if(!pThumbnail->CalculateCapturePoint()) {
		delete pThumbnail;
		pThumbnail = NULL;
		return false;
	}

	return true;
}

bool parseThumbnailInfo1(CThumbnailFilter*& pThumbnail, CXMLPref* videoPref,
	video_info_t* pVinfo, std::string destFileName)
{
	bool enableThumb = videoPref->GetBoolean("videofilter.thumb1.enabled");
	if(!enableThumb) return true;
	const char* folderPath = videoPref->GetString("videofilter.thumb1.folder");
	StrPro::CCharset encodeConvert;
	std::string thumbFolder;
	char* ansiFolder = encodeConvert.UTF8toANSI(folderPath);
	if(ansiFolder) {
		thumbFolder = ansiFolder;
		free(ansiFolder);
	}

	const char* prefixName = videoPref->GetString("videofilter.thumb1.name");
	std::string strPrefix;
	char* ansiName = encodeConvert.UTF8toANSI(prefixName);
	if(ansiName) {
		strPrefix = ansiName;
		free(ansiName);
	}

	int thumbFormat = videoPref->GetInt("videofilter.thumb1.format");
	int thumbW = videoPref->GetInt("videofilter.thumb1.width");
	int thumbH = videoPref->GetInt("videofilter.thumb1.height");
	int startTime = videoPref->GetInt("videofilter.thumb1.start");
	int endTime = videoPref->GetInt("videofilter.thumb1.end");
	int thumbCount = videoPref->GetInt("videofilter.thumb1.count");
	bool bStitching = videoPref->GetBoolean("videofilter.thumb1.stitching");
	int thumbAlign = videoPref->GetInt("videofilter.thumb1.align");
	int imageQuality = videoPref->GetInt("videofilter.thumb1.quality");
	int cropMode = videoPref->GetInt("videofilter.thumb1.crop");
	bool enableMultiSize = videoPref->GetBoolean("videofilter.thumb1.multiSize");
	bool enablePack = videoPref->GetBoolean("videofilter.thumb1.pack");
	bool bOptimize = videoPref->GetBoolean("videofilter.thumb1.optimize");
	int thumbInterval = videoPref->GetInt("videofilter.thumb1.interval");
	int thumbRow = videoPref->GetInt("videofilter.thumb1.row");
	int thumbCol = videoPref->GetInt("videofilter.thumb1.col");
	pThumbnail = new CThumbnailFilter();

	if(thumbFolder.empty()) {
		size_t startIdx = destFileName.find_last_of('\\');
		if(startIdx == std::string::npos) {
			startIdx = destFileName.find_last_of('/');
		}
		thumbFolder = destFileName.substr(0, startIdx);
	}
	pThumbnail->SetFolder(thumbFolder.c_str());

	if(!strPrefix.empty()) {
		pThumbnail->SetPrefixName(strPrefix.c_str());
	} else {
		std::string fileTitle;
		StrPro::StrHelper::getFileTitle(destFileName.c_str(), fileTitle);
		pThumbnail->SetPrefixName(fileTitle.c_str());
	}

	const char* postfix = videoPref->GetString("videofilter.thumb1.postfix");
	if(postfix && *postfix) {
		pThumbnail->SetPostfixName(postfix);
	}

	fraction_t dar = pVinfo->dest_dar;
	if(dar.den > 0) {
		pThumbnail->SetVideoDar((float)dar.num/dar.den);
		NormalizeResolution(thumbW, thumbH, dar.num, dar.den);
	}

	pThumbnail->SetThumbnailSize(thumbW, thumbH);
	pThumbnail->SetImageFormat(thumbFormat);
	pThumbnail->SetYUVFrameSize(pVinfo->res_out.width, pVinfo->res_out.height);
	pThumbnail->SetFps((float)pVinfo->fps_out.num/pVinfo->fps_out.den);
	float vdur = pVinfo->duration/1000.f;
	if(vdur <= 0) {
		vdur = 3600.f*6;		// 6 hour
	} else if(vdur < 1) {
		vdur = 1;
	} else if(vdur > 3600*12) {
		vdur = 3600.f*12;		// 12 hour
	}

	if(vdur < 20) {
		startTime = 1;
	}
	pThumbnail->SetDuration((int)vdur);
	pThumbnail->SetStartTime(startTime);
	pThumbnail->SetEndTime(endTime);

	if(imageQuality > 0 && imageQuality < 100) {
		pThumbnail->SetImageQuality((unsigned int)imageQuality);
	}
	pThumbnail->SetCropMode(cropMode);
	if(thumbCount > 0) pThumbnail->SetThumbnailCount(thumbCount);
	if(bStitching) {
		pThumbnail->SetEanbleStitching(bStitching);
		pThumbnail->SetStitchAlign(thumbAlign);
	}
	if(thumbInterval > 0) {
		pThumbnail->SetThumbnailInterval(thumbInterval);
	}
	if(thumbRow > 0) {
		pThumbnail->SetStitchGrid(thumbRow, thumbCol);
	}
	// Pack image 
	pThumbnail->SetEnablePackImage(enablePack);
	pThumbnail->SetEnableOptimize(bOptimize);

	// Multi size output support
	if(enableMultiSize) {
		pThumbnail->SetEnableMultiSize(true);
		if(thumbW > 0 && thumbH > 0) {
			pThumbnail->AddThumbSize(Vector2i(thumbW, thumbH));
		}
		const char* sizePostfix = videoPref->GetString("videofilter.thumb1.sizePostfix");
		if(sizePostfix) pThumbnail->SetSizePostfixFormat(sizePostfix);

		// Support another 4 different size output
		for(int i=1; i<=4; ++i) {
			char multiW[32] = {0};
			char multiH[32] = {0};
			sprintf(multiW, "videofilter.thumb1.width%d", i);
			sprintf(multiH, "videofilter.thumb1.height%d", i);
			int tempW = videoPref->GetInt(multiW);
			int tempH = videoPref->GetInt(multiH);
			if(dar.den > 0) {
				NormalizeResolution(tempW, tempH, dar.num, dar.den);
			}
			if(tempW > 0 && tempH > 0) {
				pThumbnail->AddThumbSize(Vector2i(tempW, tempH));
			}
		}
	}

	if(!pThumbnail->CalculateCapturePoint()) {
		delete pThumbnail;
		pThumbnail = NULL;
		return false;
	}

	return true;
}

/*------------------------------------------------------------------------------
*  Convert an uint8_t buffer holding 8 or 16 bit PCM values with
*  channels interleaved to a int16_t buffer, still with channels interleaved
*----------------------------------------------------------------------------*/
void conv (  uint32_t        bitsPerSample,
			  uint8_t     * pcmBuffer,
			  uint32_t        lenPcmBuffer,
			  int16_t         * outBuffer,
			  bool                isBigEndian ) 
{
	if ( bitsPerSample == 8 ) {
		uint32_t    i, j;

		for ( i = 0, j = 0; i < lenPcmBuffer; ) {
			outBuffer[j] = pcmBuffer[i++];
			++j;
		}
	} else if ( bitsPerSample == 16 ) {

		if ( isBigEndian ) {
			uint32_t    i, j;

			for ( i = 0, j = 0; i < lenPcmBuffer; ) {
				uint16_t       value;

				value         = pcmBuffer[i++] << 8;
				value        |= pcmBuffer[i++];
				outBuffer[j]  = (int16_t)value;
				++j;
			}
		} else {
			uint32_t    i, j;

			for ( i = 0, j = 0; i < lenPcmBuffer; ) {
				uint16_t       value;

				value         = pcmBuffer[i++];
				value        |= pcmBuffer[i++] << 8;
				outBuffer[j]  = (int16_t)value;
				++j;
			}
		}
	} else {
		logger_err(LOGM_TS_AE, "This number of bits per sample not supported: %d", bitsPerSample);
	}
}


/*------------------------------------------------------------------------------
*  Convert a short buffer holding PCM values with channels interleaved
*  to one or more float buffers, one for each channel
*----------------------------------------------------------------------------*/
void conv (  int16_t         * shortBuffer,
			  uint32_t        lenShortBuffer,
			  float            ** floatBuffers,
			  uint32_t        channels )
{
	uint32_t    i, j;

	for ( i = 0, j = 0; i < lenShortBuffer; ) {
		for ( uint32_t c = 0; c < channels; ++c ) {
			floatBuffers[c][j] = ((float) shortBuffer[i++]) / 32768.f;
		}
		++j;
	}
}


/*------------------------------------------------------------------------------
*  Convert an uint8_t buffer holding 8 bit PCM values with channels
*  interleaved to two int16_t buffers (one for each channel)
*----------------------------------------------------------------------------*/
void conv8 (uint8_t     * pcmBuffer,
			   uint32_t        lenPcmBuffer,
			   int16_t         * leftBuffer,
			   int16_t         * rightBuffer,
			   uint32_t        channels )  
{
	if ( channels == 1 ) {
		for (uint32_t i = 0; i < lenPcmBuffer; ++i ) {
			leftBuffer[i] = (int16_t) pcmBuffer[i];
		}
	} else if ( channels == 2 ) {
		uint32_t    i, j;

		for ( i = 0, j = 0; i < lenPcmBuffer; ) {
			uint16_t  value;

			value          = pcmBuffer[i++];
			leftBuffer[j]  = (int16_t) value;
			value          = pcmBuffer[i++];
			rightBuffer[j] = (int16_t) value;
			++j;
		}
	} else {
		logger_err(LOGM_TS_AE, "This number of channels not supported: %d", channels);
	}
}


/*------------------------------------------------------------------------------
*  Convert an uint8_t buffer holding 16 bit PCM values with channels
*  interleaved to two int16_t buffers (one for each channel)
*----------------------------------------------------------------------------*/
void conv16 (    uint8_t     * pcmBuffer,
				uint32_t        lenPcmBuffer,
				int16_t         * leftBuffer,
				int16_t         * rightBuffer,
				uint32_t        channels,
				bool                isBigEndian )
{
	if ( isBigEndian ) {
		if ( channels == 1 ) {
			uint32_t    i, j;

			for ( i = 0, j = 0; i < lenPcmBuffer; ) {
				uint16_t   value;

				value           = pcmBuffer[i++] << 8;
				value          |= pcmBuffer[i++];
				leftBuffer[j]  = (int16_t) value;
				++j;
			}
		} else {
			uint32_t    i, j;

			for ( i = 0, j = 0; i < lenPcmBuffer; ) {
				uint16_t   value;

				value           = pcmBuffer[i++] << 8;
				value          |= pcmBuffer[i++];
				leftBuffer[j]   = (int16_t) value;
				value           = pcmBuffer[i++] << 8;
				value          |= pcmBuffer[i++];
				rightBuffer[j]  = (int16_t) value;
				++j;
			}
		}
	} else {
		if ( channels == 1 ) {
			uint32_t    i, j;

			for ( i = 0, j = 0; i < lenPcmBuffer; ) {
				leftBuffer[j++]   = *((int16_t*)(pcmBuffer+i));
				i+=2;
			}
		} else {
			uint32_t    i, j;
			for ( i = 0, j = 0; i < lenPcmBuffer; ) {
				leftBuffer[j]   = *((int16_t*)(pcmBuffer+i));
				i+=2;
				rightBuffer[j]  = *((int16_t*)(pcmBuffer+i));
				i+=2;
				++j;
			}
		}
	}
}

/*------------------------------------------------------------------------------
*  Convert an uint8_t buffer holding 16 bit PCM values with channels
*  interleaved to two float buffers (one for each channel)
*----------------------------------------------------------------------------*/
void conv16Float ( uint8_t     * pcmBuffer,
		uint32_t        lenPcmBuffer,
		double			* leftBuffer,
		double			* rightBuffer,
		uint32_t        channels,
		bool                isBigEndian)
{
	if ( isBigEndian ) {
		if ( channels == 1 ) {
			uint32_t    i, j;

			for ( i = 0, j = 0; i < lenPcmBuffer; ) {
				uint16_t   value;

				value           = pcmBuffer[i++] << 8;
				value          |= pcmBuffer[i++];
				leftBuffer[j]  = (double)value;
				++j;
			}
		} else {
			uint32_t    i, j;

			for ( i = 0, j = 0; i < lenPcmBuffer; ) {
				uint16_t   value;

				value           = pcmBuffer[i++] << 8;
				value          |= pcmBuffer[i++];
				leftBuffer[j]   = (double)value;
				value           = pcmBuffer[i++] << 8;
				value          |= pcmBuffer[i++];
				rightBuffer[j]  = (double)value;
				++j;
			}
		}
	} else {
		if ( channels == 1 ) {
			uint32_t    i, j;

			for ( i = 0, j = 0; i < lenPcmBuffer; ) {
				leftBuffer[j++]   = (double)(*((int16_t*)(pcmBuffer+i)));
				i+=2;
			}
		} else {
			uint32_t    i, j;
			for ( i = 0, j = 0; i < lenPcmBuffer; ) {
				leftBuffer[j]   = (double)(*((int16_t*)(pcmBuffer+i)));
				i+=2;
				rightBuffer[j]  = (double)(*((int16_t*)(pcmBuffer+i)));
				i+=2;
				++j;
			}
		}
	}
}

std::string genDestFileName(const char* fileTitle, CXMLPref* muxerPref, const char* outExt,
				const char* relativeDir, size_t destIndex, const char* srcDir)
{
	StrPro::CCharset set;
	std::string destFile;

	const char* outputUrl = muxerPref->GetString("overall.muxer.url");
	if(outputUrl && *outputUrl != '\0') {
		char* ansiUrl = set.UTF8toANSI(outputUrl);
		destFile = ansiUrl;
		free(ansiUrl);
		return destFile;
	}

	if(!fileTitle || !(*fileTitle)) {
		return "";
	}

	std::string leafName = fileTitle;
	const char* outDir = muxerPref->GetString("overall.task.destdir");
	char* ansiDir = set.UTF8toANSI(outDir);
	if(ansiDir) {
		if(!FolderExist(ansiDir)) {
			MakeDirRecursively(ansiDir);
		}
		destFile = ansiDir;
		free(ansiDir);
	} else {
		logger_warn(LOGM_TS_MUX, "Output file or output folder is not set, use source folder.\n");
		if(srcDir && *srcDir) {
			destFile = srcDir;
		} else {
			logger_err(LOGM_TS_MUX, "Get output dir from source file failed.\n");
			return "";
		}
	}

    if(*destFile.rbegin() != '\\' && *destFile.rbegin() != '/') {
		destFile += PATH_DELIMITER;
	}

	std::string ext;
	if(outExt) {
		ext = outExt;
		ext.insert(ext.begin(), 1, '.');
	} 

	time_t t = time(0);
	struct tm *btm = localtime(&t);
	if (muxerPref->GetBoolean("overall.output.formatting")) {
		const char* outputFormatString = muxerPref->GetString("overall.output.fileNameFormat");
		StrPro::CCharset encodeConvert;
		char* ansiFormatStr = encodeConvert.UTF8toANSI(outputFormatString);
		// TODO: substitue variable names in the formatting string with actual value and form the output path
		const char* p = ansiFormatStr;
		while (p && *p) {
			if (*p == '$' && *(p + 1) == '(') {		// Critical param
				p = p + 2;
				bool skipNextPathDelim = false;
				if (!strncmp(p, "Year", 4)) {
					char buf[16] = {0};
					sprintf(buf, "%d", btm->tm_year + 1900);
					destFile += buf;
				} else if (!strncmp(p, "Month", 5)) {
					char buf[16] = {0};
					sprintf(buf, "%d", btm->tm_mon + 1);
					destFile += buf;
				} else if (!strncmp(p, "Day", 3)) {
					char buf[16] = {0};
					sprintf(buf, "%d", btm->tm_mday);
					destFile += buf;
				} else if (!strncmp(p, "SourceFileName", 14)) {
					destFile += leafName;
				} else if (!strncmp(p, "DestExt", 7)) {
					if(outExt) destFile += outExt;
				} else if (!strncmp(p, "DestIndex", 9)) {
					char buf[3] = {0};
					sprintf(buf, "%d", destIndex);
					destFile += buf;
				} else if (!strncmp(p,"SourceRelativeDir", 17)) {
					const char* q = relativeDir;
					if(q && (*q)) {
						if(*q == PATH_DELIMITER) q++;
						destFile += q;
						MakeDirRecursively(destFile.c_str());
					} else {	// Relative is empty, skip next delimiter
						skipNextPathDelim = true;
					}
				} else if (!strncmp(p,"PresetName", 10)) {
					muxerPref->goRoot();
					const char* presetName = muxerPref->getAttribute("name");
					if(presetName && (*presetName)) {
						char* ansiName = encodeConvert.UTF8toANSI(presetName);
						if(ansiName && *ansiName) destFile += ansiName;
						free(ansiName);
					}
				}
				p = strchr(p, ')');
				if(skipNextPathDelim && (*(p+1) == PATH_DELIMITER)) {
					p++;
				}
			} else if (*p == '#' && *(p + 1) == 'T' && *(p + 2) == 'r' &&
			 *(p + 3) == 'i' && *(p + 4) == 'm' && *(p + 5) == '(') {	// Function param
				 p = p + 6;
				 if (!strncmp(p, "Space", 5)) {
					for (std::string::iterator it=leafName.begin(); it!=leafName.end();) {
						if(*it == ' ') {
							it = leafName.erase(it);
						} else {
							++it;
						}
					}
				}
				p = strchr(p, ')');
			} else {
				if(*p == '\\' || *p == '/') {
					_mkdir(destFile.c_str());		// Ensure dir exists
				}
				destFile += *p;
			}
			p++;
		}
		destFile += ext;
		free(ansiFormatStr);
	} else {
		destFile += leafName + ext;
	}

	// Process duplicate file name
// 	if(muxerPref->GetInt("overall.task.overwrite") != 1 && !destFile.empty() && FileExist(destFile.c_str())) {		// Rename 
// 		size_t dotIdx = destFile.find_last_of('.');
// 		if(dotIdx != std::string::npos) {
// 			destFile = destFile.substr(0, dotIdx);
// 		}
// 		char curTime[20] = {0};
// 		sprintf(curTime, "_%d_%d_%d_%d", btm->tm_mday, btm->tm_hour, btm->tm_min, btm->tm_sec);
// 		destFile += curTime;
// 		destFile += ext;
// 	}
	return destFile;
}
