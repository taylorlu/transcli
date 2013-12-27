
#ifdef WIN32
#include <io.h>
#endif
#include <stdio.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include "bitconfig.h"

#ifdef DEMO_RELEASE
#include "WaterMarkFilter.h"
#endif

#include "TransWorkerUnify.h"

#include "MEvaluater.h"
#include "StreamOutput.h"
#include "TransnodeUtils.h"
#include "WorkManager.h"

#ifdef HAVE_WMENCODER
#include "WmaEncoder.h"
#endif

#include "DecoderMencoder.h"
#include "DecoderMplayer.h"
#include "DecoderVLC.h"
#include "DecoderAVS.h"
#include "DecoderCopy.h"

#include "vfhead.h"

#include "AudioResampler.h"
#include "util.h"
#include "yuvUtil.h"
#include "logger.h"
#include "strpro/StrHelper.h"
#include "FFMpegAVEncoder.h"

#define SAFESTRING(str) ((str)?(str):("")) 
#define PROCESS_PAUSE_STOP if(m_taskState == TASK_PAUSED) { \
			Sleep(100);		continue; \
		} \
		if(m_taskState >= TASK_DONE || GetErrorCode() != EC_NO_ERROR) { \
			logger_info(m_logType, "Task cancelled, exit encoding thread.\n"); break; \
		}


#ifndef _WIN32
#include <sched.h>
#include <unistd.h>
#endif

CTransWorkerUnify::CTransWorkerUnify(void) : m_pAVEncoder(NULL)
{
	m_logType = LOGM_TS_WK_UNI;
}

CTransWorkerUnify::~CTransWorkerUnify(void)
{
}

int CTransWorkerUnify::StartWork()
{
	int ret = 0;
	ret = CBIThread::Create(m_hMainThread, CTransWorkerUnify::mainThread, this);
	if(ret != 0) {
		m_taskState = TASK_ERROR;
		logger_err(m_logType, "Failed to start Worker %d .\n", m_id);
		SetErrorCode(EC_START_ERROR);
	} 
	
	return ret;
}

THREAD_RET_T WINAPI CTransWorkerUnify::mainThread(void* transWorker)
{
	#define FAIL_INFO(err) logger_err(pWorker->m_logType, err); ret=-1; break;

	CTransWorkerUnify* pWorker = static_cast<CTransWorkerUnify*>(transWorker);
	int ret = 0;
	pWorker->m_startTime = pWorker->m_lastTime = GetTickCount();
	pWorker->m_taskState = TASK_ENCODING;

	logger_status(pWorker->m_logType, "Enter tansworker main thread\n");

	do {
		if(!pWorker) {
			FAIL_INFO("Param passed in main thread is NULL.\n");
		}

		// Start transcoding (using ffmpeg)
		if(pWorker->m_pAVEncoder->GetEnableAudio() == false &&
			pWorker->m_pAVEncoder->GetEnableVideo() == false ) {
            FAIL_INFO("No audio or video stream to transcode.\n");
		}

		if(!pWorker->initialize()) {
			FAIL_INFO("Initialize or start transcoder failed.\n");
		}

		// Wait for ffmpeg to finish
		CProcessWrapper* ffmpegProc = pWorker->m_pAVEncoder->GetProcessWrapper();
		while(ffmpegProc->IsProcessRunning()) {
			if(pWorker->GetState() == TASK_CANCELLED || 
				pWorker->GetErrorCode() != EC_NO_ERROR) {
					ffmpegProc->Cleanup();
			}
			// Sleep 2 seconds
			Sleep(2000);
		}

		if(pWorker->GetState() == TASK_CANCELLED) {
			logger_warn(pWorker->m_logType, "User canceled transcoding.\n");
		}

		//check the error code
		if (pWorker->GetErrorCode() != EC_NO_ERROR) {
			FAIL_INFO("Error occurs during encoding.\n");
		}
	} while (false);

	if(ret == 0) {
		if (pWorker->m_taskState != TASK_CANCELLED) {
			logger_status(pWorker->m_logType, "Mux completed, transcode task done successfully!\n");
			pWorker->m_taskState = TASK_DONE;
		}
		pWorker->m_progress = 100.f;
	} else {
		logger_err(pWorker->m_logType, "Transcode task failed and exit!\n");
		pWorker->m_taskState = TASK_ERROR;
		pWorker->m_progress = 0.f;
	}
	
	pWorker->m_endTime = GetTickCount();
	pWorker->m_elapseSecs = (int)((pWorker->m_endTime - pWorker->m_startTime)/1000.f);
	logger_info(pWorker->m_logType, "Time consume: %ds.\n",  pWorker->m_elapseSecs);
	
	pWorker->CleanUp();
	//FIXME:
	//CWorkManager::GetInstance()->WorkerCompleteReport(pWorker->GetId(), ret);
	logger_status(pWorker->m_logType, "Exist tansworker main thread.\n");

	#undef FAIL_INFO
	return 0;
}

