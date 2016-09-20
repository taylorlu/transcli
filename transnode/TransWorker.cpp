#ifdef WIN32
#include <io.h>
#endif
#include <stdio.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>

#include "MEvaluater.h"
#include "TransWorker.h"

#include "Decoder.h"
#include "VideoEncode.h"

#include "util.h"
#include "yuvUtil.h"
#include "StrPro/StrHelper.h"
#include "logger.h"
#include "WorkManager.h"
#include "PlaylistGenerator.h"
#include "pptv_level.h"
#define cimg_use_jpeg
#define cimg_use_png
#define cimg_display 0
#include "cimg/CImg.h"

#define SAFESTRING(str) ((str)?(str):("")) 

#ifndef _WIN32
#include <sched.h>
#include <unistd.h>
#endif

CTransWorker::CTransWorker(): m_frameBufSize(0), m_pcmBufSize(48000), m_yuvBuf(NULL), m_pcmBuf(NULL),
							m_srcVideoAttrib(NULL), m_bEnableDecoderVf(false), 
							m_bEnableDecoderAf(false), m_id(0), m_uuid(0),
							m_hMainThread(NULL),m_hAudioThread(NULL), m_hVideoThread(NULL),
							m_pRootPref(NULL), m_errCode(EC_NO_ERROR),
							m_startTime(0), m_endTime(0), m_lastTime(0), m_elapseSecs(0), 
							m_taskState(TASK_READY), m_savedState(TASK_NOTSET),m_progress(0.f), 
							m_encodedFrames(0),  m_bUseCudaEncoder(false),
							m_bLosslessClip(false),
							m_logType(LOGM_TS_WK), audioClipIndex(0), videoClipIndex(0),
							m_audioClipEnd(0),m_videoClipEnd(0),
							m_audioFractionOfSecond(5), m_encAudioBytes(0),m_bAutoVolumeGain(false),
							m_fVolumeNormalDB(89.f), m_bInsertBlankAudio(false), m_bInsertBlankVideo(false)
{
#ifdef DEMO_RELEASE
	m_pWaterMarkMan = NULL; 
#endif
	m_pPlaylist = NULL;

	m_tmpBenchData.lastFrameNum = 0;
	m_tmpBenchData.mainDur  = -100;
	m_tmpBenchData.fpsDen   = 0;
	m_tmpBenchData.fpsNum   = 0;
	m_tmpBenchData.audioEncTime = 0.f;
	m_tmpBenchData.videoEncTime = 0.f;
	
#ifdef DEBUG_DUMP_RAWVIDEO
	m_rawvideo_output = getStreamOutput(CStreamOutput::FILE_STREAM);
#endif

#ifdef DEBUG_DUMP_RAWAUDIO
	m_rawaudio_output = getStreamOutput(CStreamOutput::FILE_STREAM);
#endif
	m_lastTime = m_startTime = GetTickCount(); // will be resetted later
	m_pFinishCbForCli = NULL;
}

bool CTransWorker::setSrcFile(const char* inFile)
{
	if( inFile && (strstr(inFile, "://") || FileExist(inFile)) ) {
		m_streamFiles.AddSrcFile(inFile);
		return true;
	}
	
	logger_err(m_logType, "Input file %s is not existed\n", inFile);
	SetErrorCode(EC_INVALID_MEDIA_FILE);
	return false;
}


CTransWorker::~CTransWorker(void)
{
	CleanUp();
}

bool CTransWorker::parseBasicInfo()
{
	bool ret = true;
#define FAIL_INFO(err) logger_err(m_logType, err); ret=false; break;
	do {
	    if(!m_pRootPref) {
			FAIL_INFO("Root preference hasn't been set.\n");
		}
		
		const char* tempdir = m_pRootPref->GetTempDir();
		const char* subtitleFile = m_pRootPref->GetSubTitle();
		
		std::map<std::string, StrPro::CXML2*>& srcMedias = m_pRootPref->GetMediaInfoNodes();
		std::map<std::string, StrPro::CXML2*>::const_iterator it;
		for(it=srcMedias.begin(); it != srcMedias.end(); ++it) {
			if(!setSrcFile(it->first.c_str())) {
				FAIL_INFO("Bad source file.\n");
			}
		}
	
        m_streamFiles.SetWorkerId(GetId());
		if(!m_streamFiles.SetTempDir(tempdir)) {
			SetErrorCode(EC_DIR_IO_ERROR);
			FAIL_INFO("Set temp folder failed.\n");
		}
		if(subtitleFile && *subtitleFile != '\0') m_streamFiles.SetSubTitleFile(subtitleFile);
	} while(false);
	#undef  FAIL_INFO
	return ret;
}

bool CTransWorker::errIgnored(int errCode)
{
	for (size_t i=0; i<m_ignoreErrCodes.size(); ++i) {
		if(m_ignoreErrCodes[i] == errCode) return true;
	}
	return false;
}

bool CTransWorker::parseDurationInfo(CXMLPref* pTaskPref, StrPro::CXML2* pMediaPref)
{
	int decodeStart = 0, decodeDur = 0;
	decodeStart = pTaskPref->GetInt("overall.decoding.startTime");
	if(decodeStart < 0) decodeStart = 0;
	decodeDur = pTaskPref->GetInt("overall.decoding.duration");
		
	int totalDur = 0;
	if (pMediaPref) {
		pMediaPref->goRoot();
		if(!pMediaPref->findChildNode("general")) {
			if(!pMediaPref->findChildNode("video")) {
				pMediaPref->findChildNode("audio");
			}
		}
		totalDur = pMediaPref->getChildNodeValueInt("duration");
		if(totalDur <= 0) {
			totalDur = INT_MAX;
		}
		if(totalDur < 1000 && !errIgnored(EC_DUR_LESS_THAN_ONE_SEC)) {
			SetErrorCode(EC_DUR_LESS_THAN_ONE_SEC);
			logger_err(m_logType, "Media duration is less than 1 seconds.\n");
			return false;
		}
	}

	if(totalDur > 0 && totalDur <= decodeStart ||
		(!m_clipStartSet.empty() && totalDur <= m_clipStartSet.front()) ) {
		SetErrorCode(EC_INVALID_CLIP_PARAM);
		logger_err(m_logType, "Wrong clip param, start time is bigger than media file duration.\n");
		return false;
	}

	if(decodeDur > 0) {
		m_tmpBenchData.mainDur = decodeDur;
	} else {
		m_tmpBenchData.mainDur = totalDur - decodeStart;	// decodeDur = 0, but decodeStart > 0
	}

	if(totalDur > 0 && decodeDur > 0 && (totalDur - decodeStart < decodeDur)) {
		m_tmpBenchData.mainDur = totalDur - decodeStart;
	}

	if(m_tmpBenchData.mainDur > 5*60*60*1000) {		// 5 hours
		m_tmpBenchData.mainDur = INT_MAX;
	}
	// Adjust duration according to clips
	/*if(pTaskPref->GetBoolean("overall.task.decoderTrim")) {
		int curDur = 0;
		for(size_t i=0; i<m_clipEndSet.size(); ++i) {
			curDur += m_clipEndSet[i] - m_clipStartSet[i];
		}
		if(curDur > m_tmpBenchData.mainDur) {
			curDur = m_tmpBenchData.mainDur;
		}
		m_tmpBenchData.mainDur = curDur;
	} else {*/

	if(!m_clipEndSet.empty() && m_clipEndSet.back() < m_tmpBenchData.mainDur) {
		m_tmpBenchData.mainDur = m_clipEndSet.back();
		pTaskPref->SetInt("overall.decoding.duration", m_tmpBenchData.mainDur);
	}
	
	//if(m_tmpBenchData.mainDur < 5000) {		// If duration is too short(<5s), don't insert blank audio track
	//	pTaskPref->SetBoolean("overall.audio.insertBlank", false);
	//}
	return true;
}