bool CTransWorkerUnify::SetPreset(CXMLPref* pAudioPref, CXMLPref* pVideoPref, CXMLPref* pMuxerPref)
{
	if(!pMuxerPref) {
		logger_err(m_logType, "Output setting is absent(Muxer pref is empty).\n");
		return false;
	}
	
	const char* srcFile = m_streamFiles.GetFirstSrcFile();
    const char* trackTitle = NULL; //m_streamFiles.GetTrackTitle();
	if(srcFile && (strstr(srcFile, "dvd://") || strstr(srcFile, "vcd://")) && 
		trackTitle && *trackTitle != '\0') {		// it's dvd/vcd track
		srcFile = trackTitle;
	}
	
	int destFilesIdx = m_streamFiles.GetDestFiles().size()+1;
	const char* desExt = GetFFMpegOutFileExt(pMuxerPref->GetInt("overall.container.format"));
	const char* relativePath = m_streamFiles.GetRelativeDir();
	std::string destFileName = genDestFileName(srcFile, pMuxerPref, desExt, relativePath, destFilesIdx);
	if(!destFileName.empty()) {
		m_streamFiles.AddDestFile(destFileName);
		m_pAVEncoder = new CFFMpegAVEncoder(destFileName.c_str());
		m_pAVEncoder->SetMuxerPref(pMuxerPref);
	} else {
		return false;
	}

	if(pAudioPref && pAudioPref->GetBoolean("overall.audio.enabled")) {
		int chEnum = pAudioPref->GetInt("overall.audio.channels");
		int channelNumArray[7] = {0, 1, 1, 2, 4, 5, 6};
		int outChannelNum = channelNumArray[chEnum];
		audio_format_t aFormat = (audio_format_t)pAudioPref->GetInt("overall.audio.format");
		audio_encoder_t encType = (audio_encoder_t)pAudioPref->GetInt("overall.audio.encoder");
		int outSampleRate = pAudioPref->GetInt("audiofilter.resample.samplerate");
		if(outSampleRate < 0) outSampleRate = 0;

		// correct audio encoder if not proper
		if (aFormat == AC_AAC_HE || aFormat == AC_AAC_HEV2) {
			encType = AE_NEROREF;
// 			int aacFormat = pAudioPref->GetInt("audioenc.nero.format");
// 			if(aacFormat == 1) {	// LC format
// 				aFormat = AC_AAC_LC;
// 			}
		}
		audio_info_t audioInfo = {encType, aFormat, AC_PCM, AF_DECODER, 0, outSampleRate, 
				0, outChannelNum, 0, 0, 0, -1};
		m_pAVEncoder->SetAudioInfo(audioInfo, pAudioPref);
		m_pAVEncoder->SetEnableAudio(true);
	}

	if(pVideoPref && pVideoPref->GetBoolean("overall.video.enabled")) {
		int filterType = pVideoPref->GetInt("videofilter.generic.applicator");
		if(filterType < 0) filterType = 0;
		vf_type_t vfType = (vf_type_t)(filterType);
		
		resolution_t outSize = {0, 0};
		fraction_t outFps = {0, 0}, outDar = {0, 0}, outPar = {0, 0};

		//int rateCtrlMode = pVideoPref->GetInt("overall.video.mode");
		//if(rateCtrlMode >= RC_MODE_2PASS) m_encoderPass = rateCtrlMode -1;
		int aspectType = pVideoPref->GetInt("overall.video.ar");
		if(aspectType == 2) {
			outDar.num = pVideoPref->GetInt("overall.video.arNum");
			outDar.den = pVideoPref->GetInt("overall.video.arDen");
		} else if(aspectType == 3) {
			outPar.num = pVideoPref->GetInt("overall.video.arNum");
			outPar.den = pVideoPref->GetInt("overall.video.arDen");
		}

		video_format_t  encFormat = (video_format_t)pVideoPref->GetInt("overall.video.format");
		video_encoder_t encType = (video_encoder_t)pVideoPref->GetInt("overall.video.encoder");
		/*if(encType == 0 && pVideoPref->GetBoolean("overall.video.autoEncoder")) {
			encType = VE_FFMPEG;
		}*/

		if(pVideoPref->GetBoolean("videofilter.scale.enabled")) {
			outSize.width = pVideoPref->GetInt("videofilter.scale.width");
			outSize.height = pVideoPref->GetInt("videofilter.scale.height");
		}
		if(pVideoPref->GetBoolean("videofilter.frame.enabled")) {
			outFps.num = pVideoPref->GetInt("videofilter.frame.fps");
			outFps.den = pVideoPref->GetInt("videofilter.frame.fpsScale");
		}
	
		if(CTransnodeUtils::GetUseSingleCore()) {	// Use single cpu core to encode
			pVideoPref->SetInt("videoenc.x264.threads", 1);
			pVideoPref->SetInt("videoenc.xvid.threads", 1);
		}

		video_info_t videoInfo = {CF_DEFAULT, encType, encFormat, vfType, outSize, {0, 0}, outFps, {0, 0},
		outPar, {0,0}, outDar, 0, 0, RAW_I420, 8, 0, 0, 0};
	
		m_pAVEncoder->SetVideoInfo(videoInfo, pVideoPref);
		m_pAVEncoder->SetEnableVideo(true);
	}
	return true;
}

bool CTransWorkerUnify::initialize()
{
	//================== Set decoder command and start decoder =====================
	bool success = true;
	logger_info(m_logType, "Start transcoding %s.\n", m_streamFiles.GetFirstSrcFile()); 

	// Adjust video out resolution(width or height is -1)
	video_info_t* pvInfo = m_pAVEncoder->GetVideoInfo();
	if(pvInfo) {
		resolution_t* resOut = &pvInfo->res_out;
		fraction_t dar = pvInfo->dest_dar;
		if(dar.den == 0 || dar.num == 0) {
			dar = pvInfo->src_dar;
		}
		if(dar.den != 0 && dar.num != 0) {
			if(resOut->width < 0 && resOut->height > 0) {
				resOut->width = resOut->height*dar.num/dar.den;
				EnsureMultipleOfDivisor(resOut->width, 4);
			}
			if(resOut->height < 0 && resOut->width > 0) {
				resOut->height = resOut->width*dar.den/dar.num;
				EnsureMultipleOfDivisor(resOut->height, 4);
			}
		} 
	}

// #ifdef DEMO_RELEASE
// 	// Enable transerver demo watermark
// 	if(m_pAVEncoder->GetEnableVideo() && !m_pWaterMarkMan) {
// 		m_pWaterMarkMan = new CWaterMarkManager();
// 		if(m_pWaterMarkMan) {
// 			CWaterMarkFilter* pWaterMark = m_pWaterMarkMan->AddWaterMark();
// 			pWaterMark->AddShowTimeAndPosition();
// 			pWaterMark->SetAlpha(0.3f);
// 			//m_pWaterMark->DumpImageToText("d:\\logo.txt");
// 			//m_pWaterMark->SetYUVFrameSize(pVInfo->res_in.width, pVInfo->res_in.height);
// 		}
// 	}
// #endif

	do {
		// Launch decoder (mplayer or mencoder)
		const char* srcFile = m_streamFiles.GetCurSrcFile();
		if(!srcFile || *srcFile == '\0') {
			logger_err(m_logType, "No Input file.\n");
			SetErrorCode(EC_INIT_ERROR);
			success = false;
			break;
		}
		m_pAVEncoder->SetSourceFile(srcFile);
		if(m_pAVEncoder->GetEnableAudio()) {
			success = initAudioEncodePart();
			if(!success) break;
		}

		if(m_pAVEncoder->GetEnableVideo()) {
			success = initVideoEncodePart();
			if(!success) break;
		}

		if(m_pAVEncoder->GetEnableAudio() || m_pAVEncoder->GetEnableVideo()) {
			success = m_pAVEncoder->Initialize();
		}

		logger_status(m_logType, "TransWorker is initialized successfully.\n");
	} while (false);

	return success;
}