void CTransWorker::checkAudioParam(audio_info_t* ainfo, wav_info_t* pWav)
{
	if(!ainfo || !pWav) return;

	if(ainfo->bits == 0 && pWav->bits>0 && pWav->bits<32) {
		ainfo->bits = pWav->bits;
	}
	
	ainfo->in_channels = pWav->channels;
	ainfo->in_srate = pWav->sample_rate;
	
	if(ainfo->out_channels <= 0) {
		ainfo->out_channels = pWav->channels;
	} else {	// Decoder do audio filter
		ainfo->in_channels = ainfo->out_channels;
	}
	if(ainfo->out_srate <= 0) {
		ainfo->out_srate = pWav->sample_rate;
	} else if(m_bEnableDecoderAf){	// Decoder do audio filter
		ainfo->in_srate = ainfo->out_srate;
	}

	ainfo->in_bytes_per_sec = pWav->bytes_per_second;
}

void CTransWorker::checkVideoParam(video_info_t* vinfo, yuv_info_t* pYuv)
{
	if(!vinfo || !pYuv) return;
	
	int fps_num = vinfo->fps_in.num;
	int fps_den = vinfo->fps_in.den;
	if(fps_den == 0 || (float)fps_num/fps_den > 120) {
		if(m_srcVideoAttrib) {
			fps_num = m_srcVideoAttrib->fps_num;
			fps_den = m_srcVideoAttrib->fps_den;
		} else {
			fps_num = 25;
			fps_den = 1;
		}
		vinfo->fps_in.num = fps_num;
		vinfo->fps_in.den = fps_den;
	}

	// Correct original video size (Sometimes width and height are reversed)
	//if(vinfo->res_in.height > vinfo->res_in.width && fps_num > fps_den) {
	//	std::swap(vinfo->res_in.height, vinfo->res_in.width);
	//}

	if(vinfo->src_dar.num <= 0 || vinfo->src_dar.den <= 0) {
		CAspectRatio aspectConvert(vinfo->res_in.width, vinfo->res_in.height);
		aspectConvert.SetPAR(pYuv->pixelaspect_num, pYuv->pixelaspect_den);
		aspectConvert.GetDAR(&(vinfo->src_dar.num), &(vinfo->src_dar.den));
	}
	
	if(vinfo->res_out.width <= 0) {
		vinfo->res_out.width = vinfo->res_in.width;
	} 
	if(vinfo->res_out.height <= 0) {
		vinfo->res_out.height = vinfo->res_in.height;
	} 
	if(vinfo->fps_out.num <= 0) {
		vinfo->fps_out.num = vinfo->fps_in.num;
	} 
	if(vinfo->fps_out.den <= 0) {
		vinfo->fps_out.den = vinfo->fps_in.den;
	} 

	if(vinfo->dest_dar.num <= 0 || vinfo->dest_dar.den <= 0) {	// If dar is not set
		if(vinfo->dest_par.num > 0) {	// Par is set
			CAspectRatio aspectConvert(vinfo->res_out.width, vinfo->res_out.height);
			aspectConvert.SetPAR(vinfo->dest_par.num, vinfo->dest_par.den);
			aspectConvert.GetDAR(&(vinfo->dest_dar.num), &(vinfo->dest_dar.den));
		} else {
			vinfo->dest_dar.num = vinfo->src_dar.num;
			vinfo->dest_dar.den = vinfo->src_dar.den;
		}
	} 

	if(vinfo->dest_par.num <= 0 || vinfo->dest_par.den <= 0) {	// If par is not set
		CAspectRatio aspectConvert(vinfo->res_out.width, vinfo->res_out.height);
		aspectConvert.SetDAR(vinfo->dest_dar.num, vinfo->dest_dar.den);
		aspectConvert.GetPAR(&(vinfo->dest_par.num), &(vinfo->dest_par.den));
	}
}

int CTransWorker::EndWork()
{
	int ret = 0;
	logger_status(m_logType, "worker %d enter endWork()\n", m_id);
	//logger_status(m_logType, "endWork() return %d\n", ret);
	return ret;
}

void CTransWorker::parseMediaAudioInfoNode(StrPro::CXML2* mediaInfo, attr_audio_t* pAudioAttrib)
{
	if(!pAudioAttrib) return;
	memset(pAudioAttrib->codec, 0, CODEC_NAME_LEN);
	memset(pAudioAttrib->profile, 0, CODEC_NAME_LEN);
	memset(pAudioAttrib->lang, 0, 32);
	strncpy(pAudioAttrib->codec, SAFESTRING(mediaInfo->getChildNodeValue("codec")), CODEC_NAME_LEN);
	strncpy(pAudioAttrib->profile, SAFESTRING(mediaInfo->getChildNodeValue("profile")), CODEC_NAME_LEN);
	strncpy(pAudioAttrib->lang, SAFESTRING(mediaInfo->getChildNodeValue("lang")), 32);
	strncpy(pAudioAttrib->title, SAFESTRING(mediaInfo->getChildNodeValue("title")), CODEC_NAME_LEN);

	int aId = mediaInfo->getChildNodeValueInt("id");
	if (aId >= 192 && aId <= 199) aId -= 192;
	int aIndex = mediaInfo->getChildNodeValueInt("index");
	if(aIndex > 0) pAudioAttrib->index = aIndex;
	int aDuration = mediaInfo->getChildNodeValueInt("duration");
	if(aDuration > 0) {
		pAudioAttrib->duration = aDuration;
	}
	int aBitrate = mediaInfo->getChildNodeValueInt("bitrate");
	if(aBitrate > 0) {
		pAudioAttrib->bitrate = aBitrate;
	}
	int aRcMode = mediaInfo->getChildNodeValueInt("bitratemode");
	if(aRcMode >= 0) {
		pAudioAttrib->bitratemode = aRcMode;
	}
	int aSampleRate = mediaInfo->getChildNodeValueInt("samplerate");
	int channels = mediaInfo->getChildNodeValueInt("channels");
	if(aId>=0 || aDuration>0 || aBitrate>0 || aSampleRate>0 || channels>0) {
		if(aId >= 0) pAudioAttrib->id = aId;
		if(aDuration > 0) pAudioAttrib->duration = aDuration;
		if(aBitrate > 0) pAudioAttrib->bitrate = aBitrate;
		if(aSampleRate > 0) pAudioAttrib->samplerate = aSampleRate;
		if(channels > 0) pAudioAttrib->channels = channels;
		int samplebits = mediaInfo->getChildNodeValueInt("samplebits");	
		if(samplebits > 0) pAudioAttrib->samplebits = samplebits;
		pAudioAttrib->blockalign = mediaInfo->getChildNodeValueInt("blockalign");	
		pAudioAttrib->samples = mediaInfo->getChildNodeValueInt("samples");	
		int audioDelay = mediaInfo->getChildNodeValueInt("delay");
		pAudioAttrib->delay = audioDelay;
	} 
}

void CTransWorker::parseMediaVideoInfoNode(StrPro::CXML2* mediaInfo, attr_video_t* pVideoAttrib)
{
	if(!pVideoAttrib) return;
	//int vId = -1;
	//vId = mediaInfo->getChildNodeValueInt("id");
	int vDuration = mediaInfo->getChildNodeValueInt("duration");
	int width = mediaInfo->getChildNodeValueInt("width");
	int height = mediaInfo->getChildNodeValueInt("height");

	if(vDuration > 0 || width > 0) {
		//pVideoAttrib->id = vId;
		memset(pVideoAttrib->codec, 0, CODEC_NAME_LEN);
		memset(pVideoAttrib->profile, 0, CODEC_NAME_LEN);
		memset(pVideoAttrib->version, 0, CODEC_NAME_LEN);
		memset(pVideoAttrib->standard, 0, 32);
		strncpy(pVideoAttrib->codec, SAFESTRING(mediaInfo->getChildNodeValue("codec")), CODEC_NAME_LEN);
		strncpy(pVideoAttrib->profile, SAFESTRING(mediaInfo->getChildNodeValue("profile")), CODEC_NAME_LEN);
		strncpy(pVideoAttrib->version, SAFESTRING(mediaInfo->getChildNodeValue("version")), CODEC_NAME_LEN);
		strncpy(pVideoAttrib->standard, SAFESTRING(mediaInfo->getChildNodeValue("standard")), 32);
		strncpy(pVideoAttrib->title, SAFESTRING(mediaInfo->getChildNodeValue("title")), CODEC_NAME_LEN);

		pVideoAttrib->id = mediaInfo->getChildNodeValueInt("index");
		pVideoAttrib->duration = vDuration;
		int vBitrate = mediaInfo->getChildNodeValueInt("bitrate");
		if(vBitrate > 0) pVideoAttrib->bitrate = vBitrate;
		if(width > 0) pVideoAttrib->width = width;
		if(height > 0) pVideoAttrib->height = height;
		int fps_num = mediaInfo->getChildNodeValueInt("fps_num");
		if(fps_num > 0) pVideoAttrib->fps_num = fps_num;
		int fps_den = mediaInfo->getChildNodeValueInt("fps_den");
		if(fps_den > 0) pVideoAttrib->fps_den = fps_den;
		int dar_num = mediaInfo->getChildNodeValueInt("dar_num");
		int dar_den = mediaInfo->getChildNodeValueInt("dar_den");


		if(dar_den <= 0 || dar_num <= 0) {
			CAspectRatio aspectConvert(width, height);
			aspectConvert.GetDAR(&dar_num, &dar_den);
		}

		double ddar = (dar_num * 1.0) / (dar_den * 1.0);

		if (ddar < 0.2)
		{
			CAspectRatio aspectConvert(width, height);
			aspectConvert.GetDAR(&dar_num, &dar_den);
		}

		if(dar_num > 0) pVideoAttrib->dar_num = dar_num;
		if(dar_den > 0) pVideoAttrib->dar_den = dar_den;
		float originFps = mediaInfo->getChildNodeValueFloat("origin_fps");
		if(originFps > 0.1f) {
			pVideoAttrib->originFps = originFps;
		}
		const char* hasOriginRes = mediaInfo->getChildNodeValue("origin_res");
		if(hasOriginRes && strcmp(hasOriginRes, "true") == 0) {
			pVideoAttrib->has_origin_res = 1;
		}
		int interlaceType = mediaInfo->getChildNodeValueInt("interlaced");
		if(interlaceType >= 0) pVideoAttrib->interlaced = interlaceType;
		int isVfr = mediaInfo->getChildNodeValueInt("is_vfr");
		if(isVfr >= 0) pVideoAttrib->is_vfr = isVfr;
		pVideoAttrib->rotate = mediaInfo->getChildNodeValueInt("rotate");
		pVideoAttrib->is_video_passthrough = mediaInfo->getChildNodeValueInt("passthrough");
		//if(isVfr) {		// If it's vfr, then don't pass through
		//	pVideoAttrib->is_video_passthrough = 0;
		//}
	}
}