bool CTransWorkerUnify::ParseSetting()
{
	bool ret = true;
#define FAIL_INFO(err) logger_err(m_logType, err); ret=false; break;

	do {
		if(!m_pRootPref) {
			FAIL_INFO("Root preference hasn't been set.\n");
		}

		CStreamPrefs* pStreamPref = m_pRootPref->GetStreamPrefs();
		if (!pStreamPref) {
			FAIL_INFO("Stream pref is NULL.\n");
		}

		const char* inputUrl = m_pRootPref->GetUrl();
		const char* tempdir = m_pRootPref->GetTempDir();
		const char* subtitleFile = m_pRootPref->GetSubTitle();
        const char* trackTitle = NULL;//m_pRootPref->GetTrackName();
		
		// Set several source files
		if(inputUrl && *inputUrl != '\0') {
			std::vector<std::string> srcFiles;
			if(StrPro::StrHelper::splitString(srcFiles, inputUrl, "|")) {
				for (size_t i=0; i<srcFiles.size(); ++i) {
					if(!setSrcFile(srcFiles[i].c_str())) {
						FAIL_INFO("Bad source file.\n");
					}
				}
				if(ret == false) break;
			} else {
				FAIL_INFO("Bad source file.\n");
			}
		} else {
			FAIL_INFO("Source file not provided.\n");
		}

        m_streamFiles.SetWorkerId(GetId());
		m_streamFiles.SetTempDir(tempdir);
		if(subtitleFile && *subtitleFile != '\0') m_streamFiles.SetSubTitleFile(subtitleFile);
        //if(trackTitle && *trackTitle != '\0') m_streamFiles.SetTrackTitle(trackTitle);
		
		CXMLPref* audioPref = NULL;
		if(pStreamPref->GetAudioCount() > 0) audioPref = pStreamPref->GetAudioPrefs(0);
		CXMLPref* videoPref = NULL;
		if(pStreamPref->GetVideoCount() > 0) videoPref = pStreamPref->GetVideoPrefs(0);
		CXMLPref* muxerPref = NULL;
		if(pStreamPref->GetAudioCount() > 0) muxerPref = pStreamPref->GetAudioPrefs(0);
		
		if(!SetPreset(audioPref, videoPref, muxerPref)) {
			FAIL_INFO("Set preset failed.\n");
		}

		// Parse media info and get duration
        //StrPro::CXML* pMediaPref = m_pRootPref->GetMediaInfoNode();

		if (pMediaPref != NULL) {
			if(m_tmpBenchData.mainDur <= 0) {
				pMediaPref->goRoot();
				pMediaPref->findChildNode("stream");
				m_tmpBenchData.mainDur = pMediaPref->getChildNodeValueInt("duration");
			}
			setSourceAVInfo(pMediaPref);
			if(m_tmpBenchData.mainDur <= 0 && m_srcVideoAttrib) {
				m_tmpBenchData.mainDur = m_srcVideoAttrib->duration;
			}
			if(m_tmpBenchData.mainDur <= 0 && m_srcAudioAttrib) {
				m_tmpBenchData.mainDur = m_srcAudioAttrib->duration;
			}
			if(m_tmpBenchData.mainDur <= 0) m_tmpBenchData.mainDur = INT_MAX;	// A big int
		}
	} while (false);
	
	if(!ret) {
		SetErrorCode(EC_INVALID_SETTINGS);
	}
#undef FAIL_INFO
	return ret;
}