bool CTransWorker::setAudioEncAttrib(audio_info_t* pAInfo, CXMLPref* audioPref, attr_audio_t* pAudioAttrib)
{
	pAInfo->in_srate = pAudioAttrib->samplerate;
	pAInfo->in_channels = pAudioAttrib->channels;
	pAInfo->aid = pAudioAttrib->id;
	pAInfo->index = pAudioAttrib->index;
	pAInfo->duration = pAudioAttrib->duration;
	memset(pAInfo->lang, 0, 32);
	if(*(pAudioAttrib->lang)) {
		strncpy(pAInfo->lang, pAudioAttrib->lang, 32);
	}
	memset(pAInfo->title, 0, CODEC_NAME_LEN);
	if(*(pAudioAttrib->title)) {
		strncpy(pAInfo->title, pAudioAttrib->title, 32);
	}
	// If encoding E-AC3 audio, if source channel < 6, then report error
	if(audioPref->GetBoolean("overall.dolby.sixchOnly")) {
		if(pAInfo->format == AC_EAC3 && pAInfo->in_channels < 6) {
			logger_err(m_logType, "Source audio channels is less than 6, invalid for DD+ encoding.\n");
			return false;
		}
	}

	if(pAInfo->out_channels == 0) {
		pAInfo->out_channels = pAInfo->in_channels/*<=2? pAInfo->in_channels:2*/;
		if(pAInfo->format != AC_AC3 && pAInfo->format != AC_AAC_LC && pAInfo->format != AC_PCM
			&& pAInfo->format != AC_AAC_HE && pAInfo->format != AC_AAC_HEV2 && pAInfo->format != AC_EAC3) {
				if(pAInfo->out_channels > 2) pAInfo->out_channels = 2;
		}
		if(pAInfo->out_channels > 6) pAInfo->out_channels = 6;
	} else if(pAInfo->out_channels > pAInfo->in_channels) {	// If output ch is bigger(no sense)
		if(audioPref->GetBoolean("audiofilter.channels.changeDownOnly")) {
			pAInfo->out_channels = pAInfo->in_channels;
		}
	}

	// E-AC3 and AAC-HEV2 only support 2 more channels
	if((pAInfo->format == AC_AAC_HEV2 || pAInfo->format == AC_EAC3) && pAInfo->out_channels == 1) {
		pAInfo->out_channels = 2;
	} 

	// When output channels > 2, disable replay gain
	//if(pAInfo->out_channels > 2) {
	//	m_bAutoVolumeGain = false;	// Downmix to 2 channel to anlynize volume
	//}

	if(pAInfo->out_srate > pAInfo->in_srate &&
		audioPref->GetBoolean("audiofilter.resample.downSamplingOnly") ) {
		pAInfo->out_srate = pAInfo->in_srate;
		if(pAInfo->format == AC_AAC_HE || pAInfo->format == AC_AAC_HEV2) {
			if(pAInfo->in_srate < 22050) {	// NeroAac he-aac not support samplerate < 22050
				pAInfo->out_srate = 22050;
			}
		}
	}

	// Adjust bitrate according to original bitrate
	if(audioPref->GetBoolean("audiofilter.extra.brdown")) {
		const char* brField = "audioenc.ffmpeg.bitrate";
		if(pAInfo->format == AC_AAC_LC) {
			//brField = "audioenc.faac.bitrate";
			brField = "audioenc.fdkaac.bitrate";
		} else if(pAInfo->format == AC_AAC_HE || pAInfo->format == AC_AAC_HEV2) {
			//brField = "audioenc.nero.bitrate";
			brField = "audioenc.fdkaac.bitrate";
		} else if(pAInfo->format == AC_MP3) {
			brField = "audioenc.lame.bitrate";
		}
		int brSetting = audioPref->GetInt(brField);
		int brSource = pAudioAttrib->bitrate/1000;
		
		if(pAInfo->format == AC_MP2) {	// Bitrate of mp2 must multiple of 16
			EnsureMultipleOfDivisor(brSource, 16);
		}

		// If brSource < 8k, may be it's wrongly detected by ffprobe
		if(brSource > 8 && brSetting > 0 && brSource < brSetting) {
			brSetting = brSource;
		}

		// Normalize AAC bitrate and channels
		int presetLevel = audioPref->GetInt("overall.task.ppLevel");
        if(pAInfo->out_channels > 2) {  // 5.1 output
			if (presetLevel == PPTV_LEVEL_BD) {
                brSetting = 288;
            } else {
                pAInfo->out_channels = 2;
            }
		}

		if(pAInfo->out_channels <= 0) {
			logger_err(m_logType, "Audio output channels is 0, maybe input audio is invalid.\n");
			return false;
		}
		// If bitrate of single channel is too high, then it will fail.
		if(brSetting / pAInfo->out_channels > 96) {
			brSetting = pAInfo->out_channels * 96;
		}

		audioPref->SetInt(brField, brSetting);
	}

	// Source audio attrib exist, so disable insert blank audio track
	if(pAudioAttrib->id >= 0) {	// Original audio track, not appended blank audio track.
		audioPref->SetBoolean("overall.audio.insertBlank", false);
	}
	
	return true;
	// Parse source audio format
	//if(pAudioAttrib->codec && *(pAudioAttrib->codec)) {	// && pAudioAttrib->channels == 6
	//	if(!_stricmp(pAudioAttrib->codec, "DTS")) {
	//		pAInfo->srcformat = AC_DTS;
	//	} else if(!_stricmp(pAudioAttrib->codec, "AC-3")) {
	//		pAInfo->srcformat = AC_AC3;
	//	} else if(strstr(m_srcAudioAttrib->codec, "MPEG Audio")) {
	//		if(strstr(m_srcAudioAttrib->profile, "Layer 3")) pAInfo->srcformat = AC_MP3;
	//		else if(strstr(m_srcAudioAttrib->profile, "Layer 2")) pAInfo->srcformat = AC_MP2;
	//	}
	//}

//	if(pAudioAttrib->delay != 0) {
// 		audioPref->SetBoolean("audiofilter.delay.enabled", true);
// 		for (int i=0; i<pAInfo->out_channels; ++i) {
// 			char buf[32] = {0};
// 			sprintf(buf, "audiofilter.delay.channel%d", i + 1);
// 			audioPref->SetInt(buf, pAudioAttrib->delay);
// 		}
//		audioPref->SetFloat("overall.audio.delay1", (float)pAudioAttrib->delay);
//		pAInfo->delay = pAudioAttrib->delay;
//	}

	// If input audio channel is 5.1 and out is also 5.1, reorder channel layout
	// Mplayer output PCM 5.1 layout is different from Wav format
	/*if(pAInfo->in_channels == 6 && pAInfo->out_channels == 6) {
		audioPref->SetBoolean("audiofilter.channels.enabled", true);
		audioPref->SetInt("audiofilter.channels.output", 6);
		audioPref->SetInt("audiofilter.channels.routes", 6);
		int wavLayout[6] = {1, 2, 5, 6, 3, 4};
		for (int i=0; i<6; ++i) {
			char strKey[64] = {0};
			sprintf(strKey, "audiofilter.channels.channel%d", i+1);
			audioPref->SetInt(strKey, wavLayout[i]);
		}
	}*/
}

bool CTransWorker::setVideoEncAttrib(video_info_t* pVInfo,CXMLPref* videoPref,attr_video_t* pVideoAttrib)
{
	memset(pVInfo->title, 0, CODEC_NAME_LEN);
	if(*(pVideoAttrib->title)) {
		strncpy(pVInfo->title, pVideoAttrib->title, 32);
	}
	pVInfo->src_dar.num = pVideoAttrib->dar_num;
	pVInfo->src_dar.den = pVideoAttrib->dar_den;
	pVInfo->fps_in.num = pVideoAttrib->fps_num;
	pVInfo->fps_in.den = pVideoAttrib->fps_den;
	pVInfo->res_in.width = pVideoAttrib->width;
	pVInfo->res_in.height = pVideoAttrib->height;
	pVInfo->interlaced = pVideoAttrib->interlaced;
	pVInfo->duration = pVideoAttrib->duration;
	pVInfo->is_vfr = pVideoAttrib->is_vfr;
	pVInfo->index = pVideoAttrib->id;
	pVInfo->rotate = pVideoAttrib->rotate;
	pVInfo->duration = m_tmpBenchData.mainDur;
	pVInfo->is_video_passthrough = pVideoAttrib->is_video_passthrough;
	if(pVInfo->fps_in.num <= 0 || pVInfo->fps_in.den <= 0) {
		pVInfo->fps_in.num = 24;
		pVInfo->fps_in.den = 1;
	}

	if(pVInfo->fps_out.num <= 0 || pVInfo->fps_out.den <= 0) {
		pVInfo->fps_out.num = pVInfo->fps_in.num;
		pVInfo->fps_out.den = pVInfo->fps_in.den;
	}

	// Temp workaround for bad vf fps processing (if original fps is smaller)
	if((float)pVInfo->fps_out.num/pVInfo->fps_out.den > (float)pVInfo->fps_in.num/pVInfo->fps_in.den) {
		pVInfo->fps_out.num = pVInfo->fps_in.num;
		pVInfo->fps_out.den = pVInfo->fps_in.den;
	}

	// Parse source file container format
	if(*(pVideoAttrib->fileFormat)) {
		if(_stricmp(m_srcVideoAttrib->fileFormat, "mpegts") == 0) {
			pVInfo->src_container = CF_MPEG2TS;
		} else if(_stricmp(m_srcVideoAttrib->fileFormat, "mpeg") == 0) {
			pVInfo->src_container = CF_MPEG2;
		} else if(_stricmp(m_srcVideoAttrib->fileFormat, "asf") == 0) {
			pVInfo->src_container = CF_ASF;
		} else if(_stricmp(m_srcVideoAttrib->fileFormat, "matroska,webm") == 0) {
			pVInfo->src_container = CF_MKV;
		} else if(_stricmp(m_srcVideoAttrib->fileFormat, "avi") == 0) {
			pVInfo->src_container = CF_AVI;
		} else if(_stricmp(m_srcVideoAttrib->fileFormat, "mov,mp4,m4a,3gp,3g2,mj2") == 0) {
			pVInfo->src_container = CF_MP4;
		}
	}

	return true;
	//if(pVideoAttrib->is_vfr) {	// Disable sync correction
	//	videoPref->SetBoolean("videofilter.extra.syncCorrection", false);
	//}
}

void CTransWorker::performBlackBandAutoDetect(const char* srcFile, CVideoEncoder* pVideoEnc)
{
	if(!srcFile || !pVideoEnc) return;
	video_info_t* pvInfo = pVideoEnc->GetVideoInfo();
	CXMLPref* pPref = pVideoEnc->GetVideoPref();
	if(!pvInfo || !pPref) return;
	if(pPref->GetInt("videofilter.crop.mode") != 4) {	// If not auto-detect mode
		return;
	}

	int dur = m_tmpBenchData.mainDur;
	// Auto detect crop area
	int detectThreshold = pPref->GetInt("videofilter.crop.detectLimit");
	int rightFix = pPref->GetInt("videofilter.crop.rightFix");
	int bottomFix = pPref->GetInt("videofilter.crop.bottomFix");
	CYuvUtil::FrameRect rect = CYuvUtil::AutoDetectCrop(srcFile, dur, detectThreshold);
	if(rightFix >= -10 && rightFix <= 10) {
		rect.w += rightFix;
	}
	if(bottomFix >= -10 && bottomFix <= 10) {
		rect.h += bottomFix;
	}
	pPref->SetInt("videofilter.crop.left", rect.x);
	pPref->SetInt("videofilter.crop.top", rect.y);
	pPref->SetInt("videofilter.crop.width", rect.w);
	pPref->SetInt("videofilter.crop.height", rect.h);
}

void CTransWorker::adjustVideoOutParam(CVideoEncoder* pVideoEnc, int overallBr)
{
	if(!pVideoEnc) return;
	video_info_t* pvInfo = pVideoEnc->GetVideoInfo();
	CXMLPref* pPref = pVideoEnc->GetVideoPref();
	int dividor = 2;

	if(pvInfo && pPref) {
		// Adjust bitrate according to original bitrate
		int brSource = (int)(m_srcVideoAttrib->bitrate/1000.f + 0.5f);
		if(m_srcVideoAttrib && pPref->GetBoolean("videofilter.extra.brdown") && overallBr > 0) {
			int brSetting = pPref->GetInt("overall.video.bitrate");
			if(brSource > 0 && brSource < brSetting && brSource > overallBr*0.6f) {
				pPref->SetInt("overall.video.bitrate", brSource);
			}
		}

		resolution_t* resOut = &pvInfo->res_out;
		fraction_t dar = pvInfo->dest_dar;
		if(dar.den == 0 || dar.num == 0) {
			dar = pvInfo->src_dar;
		}
		// Correct original video size (Sometimes width and height are reversed)
		//if(pvInfo->res_in.height > pvInfo->res_in.width && dar.num > dar.den) {
		//	std::swap(pvInfo->res_in.height, pvInfo->res_in.width);
		//}

		if(dar.den != 0 && dar.num != 0) {
			// 4k resolution adjust, when dar is 16:9 and width is 4096, video is not compatible with sansumg TV
			if(resOut->width > 3000) {	// 4k resolution
				if(FloatEqual(dar.num*1.f/dar.den, 1.7778f, 0.1f)) {
					resOut->width = 3840;
				} else {
					resOut->width = 4096;
				}
			}

			NormalizeResolution(resOut->width, resOut->height, dar.num, dar.den, dividor);
		} 

		// target video size is no bigger than original size
		int inWidth = pvInfo->res_in.width;
		if(pPref->GetBoolean("videofilter.scale.scaleDown")) { // Only scale down
			fraction_t inDar = pvInfo->src_dar;
			if(inDar.den > 0) {
				int inCalcWidth = (int)(pvInfo->res_in.height*inDar.num/inDar.den);
				if(inCalcWidth > inWidth) inWidth = inCalcWidth;
			}

			if(inWidth > 0 && pvInfo->res_out.width > inWidth) {
				// Judge if src w/h is equal to dest dar (if not square pixel, logo will distort)
				float destDar = -1.f;
				if(dar.den > 0) {
					destDar = dar.num/(float)dar.den;
					pvInfo->res_out.width = inWidth;
					pvInfo->res_out.height = (int)(pvInfo->res_out.width/destDar);
				} else {
					pvInfo->res_out.width = inWidth;
					pvInfo->res_out.height = pvInfo->res_in.height;
				} 
				
				EnsureMultipleOfDivisor(pvInfo->res_out.width, dividor);
				EnsureMultipleOfDivisor(pvInfo->res_out.height, dividor);
			}
		}

		// If original video highest level is HD/SD, and source bitrate is low
		int presetLevel = pPref->GetInt("overall.task.ppLevel");
		if(presetLevel == PPTV_LEVEL_HD) {
			if(inWidth >= 600 && inWidth < 1000) {  // Highest level is HD
				if(brSource > 0 && brSource < ppl_def[PPTV_LEVEL_HD].vbr) {
					pPref->SetInt("overall.video.bitrate", brSource);
					pPref->SetInt("videoenc.x264.targetQp", 0);		// Shut down bitrate lower
				}
			}
		} else if(presetLevel == PPTV_LEVEL_SD) {
			if(inWidth < 600) {  // Highest level is SD
				if(brSource > 0 && brSource < ppl_def[PPTV_LEVEL_SD].vbr) {
					pPref->SetInt("overall.video.bitrate", brSource);
					pPref->SetInt("videoenc.x264.targetQp", 0);		// Shut down bitrate lower
				}
			}
		}
		// Won't lower bitrate if original bitrate is below 2M
		//if(inWidth > 1400
		//if(brSource < 2048) {  // source bitrate < 2M
		//	pPref->SetInt("videoenc.x264.targetQp", 0);
		//}
	}
}

void CTransWorker::calcDarWhenCropAndNormalizeCrop(CVideoEncoder* pVideoEnc)
{
	if(!pVideoEnc || !m_srcVideoAttrib) return;
	video_info_t* pVInfo = pVideoEnc->GetVideoInfo();
	CXMLPref* pPref = pVideoEnc->GetVideoPref();
	if(!pVInfo || !pPref) return;

	int cropMode = pPref->GetInt("videofilter.crop.mode");
	int srcW = m_srcVideoAttrib->width;
	int srcH = m_srcVideoAttrib->height;
	
	float cropX = 0, cropY = 0, cropW = 0, cropH = 0;
	if (cropMode == 1 || cropMode == 2 || cropMode == 4) {	// Manual or auto crop
		cropX = pPref->GetFloat("videofilter.crop.left");
		cropY = pPref->GetFloat("videofilter.crop.top");
		cropW = pPref->GetFloat("videofilter.crop.width");
		cropH = pPref->GetFloat("videofilter.crop.height");
		if((cropW > 0 && cropW < 1) || (cropH > 0 && cropH < 1)) {
			cropX *= srcW;
			cropW *= srcW;
			cropY *= srcH;
			cropH *= srcH;
			int cx = (int)cropX, cy = (int)cropY;
			int cw = (int)cropW, ch = (int)cropH;
			// left and top border use bigger even number
			if(cx%2 != 0) cx += 1;
			if(cy%2 != 0) cy += 1;
			if(cw%2 != 0) cw -= 1;
			if(ch%2 != 0) ch -= 1;

			pPref->SetInt("videofilter.crop.left", cx);
			pPref->SetInt("videofilter.crop.top", cy);
			pPref->SetInt("videofilter.crop.width", cw);
			pPref->SetInt("videofilter.crop.height", ch);
		}
	}

	// If dest dar is not set, calc dar when crop
	if(pVInfo->dest_dar.den <= 0 && cropH > 0 && cropW > 0) {	
		// Get src video par
		int srcParNum = 0, srcParDen = 0;
		CAspectRatio aspectConvert(srcW, srcH);
		aspectConvert.SetDAR(m_srcVideoAttrib->dar_num, m_srcVideoAttrib->dar_den);
		aspectConvert.GetPAR(&srcParNum, &srcParDen);

		if(srcParDen > 0) {
			double dstDar = (double)cropW/cropH*srcParNum/srcParDen;
			GetFraction(dstDar, &pVInfo->dest_dar.num, &pVInfo->dest_dar.den);
		}
	}
}