bool CTransWorkerUnify::setSourceAVInfo(StrPro::CXML* mediaInfo)
{
	bool ret = initAVSrcAttrib(mediaInfo);
	if(ret) {
		const char* srcFile = m_streamFiles.GetFirstSrcFile();
		if(srcFile && strstr(srcFile, "://")) return true;
		if(!m_srcAudioAttrib && !m_srcVideoAttrib) {
			std::string ext;
			StrPro::StrHelper::getFileExt(srcFile, ext);
			if(_stricmp(ext.c_str(), ".mpg") == 0 ||		// Hik format, media info can't recognize
				_stricmp(ext.c_str(), ".264") == 0 ||
				_stricmp(ext.c_str(), ".mp4") == 0 ||
				_stricmp(ext.c_str(), ".iso") == 0 ||
				_stricmp(ext.c_str(), ".rmvb") == 0 ||
				_stricmp(ext.c_str(), ".rm") == 0) 
			return true; 
		}

		if(!m_srcAudioAttrib || (m_srcAudioAttrib->id < 0 && m_srcAudioAttrib->duration <= 0 && m_srcAudioAttrib->samplerate <= 0)) {
			logger_err(m_logType, "Invalid Audio Attrib, disable audio encoding!\n");
			m_pAVEncoder->SetEnableAudio(false);
		}
		if(!m_srcVideoAttrib || (m_srcVideoAttrib->id < 0 && m_srcVideoAttrib->width <= 0 && m_srcVideoAttrib->duration <= 0)) {
			logger_err(m_logType, "Invalid video Attrib, disable video encoding!\n");
			m_pAVEncoder->SetEnableVideo(false);
		}
	}
	
	return ret;
}

void CTransWorkerUnify::CleanUp()
{
	CTransWorker::CleanUp();
	if(m_pAVEncoder) {
		delete m_pAVEncoder;
		m_pAVEncoder = NULL;
	}
}

bool CTransWorkerUnify::initAVSrcAttrib(StrPro::CXML* mediaInfo)
{
	// Parse audio info
	mediaInfo->goRoot();
	if (mediaInfo->findChildNode("audio")) {
		if(!m_srcAudioAttrib) {
			m_srcAudioAttrib = new attr_audio_t;
		}
		memset(m_srcAudioAttrib, 0, sizeof(attr_audio_t));
		m_srcAudioAttrib->id = -1;
		parseMediaAudioInfoNode(mediaInfo, m_srcAudioAttrib);
    }
    
	// Parse video info
	mediaInfo->goRoot();
	if(mediaInfo->findChildNode("video")) {
		if(!m_srcVideoAttrib) {
			m_srcVideoAttrib = new attr_video_t;
		}	
		memset(m_srcVideoAttrib, 0, sizeof(attr_video_t));
		m_srcVideoAttrib->id = -1;
		parseMediaVideoInfoNode(mediaInfo, m_srcVideoAttrib);
	}

	// Set video source param
	if(m_srcVideoAttrib) {
		setVideoEncAttrib(m_pAVEncoder->GetVideoInfo(),m_pAVEncoder->GetVideoPref(), m_srcVideoAttrib);
	}

	// Set audio source param
	if(m_srcAudioAttrib) {
		setAudioEncAttrib(m_pAVEncoder->GetAudioInfo(), 
			m_pAVEncoder->GetAudioPref(), m_srcAudioAttrib);

		if(m_srcAudioAttrib->id < 110) {	// Only one audio track, don't set audio id
			m_pAVEncoder->GetAudioInfo()->aid = -1;
		}
	}
	return true;
}

void CTransWorkerUnify::ResetStateAndParams()
{
	CTransWorker::ResetStateAndParams();
}

void CTransWorkerUnify::fixAudioParam(audio_info_t* ainfo)
{
	if(!ainfo) return;

	if(ainfo->bits <= 0) ainfo->bits = 16;
	
	if(ainfo->out_channels <= 0) {
		ainfo->out_channels = ainfo->in_channels;
	} else if(m_bEnableDecoderAf){	// Decoder do audio filter
		ainfo->in_channels = ainfo->out_channels;
	}
	if(ainfo->out_srate <= 0) {
		ainfo->out_srate = ainfo->in_srate;
	} else if(m_bEnableDecoderAf){	// Decoder do audio filter
		ainfo->in_srate = ainfo->out_srate;
	}

	ainfo->in_bytes_per_sec = ainfo->in_srate*ainfo->in_channels*(ainfo->bits>>3);
}