void CTransWorker::normalizeDelogoTimeRect(CVideoEncoder* pVideoEnc)
{
	if(!pVideoEnc || !m_srcVideoAttrib) return;
	video_info_t* pVInfo = pVideoEnc->GetVideoInfo();
	CXMLPref* pPref = pVideoEnc->GetVideoPref();
	if(!pVInfo || !pPref) return;
	int srcW = m_srcVideoAttrib->width;
	int srcH = m_srcVideoAttrib->height;
	if (pPref->GetBoolean("videofilter.delogo.enabled")) {
		int delogoNum = pPref->GetInt("videofilter.delogo.num");
		for (int i=0; i<delogoNum; ++i) {
			char posKey[48] = {0};
			sprintf(posKey, "videofilter.delogo.pos%d", i+1);
			const char* timePosStr = pPref->GetString(posKey);
			std::vector<float> vctVal;
			
			if(StrPro::StrHelper::parseStringToNumArray(vctVal, timePosStr)) {
				if(vctVal.size() != 6) continue;
				if((vctVal[2] > 0 && vctVal[2] < 1) || (vctVal[3] > 0 && vctVal[3] < 1) &&
					(vctVal[4] > 0 && vctVal[4] < 1) || (vctVal[5] > 0 && vctVal[5] < 1)) {
					vctVal[2] *= srcW;
					vctVal[4] *= srcW;
					vctVal[3] *= srcH;
					vctVal[5] *= srcH;
					char timePos[64] = {0};
					sprintf(timePos, "%d,%d,%d,%d,%d,%d", (int)vctVal[0], (int)vctVal[1], 
						(int)vctVal[2], (int)vctVal[3], (int)vctVal[4], (int)vctVal[5]);
					pPref->SetString(posKey, timePos);
				}
			}
		}
	}
}

void CTransWorker::normalizeLogoTimeRect(CVideoEncoder* pVideoEnc)
{
	if(!pVideoEnc || !m_srcVideoAttrib) return;
	video_info_t* pVInfo = pVideoEnc->GetVideoInfo();
	CXMLPref* pPref = pVideoEnc->GetVideoPref();
	if(!pVInfo || !pPref) return;
	bool enableExpand = pPref->GetBoolean("videofilter.expand.enabled");
	int cropMode = pPref->GetInt("videofilter.crop.mode");
	bool enableWaterMark = pPref->GetBoolean("videofilter.overlay.enabled");

	int srcW = m_srcVideoAttrib->width;
	int srcH = m_srcVideoAttrib->height;
	int dstW = pVInfo->res_out.width;
	int dstH = pVInfo->res_out.height;
	
	int cropX = 0, cropY = 0, cropW = 0, cropH = 0;
	if (cropMode == 1 || cropMode == 2 || cropMode == 4) {	// Manual or auto crop
		cropX = pPref->GetInt("videofilter.crop.left");
		cropY = pPref->GetInt("videofilter.crop.top");
		cropW = pPref->GetInt("videofilter.crop.width");
		cropH = pPref->GetInt("videofilter.crop.height");
	}

	// If add logo and src/dst resolution isn't the same.
	if(enableWaterMark) {
		// Normalize relative logo position
		float offsetX = pPref->GetFloat("videofilter.overlay.offsetX");
		float offsetY = pPref->GetFloat("videofilter.overlay.offsetY");
		float relativeWidth = pPref->GetFloat("videofilter.overlay.width");
		float relativeHeight = pPref->GetFloat("videofilter.overlay.height");
		if((offsetX > 0 && offsetX < 1) || (offsetY > 0 && offsetY < 1) ||
		(relativeWidth > 0 && relativeWidth < 1) || (relativeHeight > 0 && relativeHeight < 1)) {
			int ox = (int)(offsetX*dstW);
			int oy = (int)(offsetY*dstH);
			int ow = (int)(relativeWidth*dstW);
			int oh = (int)(relativeHeight*dstH);
			pPref->SetInt("videofilter.overlay.offsetX", ox);
			pPref->SetInt("videofilter.overlay.offsetY", oy);
			pPref->SetInt("videofilter.overlay.width", ow);
			pPref->SetInt("videofilter.overlay.height", oh);
		}

		int expandX = 0, expandY = 0;
		if(enableExpand) {
			expandX = pPref->GetInt("videofilter.expand.x");
			expandY = pPref->GetInt("videofilter.expand.y");
		}
		char* imageFile = (char *)pPref->GetString("videofilter.overlay.image");
		std::vector<std::string> vctFiles;
		char *token = strtok(imageFile, "|");
		while(token!=NULL) {
			char *imgPath = token;
			std::string tmp = imgPath;
			vctFiles.push_back(tmp);
			token = strtok(NULL, "|");
		}
		const int maxLogoNum = 4;
		for (int i=1; i<=maxLogoNum; ++i) {
			char posKey[32] = {0};
			sprintf(posKey, "videofilter.overlay.pos%d", i);
			const char* timePosStr = pPref->GetString(posKey);
			if(!timePosStr || !(*timePosStr)) continue;

			std::vector<float> vctVal;
			int logox = -1, logoy = -1, logow = -1, logoh = -1;
			if(StrPro::StrHelper::parseStringToNumArray(vctVal, timePosStr)
				&& vctVal.size() == 6) {
				// Absolute axis value, convert to relative value
				if((vctVal[2] > 1 || vctVal[3] > 1 || vctVal[4] > 1 || vctVal[5] > 1)) {
					vctVal[2] /= srcW;
					vctVal[3] /= srcH;
					vctVal[4] /= srcW;
					vctVal[5] /= srcH;
				}
				if(vctVal[2]==-1&&vctVal[5]==-1) {
					cimg_library::CImg<uint8_t>* img = new cimg_library::CImg<uint8_t>(vctFiles[i-1].c_str());
					vctVal[2] = vctVal[4]-img->width()/(float)dstW;
					vctVal[5] = img->height()/(float)dstH;
					vctVal[4] = img->width()/(float)dstW;
				}
				else{
					vctVal[4] = vctVal[4]-vctVal[2];
					vctVal[5] = vctVal[5]-vctVal[3];
				}
				if(cropW > 0 && cropH >0) {
					float relativeCropx = (float)cropX/srcW;
					float relativeCropy = (float)cropY/srcH;
					float relativeCropw = (float)cropW/srcW;
					float relativeCroph = (float)cropH/srcH;
					vctVal[2] = (vctVal[2] - relativeCropx)/relativeCropw;
					vctVal[3] = (vctVal[3] - relativeCropy)/relativeCroph;
					vctVal[4] /= relativeCropw;
					vctVal[5] /= relativeCroph;
				}

				logox = (int)(vctVal[2]*dstW);
				logow = (int)(vctVal[4]*dstW);
				logoy = (int)(vctVal[3]*dstH);
				logoh = (int)(vctVal[5]*dstH);
				if(enableExpand) {
					logox += expandX;
					logoy += expandY;
				}
				
				// Change logo position
				if(logox != -1 && logoy != -1 && logow != -1 && logoh != -1) {
					char timePos[64] = {0};
					sprintf(timePos, "%d,%d,%d,%d,%d,%d", (int)vctVal[0], (int)vctVal[1], 
						logox, logoy, logow, logoh);
					pPref->SetString(posKey, timePos);
				}
			}
		}
	}
}