void CTransWorkerUnify::fixVideoParam(video_info_t* vinfo)
{
	if(!vinfo) return;
	int fps_num = vinfo->fps_in.num;
	int fps_den = vinfo->fps_in.den;
	if(fps_den == 0 || (float)fps_num/fps_den > 120 ) {
		fps_num = 25;
		fps_den = 1;
	}
	vinfo->fps_in.num = fps_num;
	vinfo->fps_in.den = fps_den;

	if(vinfo->res_out.width <= 0) {
		vinfo->res_out.width = vinfo->res_in.width;
	} else if (m_bEnableDecoderVf) {
		vinfo->res_in.width = vinfo->res_out.width;
	}
	if(vinfo->res_out.height <= 0) {
		vinfo->res_out.height = vinfo->res_in.height;
	} else if (m_bEnableDecoderVf) {
		vinfo->res_in.height = vinfo->res_out.height;
	}
	if(vinfo->fps_out.num <= 0) {
		vinfo->fps_out.num = vinfo->fps_in.num;
	} else if (m_bEnableDecoderVf) {
		vinfo->fps_in.num = vinfo->fps_out.num;
	}
	if(vinfo->fps_out.den <= 0) {
		vinfo->fps_out.den = vinfo->fps_in.den;
	} else if (m_bEnableDecoderVf) {
		vinfo->fps_in.den = vinfo->fps_out.den;
	}

	// Temp workaround for bad vf fps processing (if original fps is smaller)
	if((float)vinfo->fps_out.num/vinfo->fps_out.den > (float)vinfo->fps_in.num/vinfo->fps_in.den) {
		vinfo->fps_out.num = vinfo->fps_in.num;
		vinfo->fps_out.den = vinfo->fps_in.den;
	}

	if(vinfo->dest_dar.num <= 0) {
		vinfo->dest_dar.num = vinfo->src_dar.num;
	} 

	if(vinfo->dest_dar.den <= 0) {
		vinfo->dest_dar.den = vinfo->src_dar.den;
	} 

	CAspectRatio aspectConvert;
	aspectConvert.SetRes(vinfo->res_out.width, vinfo->res_out.height);
	aspectConvert.SetDAR(vinfo->dest_dar.num, vinfo->dest_dar.den);
	aspectConvert.GetPAR(&(vinfo->dest_par.num), &(vinfo->dest_par.den));
}

bool CTransWorkerUnify::initAudioEncodePart()
{
	bool initRet = true;
#define FAIL_INFO(err) logger_err(m_logType, err); initRet=false; break;
	logger_status(m_logType, "To init Audio Encoders\n");
	do {
		audio_info_t* pAudioInfo = m_pAVEncoder->GetAudioInfo();
		fixAudioParam(pAudioInfo);
		
#ifdef DEBUG_DUMP_RAWAUDIO
		if (m_rawaudio_output) {
			char dumpfilename[MAX_PATH];
			std::string destFile = GetOutputFileName(0);
			snprintf(dumpfilename, MAX_PATH, "%s_audio_%d_%d.raw", destFile.c_str(), pAudioInfo->in_srate, pAudioInfo->in_channels);
			DeleteFile(dumpfilename);
			m_rawaudio_output->Open(dumpfilename);
		}
#endif
	} while (false);

	if(!initRet) SetErrorCode(EC_AUDIO_INIT_ERROR);
#undef FAIL_INFO
	return initRet;
}

bool CTransWorkerUnify::initVideoEncodePart()
{
	#define FAIL_INFO(err) logger_err(m_logType, err); initRet=false; break;
	bool initRet = true;
	logger_status(m_logType, "To init Video Encoders\n");
	do {
		
		video_info_t* pVInfo = m_pAVEncoder->GetVideoInfo();
		fixVideoParam(pVInfo);
		if(m_tmpBenchData.fpsNum <= 0) {
			m_tmpBenchData.fpsNum = pVInfo->fps_out.num;
			m_tmpBenchData.fpsDen = pVInfo->fps_out.den;
			if(pVInfo->vfType == VF_NONE || pVInfo->vfType == VF_ENCODER){
				m_tmpBenchData.fpsNum = pVInfo->fps_in.num;
				m_tmpBenchData.fpsDen = pVInfo->fps_in.den;
			}
		}
			
		if(!initRet) {
			logger_err(m_logType, "Video encoding part init failed.\n");
			break;
		}
		
// #ifdef DEMO_RELEASE
// 		if(m_pAVEncoder->GetEnableVideo()) {
// 			resolution_t sourceRes = m_pAVEncoder->GetVideoInfo()->res_in;
// 			fraction_t yuvFps = m_pAVEncoder->GetVideoInfo()->fps_in;
// 			if(m_pWaterMarkMan) {
// 				m_pWaterMarkMan->SetYUVInfo(sourceRes.width, sourceRes.height, 
// 					(float)yuvFps.num/yuvFps.den);
// 			}
// 		}
// #endif

#ifdef PRODUCT_MEDIACODERDEVICE		// Device only support resolution under 720X576
		resolution_t outRes = m_pAVEncoder->GetVideoInfo()->res_out;
		if(outRes.height > 720 || outRes.width > 1280) initRet = false;
		if(!initRet) logger_err(m_logType,"Target resolution is too large.\n");
#endif

#ifdef DEBUG_DUMP_RAWVIDEO
		if (m_rawvideo_output) {
			video_info_t* pVInfo = m_pAVEncoder->GetVideoInfo();
			char dumpfilename[MAX_PATH];
			std::string destFile = GetOutputFileName(0);
			snprintf(dumpfilename, MAX_PATH, "%s_video_%dx%d_%.1ffps.raw",
				destFile.c_str(), pVInfo->res_in.width, pVInfo->res_in.height, pVInfo->fps_in.num * 1.0 / pVInfo->fps_in.den);
			DeleteFile(dumpfilename);
			m_rawvideo_output->Open(dumpfilename);
		}
#endif
	} while (false);
	
	if(!initRet) SetErrorCode(EC_VIDEO_INIT_ERROR);

#undef FAIL_INFO
	return initRet;
}

void CTransWorkerUnify::benchmark()
{
	if(m_lastTime == 0) {
		m_lastTime = m_startTime;
	}

	int64_t curTime = GetTickCount();
	float elapsedTime = (curTime - m_startTime)/1000.f;
	
	if(m_pAVEncoder->GetEnableVideo()) {
		if(m_encodedFrames%45 == 0) {
			float encodeFps = (m_encodedFrames-m_tmpBenchData.lastFrameNum)*1000.f / (curTime - m_lastTime);
			float averageFps = m_encodedFrames / elapsedTime;
			char fpsStr[16] = {0};
			sprintf(fpsStr, "%1.1f FPS", encodeFps);
			m_strEncodeSpeed = fpsStr;
			logger_info(m_logType, "Worker %d video encode speed: %1.2f(%1.2f), encoded frames: %d.\n", m_id, encodeFps, averageFps, m_encodedFrames);
#ifdef ENABLE_HTTP_STREAMING		// Control streaming speed(not too fast)
			int deltaTime = (int)(m_tmpBenchData.videoEncTime - elapsedTime) - DEFAULT_SEGMENT_DURATION*2;
			deltaTime *= 1000;		// To ms
			if(deltaTime > 0) Sleep(deltaTime);
#endif
			m_progress = m_tmpBenchData.videoEncTime*100000 / m_tmpBenchData.mainDur;
		}
	} else if(m_pAVEncoder->GetEnableAudio()) {
		static int loopCount = 0;
		if(loopCount == 2) {
			float audioSpeedX = m_tmpBenchData.audioEncTime/elapsedTime;
			char speedStr[16] = {0};
			sprintf(speedStr, "%1.1f X", audioSpeedX);
			m_strEncodeSpeed = speedStr;
			logger_info(m_logType, "Worker %d audio encode speed: %1.2fX.\n", m_id, audioSpeedX);
			loopCount = 0;
		}
		loopCount++;
		m_progress = m_tmpBenchData.audioEncTime*100000 / m_tmpBenchData.mainDur;
	}
	
	// Update elapsed time (secs)
	m_elapseSecs = (int)elapsedTime;
	m_lastTime = curTime;
	m_tmpBenchData.lastFrameNum = m_encodedFrames;
#ifndef WIN32	// Sleep a bit under linux
 	Sleep(10);
#endif
}