bool CTransWorker::validateTranscode(int decoderExitCode)
{
	// ignoreErrIdx: 0(no ignore), 1(ignore 32), 2(ignore33), 3(ignore both)
	int ignoreErrIdx = CWorkManager::GetInstance()->GetIgnoreErrorCode();

    logger_info(m_logType, "exit(%d), ignore(%d), main(%d), audio(%f), video(%f).\n",
        decoderExitCode, ignoreErrIdx,
        m_tmpBenchData.mainDur,
        m_tmpBenchData.audioEncTime,
        m_tmpBenchData.videoEncTime);

	#define FAIL_INFO(err) logger_err(m_logType, err); ret=false; break;
	bool ret = true;
	do {
		// Check decoding integrity
		if((ignoreErrIdx == 0 || ignoreErrIdx == 2) && decoderExitCode != 0 &&
			m_tmpBenchData.mainDur > 0 && m_tmpBenchData.mainDur != INT_MAX) {
			if(m_tmpBenchData.audioEncTime > 0.001f) {
				if(m_tmpBenchData.mainDur > m_tmpBenchData.audioEncTime*1200) {
					SetErrorCode(EC_DECODER_ABNORMAL_EXIT);
					ret = false;
					logger_err(m_logType, "AudioDecoder exit abnormally(exit code:%d).\n", decoderExitCode);
					break;
				}
			}
			if(m_tmpBenchData.videoEncTime > 0.001f) {
				if(m_tmpBenchData.mainDur > m_tmpBenchData.videoEncTime*1200) {
					SetErrorCode(EC_DECODER_ABNORMAL_EXIT);
					ret = false;
					logger_err(m_logType, "VideoDecoder exit abnormally(exit code:%d).\n", decoderExitCode);
					break;
				}
			}
		}

		// Check a/v duration
		if((ignoreErrIdx == 0 || ignoreErrIdx == 1) &&
		m_tmpBenchData.audioEncTime > 0.001f && m_tmpBenchData.videoEncTime > 0.001f) {
			if(fabs(m_tmpBenchData.audioEncTime - m_tmpBenchData.videoEncTime) > 60) {
				SetErrorCode(EC_AV_DURATION_BIG_DIFF);
				FAIL_INFO("A/V duration differs more than 60s.\n");
			}
		}

		//check the error code
		if (GetErrorCode() != EC_NO_ERROR) {
			ret = false;
		}
	} while(false);

	#undef FAIL_INFO
	return ret;
}

bool CTransWorker::processLosslessClip()
{
	const char* srcFile = m_streamFiles.GetFirstSrcFile();
	if(!srcFile || !(*srcFile)) return false;
	std::vector<CProcessWrapper*> spliters;
	
	for(size_t i=0; i<m_clipStartSet.size(); ++i) {
		std::string cmdStr = MP4BOX" \"";
		cmdStr += srcFile;
		cmdStr += "\" -splitx ";
		char intervalStr[64] = {0};
		float startTime = m_clipStartSet[i]/1000.f;
		float endTime = m_clipEndSet[i]/1000.f;
		sprintf(intervalStr, "%1.2f:%1.2f", startTime, endTime);
		cmdStr += intervalStr;
		cmdStr += " -out \"";
		cmdStr += m_streamFiles.GetStreamFileName(ST_MUXER, MUX_MP4, i);
		cmdStr += "\"";
		CProcessWrapper* splitter = new CProcessWrapper;
#ifdef _WIN32
		splitter->Create(cmdStr.c_str());
#else
		splitter->Run(cmdStr.c_str());
#endif
		spliters.push_back(splitter);
	}

	for (size_t i=0; i<spliters.size(); ++i) {
        spliters[i]->Wait(0xFFFFFFFF);      // Infinite
	}

	// Concat
	std::vector<std::string> tmpClipFiles = m_streamFiles.GetTempDestFiles();
	std::string catStr = MP4BOX;
	for (size_t i=0; i<tmpClipFiles.size(); ++i) {
		catStr += " -cat \"";
		catStr += tmpClipFiles[i];
		catStr += "\"";
	}
	catStr += " -new \"";
	catStr += m_streamFiles.GetDestFiles().at(0);
	catStr += "\"";
	int ret = CProcessWrapper::Run(catStr.c_str());

	for (size_t i=0; i<spliters.size(); ++i) {
		delete spliters[i];
	}
	return (ret == 0);
}

void CTransWorker::CleanUp()
{
	// Reset parameters;
	ResetParams();

	// Clear temp stream files
	m_streamFiles.Clear();		

	if(m_pRootPref) {
		delete m_pRootPref;
		m_pRootPref = NULL;
	}
	
	if(m_pcmBuf) {
		free(m_pcmBuf);
		m_pcmBuf = NULL;
	}

	if(m_yuvBuf) {
		free(m_yuvBuf);
		m_yuvBuf = NULL;
	}

	while (!m_srcAudioAttribs.empty()) {
		attr_audio_t *attrib = m_srcAudioAttribs.back();
		m_srcAudioAttribs.pop_back();
		delete attrib;
	}
	m_srcAudioAttribs.clear();

	if(m_srcVideoAttrib) {
		delete m_srcVideoAttrib;
		m_srcVideoAttrib = NULL;
	}

	if(m_pPlaylist) {
		delete m_pPlaylist;
		m_pPlaylist = NULL;
	}

	if(m_hMainThread) {
		CLOSE_THREAD_HANDLE(m_hMainThread);
		m_hMainThread = NULL;
	}
	if(m_hAudioThread) {
		CLOSE_THREAD_HANDLE(m_hAudioThread);
		m_hAudioThread = NULL;
	}
	if(m_hVideoThread) {
		CLOSE_THREAD_HANDLE(m_hVideoThread);
		m_hVideoThread = NULL;
	}
	
#ifdef DEMO_RELEASE
	if(m_pWaterMarkMan) {
		delete m_pWaterMarkMan;
		m_pWaterMarkMan = NULL;
	}
#endif

#ifdef DEBUG_DUMP_RAWVIDEO
	if (m_rawvideo_output) {
		destroyStreamOutput(m_rawvideo_output);
		m_rawvideo_output = NULL;
	}
#endif

#ifdef DEBUG_DUMP_RAWAUDIO
	if (m_rawaudio_output) {
		destroyStreamOutput(m_rawaudio_output);
		m_rawaudio_output = NULL;
	}
#endif
}

void CTransWorker::ResetParams()
{
	m_errCode = EC_NO_ERROR;
	m_lastTime = m_startTime = 0;
	m_endTime = 0;
	m_elapseSecs = 0;
	m_taskState = TASK_READY;
	m_savedState = TASK_NOTSET;
	m_progress = 0.f;
	m_strEncodeSpeed.clear();
	m_encodedFrames = 0;
	//m_encodedPcmLen = 0;
	m_bUseCudaEncoder = false;
	m_bEnableDecoderVf = false;
	m_bEnableDecoderAf = false;
	m_tmpBenchData.mainDur  = -100;
	m_tmpBenchData.fpsDen   = 0;
	m_tmpBenchData.fpsNum   = 0;
	m_tmpBenchData.audioEncTime = 0.f;
	m_tmpBenchData.videoEncTime = 0.f;
	m_tmpBenchData.lastFrameNum = 0;
	
	m_bAutoVolumeGain = false;
	m_encAudioBytes = 0;
	m_uuid = -1;
}

std::string CTransWorker::GetOutputFileName(int index)
{
	if (index < 0) return "";

	std::vector<std::string> files = m_streamFiles.GetDestFiles();

	if (index < (int)files.size()) {
		return files[index];
	} else {
		return "";
	}
}

task_state_t CTransWorker::GetAndResetState()
{
	task_state_t savedState = m_taskState;

	if (m_taskState == TASK_DONE || 
		m_taskState == TASK_ERROR || m_taskState == TASK_CANCELLED) {
		m_taskState = TASK_REUSABLE;
	}

	return savedState;
}

bool CTransWorker::Pause()
{
	if (m_taskState >= TASK_PAUSED) {
		logger_err(m_logType, "Unable to pause (current state:%d)\n", m_taskState);
		return false;
	}
	m_savedState = m_taskState;
	m_taskState = TASK_PAUSED;
	return true;
}

bool CTransWorker::Resume()
{
	if (m_taskState != TASK_PAUSED) {
		logger_err(m_logType, "Unable to resume (current state:%d)\n", m_taskState);
		return false;
	}

	m_taskState = m_savedState;

	return true;
}

bool CTransWorker::Stop()
{
	if(m_taskState < TASK_ENCODING) {
		logger_err(m_logType, "Unable to stop task, the task hasn't started.\n");
		return false;
	}

	return true;
}

bool CTransWorker::Cancel()
{
	if (m_taskState >= TASK_DONE) {
		logger_err(m_logType, "Unable to stop (current state:%d)\n", m_taskState);
		return false;
	}

	Stop();
	
	m_taskState = TASK_CANCELLED;
	return true;
}

bool CTransWorker::createTranscodeAudioThread()
{
	int ret = 0;
	ret = CBIThread::Create(m_hAudioThread, transcodeAudioEntry, this);
	if (ret != 0) {
		SetErrorCode(EC_CREATE_THREAD_ERROR);
		return false;
	}

#ifdef LOWER_ENCODE_THREAD
	CBIThread::SetPriority(m_hAudioThread, THREAD_PRIORITY_BELOW_NORMAL);
#endif

	logger_status(m_logType, "Create audio transcode thread successfully.\n");
	return true;
}


bool CTransWorker::createTranscodeVideoThread()
{
	int ret = CBIThread::Create(m_hVideoThread, transcodeVideoEntry, this);
	if (ret != 0) {
		SetErrorCode(EC_CREATE_THREAD_ERROR);
		return false;
	}

#ifdef LOWER_ENCODE_THREAD
	CBIThread::SetPriority(m_hVideoThread, THREAD_PRIORITY_BELOW_NORMAL);
#endif

	logger_status(m_logType, "Create video transcode thread successfully.\n");
	return true;
}

THREAD_RET_T WINAPI CTransWorker::transcodeAudioEntry(void* transWorker)
{
	CTransWorker* theWorker = static_cast<CTransWorker*>(transWorker);
	if(theWorker == NULL) return -1;
	return theWorker->transcodeAudio();
}

THREAD_RET_T WINAPI CTransWorker::transcodeVideoEntry(void* worker)
{
	CTransWorker* theWorker = static_cast<CTransWorker*>(worker);
	if(!theWorker) return -1;
	return theWorker->transcodeVideo();
}

bool CTransWorker::GetTaskInfo(ts_task_info_t* taskInfo)
{
	if (taskInfo == NULL) return false;

	taskInfo->id = this->GetGlobalId();
	taskInfo->localId =  this->GetId();
	taskInfo->elapsed_time = this->GetElapsedTime();
	taskInfo->progress = this->GetProgress();
	std::string speedStr = this->GetSpeedString();
	strncpy(taskInfo->speed, speedStr.c_str(), sizeof(taskInfo->speed));
	taskInfo->error_code = this->GetErrorCode();
	//strncpy(taskInfo->dstmd5, m_dstMd5.c_str(), m_dstMd5.size());
	//taskInfo->dstmd5[32] = '\0';
	//Place the GetAndResetState() at the last please
	taskInfo->lastState = this->m_savedState;
	taskInfo->state = this->GetAndResetState();
	
	return true;
}

// TODO: FIX TASK INFO AND CLEAN WORKER MEMORY
int CTransWorker::CreateTask(const char *prefs,  char* outFileName /*OUT*/)
{
#define FAIL_INFO(err) logger_err(m_logType, err); break;
	int ret = 0;
	do {
		if (prefs == NULL || *prefs == '\0') {
			ret = AJE_INVALID_PREF;
			SetErrorCode(EC_INVALID_PREFS);
			FAIL_INFO("Invalid preset, check again.\n");
		}
	
		m_pRootPref = new CRootPrefs;
		if (m_pRootPref == NULL) {
			ret = AJE_INIT_WORKER_FAIL;
			SetErrorCode(EC_INIT_ERROR);
			FAIL_INFO("Create preset parser failed.\n");
		}

		//string goodPrefs = MEvaluater::FillOutputNodeWithPresetFile(prefs);
		//----------- Modify to remove merging default mccore.xml------------
		if (!m_pRootPref->InitRoot(prefs)) {
			ret = AJE_INIT_WORKER_FAIL;
			SetErrorCode(EC_INVALID_PREFS);
			FAIL_INFO("Initialize preset parser failed.\n");
		}

		if(!this->ParseSetting()) {
			ret = AJE_INIT_WORKER_FAIL;
			FAIL_INFO("Parse preset failed.\n");
		}

		if(this->StartWork() != 0) {
			ret = AJE_START_WORKER_FAIL;
			FAIL_INFO("Start working failed.\n");
		}
		
		if (outFileName) {
			std::string destFileName = this->GetOutputFileName(0);
			strcpy(outFileName, destFileName.c_str());
		}
	} while (false);

	if(ret != 0) {
		SetState(TASK_ERROR);
	}

#undef FAIL_INFO
	return ret;
}
