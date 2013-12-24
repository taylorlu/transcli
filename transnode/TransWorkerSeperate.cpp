#ifdef WIN32
#include <io.h>
#endif
#include <stdio.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <algorithm>

#include "gain_analysis.h"
#include "bitconfig.h"

#ifdef DEMO_RELEASE
#include "WaterMarkFilter.h"
#endif

#include "MEvaluater.h"
#include "TransWorkerSeperate.h"
#include "StreamOutput.h"

#if defined(HAVE_LIBX264)
#include "CX264Encode.h"
#endif
#if defined(HAVE_LIBX265)
#include "CX265Encode.h"
#endif

#if defined(HAVE_MII_ENCODER)
#include "MiiEncode.h"
#endif

#if defined(HAVE_CAVS)
#include "CAVSVideoEncoder.h"
#endif

#include "XvidEncode.h"

#include "Mp3Encode.h"
#if defined (HAVE_LIBFAAC)
#include "FaacEncode.h"
#endif
#if defined(HAVE_LIBEAC3)
#include "CEac3Encode.h"
#endif

//#include "AudioEncodeFFMpeg.h"

#include "CLIEncoder.h"

#include "MediaSplitter.h"


#include "DecoderMencoder.h"
#include "DecoderMplayer.h"
#include "DecoderVLC.h"
#include "DecoderAVS.h"
#include "DecoderCopy.h"
#include "DecoderFFMpeg.h"

#include "muxers.h"
#include "vfhead.h"

#include "AudioResampler.h"
#include "util.h"
#include "yuvUtil.h"
#include "strpro/StrHelper.h"
#include "PlaylistGenerator.h"
#include "TransnodeUtils.h"
#include "WorkManager.h"
#include "ImageSrc.h"

#ifdef WIN32
#define ENABLE_SSRC
#endif

#define SAFESTRING(str) ((str)?(str):("")) 
#define PROCESS_PAUSE_STOP if(m_taskState >= TASK_DONE) { \
				logger_info(m_logType, "Task cancelled, exit encoding thread.\n"); break; \
		} \
		if(m_taskState == TASK_PAUSED) { \
			Sleep(100);		continue; \
		}
		
#define CHECK_VIDEO_ENCODER(pEncoder) if(!pEncoder->IsRunning()) { \
	        logger_err(m_logType, "Video encoder exit abnormally.\n"); \
			SetErrorCode(EC_VIDEO_ENCODER_ABNORMAL_EXIT); break; \
		}
#define CHECK_AUDIO_ENCODER(pEncoder) if(!pEncoder->IsRunning()) { \
	        logger_err(m_logType, "Audio encoder exit abnormally.\n"); \
			SetErrorCode(EC_AUDIO_ENCODER_ABNORMAL_EXIT); break; \
		}

#ifndef _WIN32
#include <sched.h>
#include <unistd.h>
#endif

#define AV_SYNC_DURATION 15000
#define AV_SYNC_SLEEPMS 100

CTransWorkerSeperate::CTransWorkerSeperate(): 
    m_pAudioDec(NULL), m_pVideoDec(NULL), m_audioDecType(AD_MENCODER), m_videoDecType(VD_MENCODER),
	m_pImgTail(NULL),m_pSplitter(NULL), m_muxingCount(0), m_lockReadAudio(0), m_lockReadVideo(0),
	m_bDecodeNext(false),  m_encoderPass(1), m_bCopyAudio(false),
	m_bCopyVideo(false), m_bMultiAudioTrack(false)
{
	m_logType = LOGM_TS_WK_SEP;
	m_pCopyAudioInfo = NULL;
	m_pCopyVideoInfo = NULL;
}


CTransWorkerSeperate::~CTransWorkerSeperate(void)
{
	//printf("%d\n", m_id);
	CleanUp();
}

bool CTransWorkerSeperate::setAudioPref(CXMLPref* prefs)
{
	if(!prefs) return false;
	if(!prefs->GetBoolean("overall.audio.enabled")) {	// Ignore audio stream
		logger_info(m_logType, "audio is disabled\n");
		return true;
	}

	int chEnum = prefs->GetInt("overall.audio.channels");
	int channelNumArray[7] = {0, 1, 1, 2, 4, 5, 6};
	int outChannelNum = channelNumArray[chEnum];
	if(prefs->GetInt("audiofilter.channels.mix") == 2) {	// Mix to mono
		outChannelNum = 1;
	}

	audio_format_t aFormat = (audio_format_t)prefs->GetInt("overall.audio.format");
	audio_encoder_t encType = (audio_encoder_t)prefs->GetInt("overall.audio.encoder");
	int outSampleRate = prefs->GetInt("audiofilter.resample.samplerate");
	if(outSampleRate < 0) outSampleRate = 0;
	
	audio_info_t audioInfo;
	memset(&audioInfo, 0, sizeof(audioInfo));
	audioInfo.encoder_type = encType;
	audioInfo.format = aFormat;
	audioInfo.srcformat = AC_PCM;
	audioInfo.afType = AF_DECODER;
	audioInfo.out_srate = outSampleRate;
	audioInfo.out_channels = outChannelNum;
	audioInfo.aid = -1;
	
	CAudioEncoder* pEncoder = createAudioEncoder(encType, aFormat);
	if(!pEncoder) {
		logger_err(m_logType, "Create audio encoder failed.\n");
		SetErrorCode(EC_INVALID_PREFS);
		return false;
	}
	pEncoder->SetAudioInfo(audioInfo, prefs);
	// Save audio stream file and info
	CFileQueue::queue_item_t fileItem = {pEncoder->GetOutFileName(), ST_AUDIO,NULL,pEncoder->GetAudioInfo()};
	m_streamFiles.Enqueue(fileItem);

	return true;
}

bool CTransWorkerSeperate::setVideoPref(CXMLPref* prefs)
{
	if(!prefs) return false;
	if(!prefs->GetBoolean("overall.video.enabled")) {	// Ignore video stream
		logger_info(m_logType, "Video is disabled\n");
		return true;
	}

	int filterType = prefs->GetInt("videofilter.generic.applicator");
	if(filterType < 0) filterType = 0;
	vf_type_t vfType = (vf_type_t)(filterType);
	
	resolution_t outSize = {0, 0};
	fraction_t outFps = {0, 0}, outDar = {0, 0}, outPar = {0, 0};

	int rateCtrlMode = prefs->GetInt("overall.video.mode");
	if(rateCtrlMode >= RC_MODE_2PASS) m_encoderPass = 2/*rateCtrlMode -1*/;
	int aspectType = prefs->GetInt("overall.video.ar");
	if(aspectType == 2) {
		outDar.num = prefs->GetInt("overall.video.arNum");
		outDar.den = prefs->GetInt("overall.video.arDen");
	} else if(aspectType == 3) {
		outPar.num = prefs->GetInt("overall.video.arNum");
		outPar.den = prefs->GetInt("overall.video.arDen");
	}

	video_format_t  encFormat = (video_format_t)prefs->GetInt("overall.video.format");
	video_encoder_t encType = (video_encoder_t)prefs->GetInt("overall.video.encoder");
	
	if(encType == 0 && prefs->GetBoolean("overall.video.autoEncoder")) {
		switch(encFormat) {
			case VC_XVID : encType = VE_XVID; break;
			case VC_H264:  encType = VE_X264; break;
			default:	   encType = VE_FFMPEG; break;
		}
	}

	if(prefs->GetBoolean("videofilter.scale.enabled")) {
		outSize.width = prefs->GetInt("videofilter.scale.width");
		outSize.height = prefs->GetInt("videofilter.scale.height");
	}
	if(outSize.width > 0 && outSize.height > 0 && outDar.num <= 0 && outPar.num > 0) {
		CAspectRatio aspectConvert(outSize.width, outSize.height);
		aspectConvert.SetPAR(outPar.num, outPar.den);
		aspectConvert.GetDAR(&(outDar.num), &(outDar.den));
	}

	if(prefs->GetBoolean("videofilter.frame.enabled")) {
		outFps.num = prefs->GetInt("videofilter.frame.fps");
		outFps.den = prefs->GetInt("videofilter.frame.fpsScale");
	}

	if(CTransnodeUtils::GetUseSingleCore()) {	// Use single cpu core to encode
		prefs->SetInt("videoenc.x264.threads", 1);
		prefs->SetInt("videoenc.x265.threads", 1);
		prefs->SetInt("videoenc.xvid.threads", 1);
	}

	rawvideo_format_t rawFormat = RAW_I420;
	int videoBits = 12;
	if(encType == VE_VFW) {
		rawFormat = RAW_RGB24;
		videoBits = 24;
	}

	video_info_t videoInfo = {CF_DEFAULT, encType, encFormat, vfType, outSize, {0, 0}, outFps, {0, 0},
		outPar, {0,0}, outDar, 0, 0, rawFormat, videoBits, 0, 0, 0};
	
	CVideoEncoder* pEncoder = createVideoEncoder(encType, encFormat, vfType);
	if(!pEncoder) {
		if(encType == VE_CUDA264) {
			encType = VE_X264;
		}
		pEncoder = createVideoEncoder(encType, encFormat, vfType);

		if(!pEncoder) {
			logger_err(m_logType, "Create video encoder failed.\n");
			SetErrorCode(EC_INVALID_PREFS);
			return false;
		}
	}
	pEncoder->SetVideoInfo(videoInfo, prefs);

	// Save video stream file and info
	CFileQueue::queue_item_t fileItem = {pEncoder->GetOutFileName(), ST_VIDEO, pEncoder->GetVideoInfo(), NULL};
	m_streamFiles.Enqueue(fileItem);

	return true;
}

bool CTransWorkerSeperate::setMuxerPref(CXMLPref* prefs)
{
	int muxerType = prefs->GetInt("overall.container.muxer");
	if (muxerType == PREF_ERR_NO_INTNODE) muxerType = 0;
	//If video is h263 and container is 3gp, use ffmpeg to mux
	//if(prefs->GetInt("overall.video.format") == VC_H263 && muxerType == MUX_MP4) {
	//	muxerType = MUX_FFMPEG;
	//}
	CMuxer* pMuxer = CMuxerFactory::CreateInstance(muxerType);
	if(!pMuxer) {
		logger_err(m_logType, "Create Muxer failed.\n");
		SetErrorCode(EC_INVALID_PREFS);
		return false;
	}
	pMuxer->SetPref(prefs);
	pMuxer->SetFileQueue(&m_streamFiles);
	const char* srcFile = m_streamFiles.GetFirstSrcFile();
	char* strSrcDir = NULL;
	if(!strstr(srcFile, "://")) {		// it's dvd/vcd track or stream protocol
		strSrcDir = _strdup(srcFile);
		char* lastDelim = strrchr(strSrcDir, PATH_DELIMITER);
		if(lastDelim) *lastDelim = NULL;
	}
	
	size_t outFileIdx = m_streamFiles.GetDestFiles().size()+1;
	const char* fileTitle = m_pRootPref->GetSrcFileTitle();
	std::string destFileName = pMuxer->GenDestFileName(fileTitle, outFileIdx, strSrcDir);

	if(strSrcDir) free(strSrcDir);
	if(!destFileName.empty()) {
		m_streamFiles.AddDestFile(destFileName);
		m_muxers.push_back(pMuxer);
		logger_info(m_logType, "Dest file:%s.\n", destFileName.c_str());
	} else {
		logger_err(m_logType, "Generate destinate file failed.\n");
		return false;
	}

	return true;
}

bool CTransWorkerSeperate::setSourceAVInfo(StrPro::CXML2* mediaInfo)
{
	initAVSrcAttrib(mediaInfo);
	
	const char* srcFile = m_streamFiles.GetFirstSrcFile();
	if(srcFile && strstr(srcFile, "://")) return true;
	if(m_srcAudioAttribs.empty() && !m_srcVideoAttrib) {
		/*const char* ext = strrchr(srcFile, '.');
		if(ext && (_stricmp(ext, ".mpg") == 0 ||		// Hik format, media info can't recognize
			_stricmp(ext, ".264") == 0 || _stricmp(ext, ".mp4") == 0 ||
			_stricmp(ext, ".iso") == 0 || _stricmp(ext, ".rmvb") == 0 ||
			_stricmp(ext, ".rm") == 0) ) {
			return true; 
		}*/
		return false;
	}

	bool invalidAudio = false;
	bool invalidVideo = false;
	attr_audio_t* audioAttrib = NULL;
	if(!m_srcAudioAttribs.empty()) audioAttrib = m_srcAudioAttribs[0];
	if(!audioAttrib || (audioAttrib->id < 0 && audioAttrib->duration <= 0 && 
	audioAttrib->samplerate <= 0 && audioAttrib->channels <= 0 && audioAttrib->bitrate <= 0)) {
		logger_err(m_logType, "Invalid Audio Attrib, Clean up all audio encoder!\n");
		cleanAudioEncoders();
		m_streamFiles.ClearAudioFiles();
		invalidAudio = true;
	}
	if(!m_srcVideoAttrib || (m_srcVideoAttrib->id < 0 && m_srcVideoAttrib->width <= 0 && 
		m_srcVideoAttrib->duration <= 0 && m_srcVideoAttrib->bitrate <= 0)) {
		logger_err(m_logType, "Invalid Video Attrib, Clean up all video encoder!\n");
		cleanVideoEncoders();
		m_streamFiles.ClearVideoFiles();
		invalidVideo = true;
	}

	if(invalidAudio && invalidVideo) return false;
	
	return true;
}

void CTransWorkerSeperate::initAudioFractionOfSecond()
{
	if(m_videoEncs.empty()) return;
	video_info_t* pVInfo = m_videoEncs[0]->GetVideoInfo();
	fraction_t fpsIn = pVInfo->fps_in;
	fraction_t fpsOut = pVInfo->fps_out;
	if (fpsIn.num <= 0 || fpsIn.den <= 0) {
		fpsIn.num = 25;
		fpsIn.den = 1;
	}
	if((pVInfo->vfType==VF_ENCODER) && fpsOut.num > 0) {
		m_audioFractionOfSecond = (int)((double)fpsOut.num/(double)fpsOut.den+0.5);
	} else {
		m_audioFractionOfSecond = (int)((double)fpsIn.num/(double)fpsIn.den+0.5);
	}
	m_audioFractionOfSecond *= 2;
}

void CTransWorkerSeperate::setDestNameForVideo(CVideoEncoder* pVideoEnc, size_t encIdx)
{
	if(!pVideoEnc) return;
	//Find muxer index according to video index
	size_t muxerIdx = 0;
	for(; muxerIdx<m_muxers.size(); ++muxerIdx) {
		CMuxer* pmuxer = m_muxers.at(muxerIdx);
		if(pmuxer->ContainStream(ST_VIDEO, encIdx)) break;
	}
	// Get dest file name and set to video encoder
	std::string destFile = GetOutputFileName(muxerIdx);
	pVideoEnc->SetDestFileName(destFile.c_str());
}

bool CTransWorkerSeperate::initialize()
{
	const char* srcFile = m_streamFiles.GetCurSrcFile();
	if(!srcFile || *srcFile == '\0') {
		SetErrorCode(EC_INIT_ERROR);
		logger_err(m_logType,"No Input file.\n");
		return false;
	}
	//================== Set decoder command and start decoder =====================
	bool success = true;
	int64_t srcFileSize = StatFileSize(srcFile)/1024;	// kb
	std::string srcSizeStr = FormatFileSize(srcFileSize);
	logger_info(m_logType, "Transcode %s(File size:%s).\n", srcFile, srcSizeStr.c_str()); 
	int overallBr = 0;		// Kbps
	if(m_tmpBenchData.mainDur > 0 && m_tmpBenchData.mainDur != INT_MAX) {
		overallBr = (int)(srcFileSize / (float)m_tmpBenchData.mainDur * 8000);
	}

	if(m_bCopyVideo && m_bCopyAudio) return true;

	#define FAIL_INFO(err) logger_err(m_logType, err); success=false; break;
	do {
		if(!m_bCopyVideo && m_videoEncs.empty()) {		// Convert music only
			m_encoderPass = 1;		// if no video, no need to perform 2 pass
			m_streamFiles.ClearDestFiles();
		
			// Reset muxer -- Create dummy muxer(Output music)
			if(!m_audioEncs.empty() && !resetMuxersForMusic()) {
				FAIL_INFO("Reset dummy muxer for music failed.\n");
			}
		}
		// If there is no audio encoders, disable replay gain analyse
		if(m_audioEncs.empty()) {
			m_bAutoVolumeGain = false;
		}

		// Perform auto-detection of black band
		if(!m_videoEncs.empty()) {
			performBlackBandAutoDetect(srcFile, m_videoEncs[0]);
		}

		for (size_t i = 0; i < m_videoEncs.size(); i++) {
			CVideoEncoder* pVideoEnc = m_videoEncs.at(i);
			if(pVideoEnc && pVideoEnc->GetVideoFilter() == NULL) {
				setDestNameForVideo(pVideoEnc, i);
				calcDarWhenCropAndNormalizeCrop(pVideoEnc);
				// Adjust video out resolution(width or height is -1)
				adjustVideoOutParam(pVideoEnc, overallBr);
				normalizeLogoTimeRect(pVideoEnc);
				normalizeDelogoTimeRect(pVideoEnc);
				success = createVideoFilter(pVideoEnc);
				if(!success) {
					SetErrorCode(EC_VIDEO_ENCODER_ERROR);
					FAIL_INFO("Failed to create video filter.\n");
				}
			}

			// Set multiple pass
			if(m_encoderPass > 1 && !pVideoEnc->GetIsMultiPass()) {
				pVideoEnc->SetEncodePass(m_encoderPass);
				pVideoEnc->SetPassLogFile(m_streamFiles.GetEncoderStatFile(i));
			}
		}
		if(!success) break;

		// Set audio time counter accuracy same as video
		initAudioFractionOfSecond();

		decideAfType();
		
	#ifdef DEMO_RELEASE
		// Enable transerver demo watermark
		if(!m_videoEncs.empty() && !m_pWaterMarkMan) {
			m_pWaterMarkMan = new CWaterMarkManager();
			if(m_pWaterMarkMan) {
				std::vector<const char*> vctFiles;
				//vctFiles.push_back("e:\\logo0000.tga");
				//const char* logoFloder = "e:\\logoTest";
				CWaterMarkFilter* pWaterMark = m_pWaterMarkMan->AddWaterMark(true);
				pWaterMark->AddShowTimeAndPosition(0, -1);
				//pWaterMark->AddShowTimeAndPosition(12000, 30000, WATERMARK_LU);
				//pWaterMark->AddShowTimeAndPosition(30000, 90000, WATERMARK_CEN);
				pWaterMark->SetAlpha(0.6f);
				pWaterMark->EnableColor(true);
				//pWaterMark->SetInterval(100);
				//m_pWaterMark->DumpImageToText("d:\\logo.txt");
				//m_pWaterMark->SetYUVFrameSize(pVInfo->res_in.width, pVInfo->res_in.height);
			}
		}
	#endif

		m_bDecodeNext = m_streamFiles.GetSrcFileCount() > 1 ? true : false;
	
		if(m_audioEncs.empty() && m_videoEncs.empty() && !m_bCopyVideo && !m_bCopyAudio) {
			SetErrorCode(EC_INIT_ERROR);
			FAIL_INFO("Failed to initialize, no any audio/video encoder.\n");
		}

		// Launch decoder (mplayer or mencoder)
		if (!startDecoder(srcFile)) {
			SetErrorCode(EC_DECODER_ERROR);
			FAIL_INFO("Failed to initialize, failed to decode!\n");
		}

		// Init resources in 2nd pass or ABR mode.
		if(!initResourceInOnePassOrLastPass(srcFile)) {
			success = false; break;
		}

		logger_status(m_logType, "TransWorker is initialized successfully.\n");
	} while (false);
#undef FAIL_INFO
	return success;
}

bool CTransWorkerSeperate::initResourceInOnePassOrLastPass(const char* srcFile)
{
	// Init splitter (Only support single audio and video stream)
	if(m_encoderPass == 1 && m_pSplitter) {		// Only 1 pass or last pass consider split file
		m_pSplitter->BindWorker(this);
		m_pSplitter->BindFileQueue(&m_streamFiles);
		if(!m_audioEncs.empty()) m_pSplitter->BindAudioEncoder(m_audioEncs[0]);
		if(!m_videoEncs.empty()) m_pSplitter->BindVideoEncoder(m_videoEncs[0]);
		if(!m_muxers.empty()) m_pSplitter->SetMuxInfoSizeRatio(m_muxers[0]->GetMuxInfoSizeRatio());
	}

	// If analyse replay gain, aux decoders should be started at first pass when 2-pass encoding
	if((m_encoderPass == 1 || m_bAutoVolumeGain) && m_bMultiAudioTrack) {
		if (!startAuxDecoders(srcFile)) {
			SetErrorCode(EC_DECODER_ERROR);
			logger_err(m_logType, "Start multiAudio decoder failed!\n");
			return false;
		}
	}
	return true;
}

void CTransWorkerSeperate::decideAfType() 
{
	bool bSameOutSampleRate = true;		// Test if out samplerate of all audios are the same
	m_bEnableDecoderAf = true;
	for (size_t i=0; i<m_audioEncs.size(); ++i) {
		CAudioEncoder* pAudioEnc = m_audioEncs.at(i);
		if(pAudioEnc) {
			int outSampleRate = pAudioEnc->GetAudioInfo()->out_srate;
			if (m_audioEncs.size() > 1) {
				if(i<m_audioEncs.size()-1) {
					CAudioEncoder* nextEnc = m_audioEncs.at(i+1);
					bSameOutSampleRate = bSameOutSampleRate && 
						(outSampleRate == nextEnc->GetAudioInfo()->out_srate);
				}
			} else {
				if(outSampleRate != 0) {	// Change samplerate
					pAudioEnc->GetAudioInfo()->afType = AF_DECODER;
				} 
				if(m_audioDecType == VD_VLC) {	// VLC hasn't audio filter
					pAudioEnc->GetAudioInfo()->afType = AF_CPU;
					m_bEnableDecoderAf = false;
				}
			}
		}
	}

	if(!m_bMultiAudioTrack && !bSameOutSampleRate && m_audioEncs.size() > 1) {
		for (size_t i=0; i<m_audioEncs.size(); ++i) {
			m_audioEncs.at(i)->GetAudioInfo()->afType = AF_CPU;
			m_bEnableDecoderAf = false;
		}
	}
}

bool CTransWorkerSeperate::resetMuxersForMusic()
{
	std::vector<CXMLPref*> muxerPrefs;
	if(!m_muxers.empty()) {
		for (size_t i=0; i<m_muxers.size(); ++i) {
			CXMLPref* pMuxerPref = m_muxers[i]->GetPref();
			//audio_format_t aFormat = (audio_format_t)pMuxerPref->GetInt("overall.audio.format");
			if(pMuxerPref) {
				pMuxerPref->SetInt("overall.container.muxer", MUX_DUMMY);
				muxerPrefs.push_back(pMuxerPref);
			}
		}
	} else {
		CXMLPref* pMuxerPref = m_audioEncs[0]->GetAudioPref();
		if(pMuxerPref) {
			pMuxerPref->SetInt("overall.container.muxer", MUX_DUMMY);
			muxerPrefs.push_back(pMuxerPref);
		}
	}

	cleanMuxers();
	if(muxerPrefs.empty()) {
		return false;
	}
	for(size_t i=0; i<muxerPrefs.size(); ++i) {
		setMuxerPref(muxerPrefs[i]);
	}

	return true;
}

bool CTransWorkerSeperate::setDecoderParam(video_info_t* pVInfo, CXMLPref* pVideoPref,
	audio_info_t* pAInfo, CXMLPref* pAudioPref)
{
	if(m_pVideoDec) {
		m_pVideoDec->BindWorkerId(m_id);
		m_pVideoDec->BindStreamFiles(&m_streamFiles);
		if(pVInfo) m_pVideoDec->BindVideoInfo(pVInfo);
		if(pVideoPref) m_pVideoDec->BindVideoPref(pVideoPref);
		m_pVideoDec->SetEnableVf(m_bEnableDecoderVf);
		m_pVideoDec->SetEnableVideo(true);
	}

	if(m_pAudioDec) {
		m_pAudioDec->BindWorkerId(m_id);
		m_pAudioDec->BindStreamFiles(&m_streamFiles);
		m_pAudioDec->SetEnableAudio(true);
		// Enable audio decode in both passes, 
		// ensure each pass get same video frames(consider a/v sync of decoder)
		if(pAInfo) {
			m_pAudioDec->BindAudioInfo(pAInfo);
			m_pAudioDec->SetEanbleAf(m_bEnableDecoderAf);
		}
		if(pAudioPref) m_pAudioDec->BindAudioPref(pAudioPref);
	}
	
	return true;
}

CDecoder* CTransWorkerSeperate::createAudioDecoder(audio_decoder_t decType)
{
	switch(decType) {
	case AD_MPLAYER: return (new CDecoderMplayer);
	case AD_AVS: return (new CDecoderAVS);
	//case AD_VLC: return (new CDecoderVLC);
	case AD_FFMPEG: return (new CDecoderFFMpeg);
	default: return (new CDecoderMencoder);
	}
}

CDecoder* CTransWorkerSeperate::createVideoDecoder(video_decoder_t decType)
{
	switch(decType) {
	case VD_MPLAYER: return (new CDecoderMplayer);
	case VD_AVS: return (new CDecoderAVS);
	//case VD_VLC: return (new CDecoderVLC);
	case VD_FFMPEG: return (new CDecoderFFMpeg);
//#ifdef WIN32
//	case VD_HKV: return (new CDecoderHik);
//#endif
	default: return (new CDecoderMencoder);
	}
}

bool CTransWorkerSeperate::startDecoder(const char* srcFileName)
{
	// Get audio & video prefs and video info
	video_info_t* pVInfo = NULL;
	CXMLPref*     pVideoPref = NULL;
	audio_info_t* pAInfo = NULL;
	CXMLPref*     pAudioPref = NULL;

	if(!m_videoEncs.empty()) {
		CVideoEncoder* pEncoder = m_videoEncs.at(0);
		pVideoPref = pEncoder->GetVideoPref();
		pVInfo = pEncoder->GetVideoInfo();

		if(pVideoPref) {
			int srcType = pVideoPref->GetInt("overall.video.source");
			if(srcType >= 0) {
				m_videoDecType = (video_decoder_t)(srcType);
			}
		}
		if (strncmp(srcFileName, "dshow://", 8) == 0) {
			logger_info(m_logType, "Input is dshow, set the decoder to VLC\n");
			m_videoDecType = VD_VLC;
		}
	}

	if(!m_audioEncs.empty()) {
		CAudioEncoder* pEncoder = m_audioEncs.at(0);
		pAudioPref = pEncoder->GetAudioPref();
		pAInfo = pEncoder->GetAudioInfo();

		if(pAudioPref) {
			int srcType = pAudioPref->GetInt("overall.audio.source");
			if(srcType >= 0) {
				m_audioDecType = (audio_decoder_t)(srcType);
			}
		}
	}

	// Auto select suitable decoders
	CXMLPref* pPref = pVideoPref;
	if(!pPref) pPref = pAudioPref;
	if(pPref && pPref->GetBoolean("overall.video.autoSource")) {
		autoSelectBestDecoder2(srcFileName, pVideoPref, pAudioPref);
	}

	// If 1-pass encoding and analyse volume gain, only decode audio
	if(m_bAutoVolumeGain && m_encoderPass == 1 && !m_videoEncs.empty()) {
		m_pAudioDec = createAudioDecoder(AD_FFMPEG);
		setDecoderParam(pVInfo, pVideoPref, pAInfo, pAudioPref);
		return m_pAudioDec->Start(srcFileName);
	}

	if(((int)m_audioDecType == (int)m_videoDecType)) {
		if(!m_audioEncs.empty()) {
			m_pAudioDec = createAudioDecoder(m_audioDecType);
			if(!m_videoEncs.empty()) m_pVideoDec = m_pAudioDec;
		} else if(!m_videoEncs.empty()) {
			m_pVideoDec = createVideoDecoder(m_videoDecType);
			if(!m_audioEncs.empty()) m_pAudioDec = m_pVideoDec;
		}
	} else {
		if(!m_audioEncs.empty()) {
			m_pAudioDec = createAudioDecoder(m_audioDecType);
		}
		if(!m_videoEncs.empty()) {
			m_pVideoDec = createVideoDecoder(m_videoDecType);
		}
	}
	
	//std::string tmpSrcFile = m_streamFiles.GetTempSrcFile(srcFileName);
	bool success = setDecoderParam(pVInfo, pVideoPref, pAInfo, pAudioPref);
	if(m_pAudioDec == m_pVideoDec) {
		if(m_pVideoDec) {
			success = m_pAudioDec->Start(srcFileName);
		}
	} else {
		if(m_pAudioDec) {
			success = m_pAudioDec->Start(srcFileName);
		}
		if(m_pVideoDec) {
			success = m_pVideoDec->Start(srcFileName);
		}
	}

	if(!success && (m_pAudioDec == m_pVideoDec) && m_pVideoDec) {	// Try another decoder once
		logger_warn(m_logType, "First decoder failed, try another one.\n");
		delete m_pAudioDec;
		if(m_videoDecType == VD_FFMPEG) {
			m_videoDecType = VD_MENCODER;
			m_audioDecType = AD_MENCODER;
			m_pAudioDec = new CDecoderMencoder;
			m_pVideoDec = m_pAudioDec;
		} else {
			m_videoDecType = VD_FFMPEG;
			m_audioDecType = AD_FFMPEG;
			m_pAudioDec = new CDecoderFFMpeg;
			m_pVideoDec = m_pAudioDec;
		}
		if(!setDecoderParam(pVInfo, pVideoPref, pAInfo, pAudioPref)) return false;
		success = m_pAudioDec->Start(srcFileName);
	}
	
	return success;
}

bool CTransWorkerSeperate::startAuxDecoders(const char* srcFileName)
{
	bool auxDecoderCreated = m_auxDecoders.size() > 0;
	if(m_audioEncs.size() > 1) {
		for (size_t i=1; i<m_audioEncs.size(); ++i) {
			CAudioEncoder* pAudioEnc = m_audioEncs[i];
			if (pAudioEnc && pAudioEnc->GetSourceIndex() > 0) {
				CDecoder* pTmpDecoder = NULL;
				if(auxDecoderCreated) {
					pTmpDecoder = m_auxDecoders[i-1];
				} else {
					pTmpDecoder = new CDecoderFFMpeg;
					m_auxDecoders.push_back(pTmpDecoder);
				}
				pTmpDecoder->BindWorkerId(m_id);
				pTmpDecoder->SetEnableAudio(true);
				pTmpDecoder->SetEnableVideo(false);
				pTmpDecoder->BindAudioInfo(pAudioEnc->GetAudioInfo());
				pTmpDecoder->SetEanbleAf(true);
				pTmpDecoder->BindAudioPref(pAudioEnc->GetAudioPref());
				pTmpDecoder->BindStreamFiles(&m_streamFiles);
				
				std::string tmpSrcFile = m_streamFiles.GetTempSrcFile(srcFileName);
				
				bool success = pTmpDecoder->Start(tmpSrcFile.c_str());
				/*int pid = pTmpDecoder->GetCLIPid();
				if (pid > 0) m_CLIPids.push_back(pid);*/

				if (!success) return false;
			}
		}
	}
	return true;
}

bool CTransWorkerSeperate::initAudioEncoders()
{
	bool initRet = true;
#define FAIL_INFO(err) logger_err(m_logType, err); initRet=false; break;

	logger_status(m_logType, "To init Audio Encoders\n");

	if (m_pAudioDec->GetAudioReadHandle() < 0) return false;
	do {
		// Init main decoder
		initRet = m_pAudioDec->ReadAudioHeader();
		if(!initRet) {
			m_streamFiles.ClearAudioFiles();	// Audio streams not existed
			cleanAudioEncoders();				// Clean audio encoders
			if(m_pSplitter) {
				m_pSplitter->BindAudioEncoder(NULL);
			}
			m_pAudioDec->CloseAudioReadHandle();
			FAIL_INFO("Can not get enough audio info, no audio output.\n");
		}
	
		wav_info_t* pWav = m_pAudioDec->GetWavInfo();
		//printf("Bits:%d, Ch:%d, SR:%d\n", pWav->bits, pWav->channels, pWav->sample_rate);
		if (!pWav || pWav->bits < 8 || pWav->channels < 1 || pWav->sample_rate < 4000) {
			FAIL_INFO("Invalid audio properties.\n");
		}
		audio_info_t* pAudioInfo = NULL;
		for (size_t i=0; i<m_audioEncs.size(); ++i) {
			wav_info_t* pTmpWav = pWav;
			CAudioEncoder* pEncoder = m_audioEncs.at(i);
			if(!pEncoder) continue;
			pAudioInfo = pEncoder->GetAudioInfo();
			int decoderIdx = pEncoder->GetSourceIndex();
			if(decoderIdx > 0) {	// Use aux decoder
				CDecoder* pDecoder = m_auxDecoders[decoderIdx-1];
				if(pDecoder->GetAudioReadHandle()>0 && pDecoder->ReadAudioHeader()) {
					pTmpWav = pDecoder->GetWavInfo();
				} else {
					FAIL_INFO("Start aux decoder failed.\n");
				}
			} 
			checkAudioParam(pAudioInfo, pTmpWav);
			bool success = pEncoder->Initialize();
			if (!success) {
				FAIL_INFO("Audio stream init failed.\n");
			}	

			// Init Splitter's overall bits
			if(m_pSplitter) {
				if(m_videoEncs.empty()) {
					m_pSplitter->SetOverallKBits(pEncoder->GetBitrate());
				} else if( m_videoEncs[0]->GetBitrate()>0) {
					int overallKBits = pEncoder->GetBitrate() + m_videoEncs[0]->GetBitrate();
					m_pSplitter->SetOverallKBits(overallKBits);
				}
			}
		}
			
		if(!initRet) break;

		// calculate raw audio buffer size(1 second audio sample data)
		if(!m_pcmBuf) {
			if(m_audioEncs[0]) {
				m_pcmBufSize = m_audioEncs[0]->GetFrameLen();
			}
			if(m_pcmBufSize <= 0) {
				const int sampleBytes = (pWav->bits>>3)*pWav->channels;
				pWav->bytes_per_second = sampleBytes*pWav->sample_rate;	// 1 second
				if(pWav->bytes_per_second == 0) {
					initRet = false;
					FAIL_INFO("Invalid audio source data, bytes per second is 0.\n");
				}
				m_pcmBufSize = pWav->bytes_per_second/m_audioFractionOfSecond;
				EnsureMultipleOfDivisor(m_pcmBufSize, sampleBytes);
			}
			m_pcmBuf = (uint8_t*)malloc(m_pcmBufSize);
		}

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

bool CTransWorkerSeperate::initVideoEncoders()
{
#define FAIL_INFO(err) logger_err(m_logType, err); initRet=false; break;
	bool initRet = true;
	logger_status(m_logType, "To init Video Encoders.\n");
	do {
		if (m_videoDecType != VD_FFMPEG) {
			initRet = m_pVideoDec->ReadVideoHeader();
			if(!initRet) {
				m_streamFiles.ClearVideoFiles();	// Video streams not existed
				cleanVideoEncoders();
				FAIL_INFO("Can not get enough video info.\n");
			}
		}

		CVideoEncoder* pEncoder = NULL;
		for (size_t i=0; i<m_videoEncs.size(); ++i) {
			pEncoder = m_videoEncs.at(i);
			video_info_t* pVInfo = pEncoder->GetVideoInfo();
			CXMLPref* pPref = pEncoder->GetVideoPref();
			yuv_info_t* pYuv = m_pVideoDec->GetYuvInfo();
			if(m_videoDecType != VD_FFMPEG) {
				if (!pYuv || pYuv->width <= 0 || pYuv->height <= 0) {
					FAIL_INFO("Invalid video properties.\n");
				}
			} else {
				if(pVInfo->res_out.width > 0 || pVInfo->res_out.height > 0) {
					pYuv->width = pVInfo->res_out.width;
					pYuv->height = pVInfo->res_out.height;
				} else if(pVInfo->res_in.width > 0 || pVInfo->res_in.height > 0) {
					pYuv->width = pVInfo->res_in.width;
					pYuv->height = pVInfo->res_in.height;
				}
			}

			checkVideoParam(pVInfo, pYuv);
			if(m_tmpBenchData.fpsNum <= 0) {
				m_tmpBenchData.fpsNum = pVInfo->fps_out.num;
				m_tmpBenchData.fpsDen = pVInfo->fps_out.den;
				if(pVInfo->vfType == VF_NONE || pVInfo->vfType == VF_ENCODER){
					m_tmpBenchData.fpsNum = pVInfo->fps_in.num;
					m_tmpBenchData.fpsDen = pVInfo->fps_in.den;
				}
			}
			
			// When cpu core num is bigger than 4, then set threads to core num
			int coreNum = CWorkManager::GetInstance()->GetCPUCoreNum();
			if(pVInfo->encoder_type == VE_X264) {
				if(m_encoderPass > 1) {		// First pass
					if(coreNum >= 8) {		// First pass is fast preset, when threads is 6, better
						coreNum = 8;
					} 
				} else {	// One pass or second pass
					if(coreNum < 12) {
						coreNum = coreNum*3/2;
					}
				}
				pPref->SetInt("videoenc.x264.threads", coreNum);
			} else if(pVInfo->encoder_type == VE_XVID){
				if(coreNum >= 4) coreNum = 4;
				pPref->SetInt("videoenc.xvid.threads", coreNum);
			}

			initRet = pEncoder->Initialize();
			// if CUDA 264 init failed, downgrade to x264
			if(!initRet && pVInfo->encoder_type == VE_CUDA264) {	
				CVideoFilter* pFilter = pEncoder->GetVideoFilter();
				//CStreamOutput* pStreamOut = pEncoder->GetOutputStream();
                #if defined(HAVE_MINDERIN_ENCODER) //minderin encoder
			    CVideoEncoder* tempX264Enc = new CMindCLIEncoder(pEncoder->GetOutFileName().c_str());
                #elif defined(HAVE_LIBX264) //libx264
			    CVideoEncoder* tempX264Enc = new CX264Encode(pEncoder->GetOutFileName().c_str());
				#else //x264 cli
			    CVideoEncoder* tempX264Enc = new CX264CLIEncoder(pEncoder->GetOutFileName().c_str());
                #endif
                tempX264Enc->SetVideoInfo(*pVInfo, pPref);
				if(pFilter) tempX264Enc->SetVideoFilter(pFilter);
				initRet = tempX264Enc->Initialize();
				delete pEncoder;
				pEncoder = tempX264Enc;
				m_videoEncs[i] = tempX264Enc;
				m_bUseCudaEncoder = false;
				logger_warn(m_logType, "Video stream %i init CUDA264 failed, downgrade to h264.\n", (int)i);
			    if(!initRet) break;
			}	

			// Init video filters
			if(!pEncoder->GetIsMultiPass() || m_encoderPass == 2) {
				if(!initVideoFilter(pEncoder)) {
					FAIL_INFO("Init video filter failed.\n");
				}
			}

			// Init Splitter's overall bits
			if(m_pSplitter && m_encoderPass == 1) {
				int videoKbits = pEncoder->GetBitrate();
				if(m_audioEncs.empty()) {
					m_pSplitter->SetOverallKBits(videoKbits);
				} else if( m_audioEncs[0]->GetBitrate()>0) {
					int overallKBits = videoKbits + m_audioEncs[0]->GetBitrate();
					m_pSplitter->SetOverallKBits(overallKBits);
				}
				m_pSplitter->SetVideoFrameBytes(videoKbits*125*m_tmpBenchData.fpsDen/m_tmpBenchData.fpsNum);
			}

			if(!initRet) {
				logger_err(m_logType, "Video stream %d init failed.\n", (int)i);
				break;
			}
		}
		if(!initRet) break;

		if(!m_yuvBuf) {
			m_frameBufSize = pEncoder->GetFrameSize();
			m_yuvBuf = (uint8_t*)malloc(m_frameBufSize);
			if(m_pImgTail) {	// Only init once when 2-pass mode
				fraction_t dar = pEncoder->GetVideoInfo()->dest_dar;
				resolution_t res = pEncoder->GetVideoInfo()->res_out;
				fraction_t fps = pEncoder->GetVideoInfo()->fps_out;
				m_pImgTail->SetDestDar(dar.num, dar.den);
				m_pImgTail->SetDestYuvSize(res.width, res.height);
				m_pImgTail->SetDestFps(fps.num, fps.den);
				if(!m_pImgTail->Initialize()) {
					delete m_pImgTail;
					m_pImgTail = NULL;
				}
			}
		}

#ifdef DEMO_RELEASE
		resolution_t sourceRes = pEncoder->GetVideoInfo()->res_in;
		fraction_t yuvFps = pEncoder->GetVideoInfo()->fps_in;
		if(m_pWaterMarkMan) {
			m_pWaterMarkMan->SetYUVInfo(sourceRes.width, sourceRes.height, 
				(float)yuvFps.num/yuvFps.den);
		}
#endif

#ifdef PRODUCT_MEDIACODERDEVICE		// Device only support resolution under 720X576
		resolution_t outRes = pEncoder->GetVideoInfo()->res_out;
		if(outRes.height > 720 || outRes.width > 1280) initRet = false;
		if(!initRet) logger_err(m_logType,"Target resolution is too large.\n");
#endif

#ifdef DEBUG_DUMP_RAWVIDEO
		if (m_rawvideo_output) {
			video_info_t* pVInfo = pEncoder->GetVideoInfo();
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

int CTransWorkerSeperate::StartWork()
{
	int ret = 0;
	ret = CBIThread::Create(m_hMainThread, CTransWorkerSeperate::mainThread, this);
	if(ret != 0) {
		SetState(TASK_ERROR);
		logger_err(m_logType, "Failed to start Worker %d .\n", m_id);
		SetErrorCode(EC_START_ERROR);
	} 
	
	return ret;
}

bool CTransWorkerSeperate::createAuxAudioAnalyseThread(std::vector<CBIThread::Handle>& threadHandles)
{
	for(size_t i=0; i<m_auxDecoders.size(); ++i) {
		int ret = 0;
		CBIThread::Handle curHandle;
		multi_thread_param_t* decodeParam = new multi_thread_param_t;
		decodeParam->curStreamId = i;
		decodeParam->worker = this;
		ret = CBIThread::Create(curHandle, auxAudioAnalyseEntry, decodeParam);
		if (ret != 0) {
			SetErrorCode(EC_CREATE_THREAD_ERROR);
			return false;
		}
		#ifdef LOWER_ENCODE_THREAD
		CBIThread::SetPriority(curHandle, THREAD_PRIORITY_BELOW_NORMAL);
		#endif
		threadHandles.push_back(curHandle);
	}
	
	logger_status(m_logType, "Create replay gain analyse of aux audio thread successfully.\n");
	return true;
}

THREAD_RET_T WINAPI CTransWorkerSeperate::auxAudioAnalyseEntry(void* decoderParam)
{
	multi_thread_param_t* pParams = static_cast<multi_thread_param_t*>(decoderParam);
	CTransWorkerSeperate* theWorker = pParams->worker;
	int curIdx = pParams->curStreamId;
	CDecoder* curDecoder = theWorker->m_auxDecoders[curIdx];
	delete pParams;
	// Init audio decoder
	if(!curDecoder || !curDecoder->ReadAudioHeader()) return -1;

	wav_info_t* pWav = curDecoder->GetWavInfo();
	int sampleNumEachCh = pWav->sample_rate;
	if(sampleNumEachCh <= 0 || sampleNumEachCh > 44100*20) {
		sampleNumEachCh = 44100;
	}
	int pcmBufLen = sampleNumEachCh*pWav->channels*(pWav->bits >> 3);
	uint8_t* pOriginAudioBuf = (uint8_t*)malloc(pcmBufLen);

	// Initialize replay gain analyse
	void* rgContext = CreateGainContext();
	if(!InitGainAnalysis(rgContext, pWav->sample_rate)) {
		FreeGainContext(rgContext);
		logger_err(LOGM_TS_WK_SEP, "Init replay gain analyse failed.\n");
		return -1;
	}

	Float_t* leftBuf = (Float_t*)malloc(sampleNumEachCh*sizeof(Float_t));
	Float_t* rightBuf = (Float_t*)malloc(sampleNumEachCh*sizeof(Float_t));
	
	
	int ret = 0;
	while (theWorker->m_errCode == EC_NO_ERROR) {
		if(theWorker->m_taskState >= TASK_DONE) { 
				logger_info(LOGM_TS_WK_SEP, "Task cancelled, exit encoding thread.\n"); break; 
		} 
		if(theWorker->m_taskState == TASK_PAUSED) { 
			Sleep(100);		continue; 
		}
		// Process multi source files decoding (Joining)
		if(curDecoder && curDecoder->GetAudioReadHandle() < 0 ) { 
			if(!theWorker->m_bDecodeNext) break;
		} 
		if(theWorker->m_lockReadAudio == 1) { 
			if(curDecoder && curDecoder->GetAudioReadHandle()>0) { 
				if(curDecoder->ReadAudioHeader()) { 
					audio_info_t* pTmpAudioInfo = curDecoder->GetAudioInfo(); 
					wav_info_t* pTmpWav = curDecoder->GetWavInfo(); 
					theWorker->checkAudioParam(pTmpAudioInfo, pTmpWav); 
				} else {break; }
				theWorker->m_lockReadAudio = 0; 
			} else {
				continue;
			}
		} 

		if(curDecoder && curDecoder->GetAudioReadHandle() > 0) {
			// Read audio data (1 second data each time), just discard, don't process
			int readLen = curDecoder->ReadAudio(pOriginAudioBuf, pcmBufLen);
			if(readLen <= 0) {
				if(theWorker->m_bDecodeNext) {
					curDecoder->CloseAudioReadHandle();
				} else {	
					break;
				}
			}

			// Analyse every audio sample
			if(rgContext) {
				if(readLen < pcmBufLen) {	// Last read operation, sample num may < samplerate
					sampleNumEachCh = readLen * 8 / pWav->bits / pWav->channels;
					readLen = sampleNumEachCh*(pWav->bits >> 3)*pWav->channels;
				}
				conv16Float(pOriginAudioBuf, readLen, leftBuf, rightBuf, pWav->channels);
				if(!AnalyzeSamples(rgContext, leftBuf, rightBuf, sampleNumEachCh, pWav->channels)) {
					logger_err(LOGM_TS_WK_SEP, "Init replay gain analyse failed.\n");
					ret = -1;
					break;
				}
			}
		}
	}

	if(rgContext) {
		if(ret == 0) {
			audio_info_t* aInfo = curDecoder->GetAudioInfo();
			aInfo->volGain = (float)GetTitleGain(rgContext);
		}
		free(leftBuf);
		free(rightBuf);
		FreeGainContext(rgContext);
	}
	free(pOriginAudioBuf);
	return ret;
}

bool CTransWorkerSeperate::firstPassAnalyse()
{
	bool ret = false;
	do {
		// First pass video analyse
		if(m_encoderPass > 1 && !createTranscodeVideoThread()) {
			logger_err(m_logType, "Create video transcode thread failed.\n");
			break;
		}

		if(!m_audioEncs.empty() && (m_encoderPass > 1 || m_bAutoVolumeGain)) {
			// Aux audio replay gain analyse(start threads to analyse aux audio)
			std::vector<CBIThread::Handle> audioHandles;
			bool auxThreadSucess = false;
			// At least source video has two audio track
			if(m_bAutoVolumeGain && !m_auxDecoders.empty() && m_srcAudioAttribs.size() > 1) {
				auxThreadSucess = createAuxAudioAnalyseThread(audioHandles);
			}
			
			// discard or gain analyse of main audio track simultaneously
			if(analyseMainAudioTrack() != 0) {
				logger_err(m_logType, "First pass audio analyse failed.\n");
				break;
			}
			
			if(!audioHandles.empty() && auxThreadSucess) {
				for(size_t i=0; i<audioHandles.size(); ++i)
					CBIThread::Join(audioHandles[i]);
			}
		}
		
		if(m_encoderPass > 1) CBIThread::Join(m_hVideoThread);

		m_hVideoThread = NULL;
		m_hAudioThread = NULL;

		closeDecoders();

		// Validate if transcode is complete 
		int decoderExitCode = -1;
		CDecoder* pDecoder = m_pVideoDec;
		if(!pDecoder) pDecoder = m_pAudioDec;
		if(pDecoder) {
			decoderExitCode = pDecoder->GetExitCode();
		}
		if(!validateTranscode(decoderExitCode)) {
			break;
		}

		ret = true;
	} while (false);
	return ret;
}

bool CTransWorkerSeperate::initPass2()
{
	// Reset aux decoder's command line
	if(m_bAutoVolumeGain) {
		for(size_t i=0; i<m_auxDecoders.size(); ++i) {
			m_auxDecoders[i]->ResetCommandLine();
		}
	}

	m_streamFiles.RewindSrcFiles();
	const char* srcFile = m_streamFiles.GetCurSrcFile();
	if(m_bAutoVolumeGain && m_encoderPass == 1 && !m_videoEncs.empty()) {
		delete m_pAudioDec;
		m_bAutoVolumeGain = false;	// Ensure init video decoder correctly
		if (!startDecoder(srcFile)) {
			SetErrorCode(EC_DECODER_ERROR);
			return false;
		}
	} else {
		m_bDecodeNext = m_streamFiles.GetSrcFileCount() > 1 ? true : false;
		if(m_bDecodeNext || m_bAutoVolumeGain) {	
			if(m_pVideoDec) m_pVideoDec->ResetCommandLine();
			if(m_pAudioDec) m_pAudioDec->ResetCommandLine();	
		}
		if(m_bDecodeNext) {
			// After pass 1, original audio samplerate is the last file's, should force to resample to dest value.
			if(m_pAudioDec) m_pAudioDec->SetForceResample(true);
		}

		if(m_encoderPass > 1) {
			m_encoderPass--;
			m_pVideoDec->SetLastPass(true);
			if(m_pImgTail) m_pImgTail->Reset();
		}

		if(m_pAudioDec == m_pVideoDec) {
			m_pVideoDec->Start(srcFile);
		} else {
			if(m_pVideoDec) m_pVideoDec->Start(srcFile);
			if(m_pAudioDec) m_pAudioDec->Start(srcFile);
		}
	}

	// Init resources in 2nd pass or ABR mode.
	if(!initResourceInOnePassOrLastPass(srcFile)) {
		return false;
	}

	// Reset benchmark data
	m_encodedFrames = 0;
	m_encAudioBytes = 0;
    m_tmpBenchData.lastFrameNum = 0;
	m_tmpBenchData.videoEncTime = 0.f;
	m_tmpBenchData.audioEncTime = 0.f;
	videoClipIndex = 0;
	audioClipIndex = 0;
	m_videoClipEnd = 0;
	m_audioClipEnd = 0;
	return true;
}

THREAD_RET_T WINAPI CTransWorkerSeperate::mainThread(void* transWorker)
{
	CTransWorkerSeperate* pWorker = static_cast<CTransWorkerSeperate*>(transWorker);
	return pWorker->mainFunc();
}

THREAD_RET_T CTransWorkerSeperate::mainFunc()
{
	#define FAIL_INFO(err) logger_err(m_logType, err); ret=-1; break;
	int ret = 0;
	m_startTime = m_lastTime = GetTickCount();
	SetState(TASK_ENCODING);

	logger_status(m_logType, "Enter tansworker main thread.\n");

	do {
		if(m_bLosslessClip) {
			if(!processLosslessClip()) {
				logger_err(m_logType, "Init worker %d failed.\n", GetId());
				ret=-1;
			}
			break;
		}

		if((m_bCopyAudio || m_bCopyVideo) && !processStreamCopy()) {
			FAIL_INFO("processStreamCopy failed.\n");
		}

		// If copy audio, no need analyse volume gain
		if(m_bCopyAudio) m_bAutoVolumeGain = false;

		// Backup out channels of video info
		// To analyse volume even if original audio is 5.1(downmix to 2 channel)
		std::vector<int> outChannelConfigs;
		if(m_bAutoVolumeGain) {
			for(size_t i=0; i<m_audioEncs.size(); ++i) {
				audio_info_t* pTmpAInfo = m_audioEncs[i]->GetAudioInfo();
				outChannelConfigs.push_back(pTmpAInfo->out_channels);
				if(pTmpAInfo->out_channels > 2) {
					pTmpAInfo->out_channels = 2;
				}
			}
		}

		if(!m_bCopyAudio || !m_bCopyVideo) {	// Init when at least transcode one stream
			if(!initialize()) {
				logger_err(m_logType, "Init worker %d failed.\n", GetId());
				ret=-1; break;
			}
		}

		if(m_encoderPass > 1 || m_bAutoVolumeGain) {
			if(!firstPassAnalyse()) {
				FAIL_INFO("Perform first pass analyse failed.\n");
			}
			// Restore out channels of video info after volume gain analyse in pass 1
			if(m_bAutoVolumeGain) {
				for(size_t i=0; i<m_audioEncs.size(); ++i) {
					audio_info_t* pTmpAInfo = m_audioEncs[i]->GetAudioInfo();
					pTmpAInfo->out_channels = outChannelConfigs[i];
				}
			}

			if(!initPass2()) {
				FAIL_INFO("Init pass 2 failed.\n");
			}
		}

		if(!m_videoEncs.empty() && !createTranscodeVideoThread()) {
			FAIL_INFO("Create video transcode thread failed.\n");
		}

		if(!m_audioEncs.empty()) {
			if(!createTranscodeAudioThread()) {
				FAIL_INFO("Create audio transcode thread failed.\n");
			}
		} 

		// Wait for audio/video thread finish
		if(m_hAudioThread) {
			CBIThread::Join(m_hAudioThread);
			m_hAudioThread = NULL;
		}
		if(m_hVideoThread) {
			CBIThread::Join(m_hVideoThread);
			m_hVideoThread = NULL;
		}
		
		// Wait for audio/video copy finish
		waitForCopyStreams();
		closeDecoders();
		
		if(GetState() == TASK_CANCELLED) {
			FAIL_INFO("User canceled transcoding.\n");
		}

		// Validate transcoding, check if error exists
		int decoderExitCode = -1;
		CDecoder* pDecoder = m_pVideoDec;
		if(!pDecoder) pDecoder = m_pAudioDec;
		if(pDecoder) {
			decoderExitCode = pDecoder->GetExitCode();
		}
		if(!validateTranscode(decoderExitCode)) {
			ret = -1;  break;
		}

		if(!m_bCopyAudio && !m_bCopyVideo && 
			m_videoEncs.empty() && m_audioEncs.empty()) {
			FAIL_INFO("No stream to be encoded or copyed.\n");
		}

		// After all thread finished, do muxing
		logger_status(m_logType, "Start muxing...\n");
		SetState(TASK_MUXING);
		if(!doMux()) {
			FAIL_INFO("Muxing failed.\n");
		}
#ifdef BUILD_FOR_PPLIVE
 		std::string dstFile = GetOutputFileName(0);
 		m_dstMd5 = GetFileMd5(dstFile.c_str());
#endif
		// Generate playlist
		if(m_pPlaylist && m_pSplitter) {	// Record every item's duration in playlist
			std::vector<int64_t>& segTimes = m_pSplitter->GetSegmentTimes();
			for (size_t i=0; i<segTimes.size(); ++i) {
				if(i==0) m_pPlaylist->AddSegmentDuration(segTimes[i]);
				else m_pPlaylist->AddSegmentDuration(segTimes[i]-segTimes[i-1]);
			}
			m_pPlaylist->GenPlaylist();
		}
	} while (false);

	m_endTime = GetTickCount();
	m_elapseSecs = (int)((m_endTime - m_startTime)/1000.f);
	logger_info(m_logType, "Time consume: %ds.\n",  m_elapseSecs);

	if(ret == 0) {
		logger_status(m_logType, "Mux completed, transcode task done successfully!\n");
		SetState(TASK_DONE);
		m_progress = 100.f;
	} else {
		logger_err(m_logType, "Transcode task failed and exit!\n");
		SetState(TASK_ERROR);
		m_progress = 0.f;
	}
	
	deleteDecoders();
	// Notify cli to finish
	if(m_pFinishCbForCli) m_pFinishCbForCli();

	#undef FAIL_INFO
	return 0;
}


void CTransWorkerSeperate::waitForCopyStreams()
{
	if(m_bCopyAudio && m_pAudioDec) {
		m_pAudioDec->Wait(INFINITE);
	}
	if(m_bCopyVideo && m_pVideoDec) {
		m_pVideoDec->Wait(INFINITE);
	}
}

THREAD_RET_T CTransWorkerSeperate::transcodeVideo()
{
	if(!initVideoEncoders()) {
	    if(m_pVideoDec) {
			m_pVideoDec->CloseVideoReadHandle();
		}
		logger_err(m_logType, "Video encoders init failed.\n");
		return -1;
	}
	
	THREAD_RET_T ret = 0;
	size_t videoNum = m_videoEncs.size();
	// Select transcode methods according to conditions
	if(videoNum == 1) {		// Single video stream encoding
		if(m_bDecodeNext || m_pSplitter || !m_clipStartSet.empty()) {	// Complex mode (join or split mode)
			ret = transcodeSingleVideoComplex();
		} else {							// Simple mode
			ret = transcodeSingleVideo();
		}
	} else if(videoNum > 1){				// Multi video streams encoding
		if(!m_pSplitter) {
			ret = transcodeMbrVideo();
		} else {
			ret = -1;
			logger_err(m_logType, "Splitting video doesn't support in Mbr mode.\n");
		}
	} else {
		logger_info(m_logType, "No video to transcode, only transcode audio.\n"); 
	}

	// Rename firstPass x264 stat file(when transcode multiple file, x264 rename will fail)
	if(m_encoderPass == 2) {
		m_streamFiles.RenameX264StatFile();
	}
	
	return ret;
}

bool CTransWorkerSeperate::addImageTailToVideoStream(CVideoEncoder* pEncoder)
{
	if(!m_pImgTail) return true;
	uint8_t* pYuv = m_pImgTail->GetNextFrame();
	while(pYuv) {
		int frameCount = pEncoder->EncodeFrame(pYuv, -1);
		if(frameCount <= 0) {
			return false;
		}
		pYuv = m_pImgTail->GetNextFrame();
	}
	return true;
}

#define AV_DUR_DIFF_ALLOW 5
bool CTransWorkerSeperate::appendBlankAudio(CAudioEncoder* pEncoder)
{
	int ignoreErrIdx = CWorkManager::GetInstance()->GetIgnoreErrorCode();
	if(ignoreErrIdx < 2) return true;

	uint32_t bytesPerSec = m_pAudioDec->GetWavInfo()->bytes_per_second;
	if(bytesPerSec > INT_MAX) return false;
	if(m_tmpBenchData.audioEncTime > 0 && m_tmpBenchData.videoEncTime > 0) {
		int audioShortSec = (int)(m_tmpBenchData.videoEncTime - m_tmpBenchData.audioEncTime);
		if(audioShortSec > AV_DUR_DIFF_ALLOW) {
			uint8_t* buf = (uint8_t*)malloc(bytesPerSec);
			memset(buf, 0, bytesPerSec);
			for(int i=audioShortSec-1; i>=0; --i) {
				pEncoder->EncodeBuffer(buf, bytesPerSec);
			}
			free(buf);
		}
	}
	return true;
}

bool CTransWorkerSeperate::appendBlankVideo(CVideoEncoder* pEncoder)
{
	int ignoreErrIdx = CWorkManager::GetInstance()->GetIgnoreErrorCode();
	if(ignoreErrIdx < 2) return true;

	if(m_tmpBenchData.audioEncTime > 0 && m_tmpBenchData.videoEncTime > 0) {
		int videoShortSec = (int)(m_tmpBenchData.audioEncTime - m_tmpBenchData.videoEncTime);
		if(videoShortSec > AV_DUR_DIFF_ALLOW) {
			int totalFrames = videoShortSec*m_tmpBenchData.fpsNum/m_tmpBenchData.fpsDen;
			int bufSize = pEncoder->GetFrameSize();
			resolution_t videoRes = pEncoder->GetVideoInfo()->res_out;
			int planeSize = videoRes.width*videoRes.height;
			uint8_t* buf = (uint8_t*)malloc(bufSize);
			// YUV 为黑色的值为(0, 128, 128)
			memset(buf, 128, bufSize);
			memset(buf, 0, planeSize);
			for(int i=totalFrames-1; i>=0; --i) {
				pEncoder->EncodeFrame(buf, bufSize);
			}
			free(buf);
		}
	}
	return true;
}

THREAD_RET_T CTransWorkerSeperate::transcodeSingleVideo()
{
	#define FAIL_INFO(err) logger_err(m_logType, err); ret=-1; break;
	int ret = 0;
	CVideoEncoder* pSingleEncoder = m_videoEncs[0];
	
	double eachFrameSec = 0.0;
	if(m_tmpBenchData.fpsNum > 0) {
		eachFrameSec = (double)m_tmpBenchData.fpsDen/m_tmpBenchData.fpsNum;
	}

	// Encoding loop
	while (ret >= 0 && m_errCode == EC_NO_ERROR) {
		PROCESS_PAUSE_STOP;
		if(m_pVideoDec->ReadVideo(m_yuvBuf, m_frameBufSize) != m_frameBufSize) {	//read framebuf first
			break;
		}

#ifdef DEMO_RELEASE		// Add demo watermark
		if(m_pWaterMarkMan) {
			m_pWaterMarkMan->RenderWaterMark(m_yuvBuf);
		}
#endif
		//filter
		uint8_t* pFrameBuf = pSingleEncoder->FilterFrame(m_yuvBuf, m_frameBufSize);
		if (pFrameBuf == NULL) {	//skiped or failed
			continue;
		}

		CHECK_VIDEO_ENCODER(pSingleEncoder);

		m_tmpBenchData.videoEncTime = (float)(m_encodedFrames*eachFrameSec);
		//encode
		int frameCount = pSingleEncoder->EncodeFrame(pFrameBuf, -1 /*TODO:BufSize*/);
		if(frameCount > 0) {
			m_encodedFrames++;
			if(m_encodedFrames%UPDATE_FRAMES_INTERVAL == 0) {
				benchmark();
			}
		} else {
			FAIL_INFO("Encode frame failed.\n");
		}
	}

	if(ret == 0 && !addImageTailToVideoStream(pSingleEncoder)) {
		ret = -1;
		logger_err(m_logType, "Add image tail failed.\n");
	}

	if(!pSingleEncoder->StopThumbnail()) {
		logger_warn(m_logType, "Add image tail failed.\n");
		//ret = -1;
		//SetErrorCode(EC_GEN_THUMBNAIL_ERROR);
	}

	appendBlankVideo(pSingleEncoder);

	pSingleEncoder->Stop();
	
	if(ret < 0) {
		SetErrorCode(EC_VIDEO_ENCODER_ERROR);
		logger_err(m_logType, "Transcode video failed.\n");
		m_pVideoDec->Cleanup();
	} else {
		printf("Processed Frames: %d\n", m_encodedFrames);
		logger_status(m_logType, "Video stream transcode have completed.\n");
	}

#undef FAIL_INFO
	return ret;
}

THREAD_RET_T CTransWorkerSeperate::transcodeMbrVideo()
{
	#define FAIL_INFO(err) logger_err(m_logType, err); ret=-1; break;
	int ret = 0;
	std::vector<CBIThread::Handle> videoHandles;
	const size_t videoNum = m_videoEncs.size();
	for (size_t i=0; i<videoNum; ++i) {
		m_videoEncs[i]->GeneratePipeHandle();
		CBIThread::Handle tmpVideoHandle = NULL;
		multi_thread_param_t* encodeParam = new multi_thread_param_t;
		encodeParam->worker = this;
		encodeParam->curStreamId = i;
		// Create video transcode thread
		int createRet = CBIThread::Create(tmpVideoHandle, encodeVideoThread, encodeParam);
		if(createRet == 0) {
			videoHandles.push_back(tmpVideoHandle);
#ifdef LOWER_ENCODE_THREAD
			CBIThread::SetPriority(tmpVideoHandle, THREAD_PRIORITY_BELOW_NORMAL);
#endif
		} else {
			FAIL_INFO("Create video transcode thread failed.\n");
		}
	}
		
	// Encoding loop
	while (ret >= 0 && m_yuvBuf && m_errCode == EC_NO_ERROR) {
		PROCESS_PAUSE_STOP;
		// Process multi source files decoding (Joining)
		if(m_pVideoDec && m_pVideoDec->GetVideoReadHandle() < 0 ) { 
			if(m_bDecodeNext) { 
				decodeNext(); 
				Sleep(50); continue; 
			} else break;
		} 
		if(m_lockReadVideo == 1) { 
			if(m_pVideoDec && m_pVideoDec->GetVideoReadHandle() > 0) {
				if(m_videoDecType != VD_FFMPEG) {
					if(!m_pVideoDec->ReadVideoHeader()) break; 
				}
				m_lockReadVideo = 0; 
			} else {
				continue;
			}
		}
	
		if(m_pVideoDec->ReadVideo(m_yuvBuf, m_frameBufSize) != m_frameBufSize) {	//read framebuf first
			if(!m_bDecodeNext) break;	// End of the file
			m_pVideoDec->CloseVideoReadHandle();
			continue;	
		}

#ifdef DEMO_RELEASE		// Add demo watermark
		if(m_pWaterMarkMan) {
			m_pWaterMarkMan->RenderWaterMark(m_yuvBuf);
		}
#endif
		// Parallel encoding multiple video streams
		for (size_t i=0; i<videoNum; ++i) {
			CVideoEncoder* pEncoder = m_videoEncs[i];
			int fdWr = pEncoder->GetWirteHandle();
			if(fdWr < 0) {
				FAIL_INFO("Get read handle failed.\n");
			}
			if(SafeWrite(fdWr, m_yuvBuf, m_frameBufSize) < 0) {
				FAIL_INFO("Write yuv data for video stream failed.\n");
			}
		}
	}

	//m_pVideoDec->CloseVideoReadHandle();
	for (size_t i=0; i<videoNum; ++i) {	// Close write handle
		m_videoEncs[i]->CloseWriteHandle();
	}
	// Wait for multiple video encode thread to exit
	for (size_t i=0; i<videoHandles.size(); ++i) {
		CBIThread::Join(videoHandles[i]);
	}

	if(ret < 0) {
		SetErrorCode(EC_VIDEO_ENCODER_ERROR);
		m_pVideoDec->Cleanup();
		logger_err(m_logType, "Transcode video failed.\n");
	} else {
		logger_status(m_logType, "All video streams transcode have completed.\n");
	}

#undef FAIL_INFO
	return ret;
}

THREAD_RET_T CTransWorkerSeperate::transcodeSingleVideoComplex()
{
	#define FAIL_INFO(err) logger_err(m_logType, err); ret=-1; break;
	int ret = 0;
	CVideoEncoder* pSingleEncoder = m_videoEncs[0];
	
	bool bUpdateEveryTime = false;
	size_t clipsCount = m_clipStartSet.size();

	double eachFrameSec = 0.f;
	if(m_tmpBenchData.fpsNum > 0) {
		eachFrameSec = (double)m_tmpBenchData.fpsDen/(double)m_tmpBenchData.fpsNum;
	}

	while (ret >= 0 && m_errCode == EC_NO_ERROR) {	// Encoding loop
		PROCESS_PAUSE_STOP;
		// Process multi source files decoding (Joining)
		if(m_pVideoDec && m_pVideoDec->GetVideoReadHandle() < 0 ) { 
			if(m_bDecodeNext) { 
				decodeNext(); 
				Sleep(50); continue; 
			} else break;
		} 
		if(m_lockReadVideo == 1) { 
			if(m_pVideoDec && m_pVideoDec->GetVideoReadHandle() > 0) {
				if(m_videoDecType != VD_FFMPEG) {
					if(!m_pVideoDec->ReadVideoHeader()) break; 
				}
				m_lockReadVideo = 0; 
			} else {
				continue;
			}
		}
	
		// Process Splitting
		bool bDiscardData = false;
		if(m_pSplitter) {
			bDiscardData = m_pSplitter->BeginSplitVideo();
		}

		if(m_pVideoDec && m_pVideoDec->GetVideoReadHandle() > 0) {
			if(m_pVideoDec->ReadVideo(m_yuvBuf, m_frameBufSize) != m_frameBufSize) {	//read framebuf first
				if(!m_bDecodeNext) break;	// End of the file
				m_pVideoDec->CloseVideoReadHandle();
				continue;	
			}
		} else {
			continue;
		}

		if(!m_videoClipEnd) {
			m_encodedFrames++;
			m_tmpBenchData.videoEncTime = (float)(m_encodedFrames*eachFrameSec);
		}

		if(videoClipIndex < clipsCount) {
			// When audio time is smaller than current clip start, then discard data
			if(m_tmpBenchData.videoEncTime*1000 < m_clipStartSet[videoClipIndex]) {
				bDiscardData = true;
			} else if(m_tmpBenchData.videoEncTime*1000 > m_clipEndSet[videoClipIndex]) {
				// When reach to clip end, then change to next clip
				videoClipIndex++;	
			}
		} else if(clipsCount > 0) {		//videoClipIndex = clipsCount
			m_videoClipEnd = 1;
			if(m_pVideoDec != m_pAudioDec || m_audioClipEnd) {
				m_pVideoDec->Cleanup();
				break;
			} else {	// Keep decoding video, but not record time, in order to not block audio decoding
				bDiscardData = true;
			}
		}

		if(m_encodedFrames%UPDATE_FRAMES_INTERVAL == 0) benchmark();
		if(bDiscardData) continue;		// Discard video data

#ifdef DEMO_RELEASE		// Add demo watermark
		if(m_pWaterMarkMan) {
			m_pWaterMarkMan->RenderWaterMark(m_yuvBuf);
		}
#endif
		//filter
		uint8_t* pFrameBuf = pSingleEncoder->FilterFrame(m_yuvBuf, m_frameBufSize);
		if (pFrameBuf == NULL) {	//skiped or failed
			continue;
		}

		CHECK_VIDEO_ENCODER(pSingleEncoder);
		//encode
		int frameCount = pSingleEncoder->EncodeFrame(pFrameBuf, -1 /*TODO:BufSize*/);
		if(frameCount > 0) {
			if(m_pSplitter) {
				if(bUpdateEveryTime || m_encodedFrames%UPDATE_FRAMES_INTERVAL == 0) {
					m_pSplitter->UpdateVideoTime((int)(m_tmpBenchData.videoEncTime*1000));
					/*if(m_pSplitter->GetUnitType() == SPLIT_UNIT_BYSIZE) {
						int64_t tmpVideoSize = StatFileSize(pSingleEncoder->GetOutFileName().c_str());
						m_pSplitter->UpdateVideoSize(tmpVideoSize);
					}*/
					bUpdateEveryTime = m_pSplitter->EndSplitVideo();
				}
			} 
		} else {
			FAIL_INFO("Encode frame failed.\n");
		}
	}

	if(clipsCount > 0) m_videoClipEnd = 1;

	//Finshed or failed
	if(ret == 0 && !addImageTailToVideoStream(pSingleEncoder)) {
		ret = -1;
		logger_err(m_logType, "Add image tail failed.\n");
	}

	if(!pSingleEncoder->StopThumbnail()) {
		logger_warn(m_logType, "Add image tail failed.\n");
		//ret = -1;
		//SetErrorCode(EC_GEN_THUMBNAIL_ERROR);
	}

	appendBlankVideo(pSingleEncoder);

	pSingleEncoder->Stop();

	if(ret < 0) {
		SetErrorCode(EC_VIDEO_ENCODER_ERROR);
		logger_err(m_logType, "Transcode video failed.\n");
		m_pVideoDec->Cleanup();
	} else {
		logger_status(m_logType, "Video stream transcode have completed.\n");
	}

#undef FAIL_INFO
	return ret;
}

THREAD_RET_T CTransWorkerSeperate::analyseMainAudioTrack()
{
	if(!m_pAudioDec || m_pAudioDec->GetAudioReadHandle() < 0) return -1;
	
	// Init audio decoder
	if(!m_pAudioDec->ReadAudioHeader()) return -1;
	wav_info_t* pWav = m_pAudioDec->GetWavInfo();
	int sampleNumEachCh = pWav->sample_rate;
	if(sampleNumEachCh <= 0 || sampleNumEachCh > 44100*20) {
		sampleNumEachCh = 44100;
	}
	int pcmBufLen = sampleNumEachCh*pWav->channels*(pWav->bits >> 3);
	uint8_t* pOriginAudioBuf = (uint8_t*)malloc(pcmBufLen);

	// Initialize replay gain analyse
	void* rgContext = NULL;
	Float_t *leftBuf = NULL, *rightBuf = NULL;
	if(m_bAutoVolumeGain) {
		rgContext = CreateGainContext();
		if(!InitGainAnalysis(rgContext, pWav->sample_rate)) {
			FreeGainContext(rgContext);
			logger_err(m_logType, "Init replay gain analyse failed.\n");
			return -1;
		}
		leftBuf = (Float_t*)malloc(sampleNumEachCh*sizeof(Float_t));
		rightBuf = (Float_t*)malloc(sampleNumEachCh*sizeof(Float_t));
	}
	
	int ret = 0;
	while (m_errCode == EC_NO_ERROR) {
		PROCESS_PAUSE_STOP;
		// Process multi source files decoding (Joining)
		if(m_pAudioDec && m_pAudioDec->GetAudioReadHandle() < 0 ) { 
			if(!m_bDecodeNext) break;
		} 
		if(m_lockReadAudio == 1) { 
			if(m_pAudioDec && m_pAudioDec->GetAudioReadHandle()>0) { 
				if(m_pAudioDec->ReadAudioHeader()) { 
					audio_info_t* pTmpAudioInfo = m_pAudioDec->GetAudioInfo(); 
					wav_info_t* pTmpWav = m_pAudioDec->GetWavInfo(); 
					checkAudioParam(pTmpAudioInfo, pTmpWav); 
				} else {break; }
				m_lockReadAudio = 0; 
			} else {
				continue;
			}
		} 

		if(!m_clipStartSet.empty()) {	// Cut media file, when reach end of clip, exit
			if(m_tmpBenchData.audioEncTime*1000 >= m_clipEndSet.back()) {
				// if video decoder exist and it's equal to audio decoder, don't close
				// decoder in audio encoding thread (close it in video encoding thread)
				m_audioClipEnd = 1;
				if(m_pVideoDec != m_pAudioDec || m_videoClipEnd) {
					m_pAudioDec->Cleanup();
					break;
				}
			}
		}

		if(m_pAudioDec && m_pAudioDec->GetAudioReadHandle() > 0) {
			// Read audio data (1 second data each time), just discard, don't process
			int readLen = m_pAudioDec->ReadAudio(pOriginAudioBuf, pcmBufLen);
			if(readLen <= 0) {
				if(m_bDecodeNext) {
					m_pAudioDec->CloseAudioReadHandle();
				} else {	
					break;
				}
			}

			// Analyse every audio sample
			if(rgContext) {
				if(readLen < pcmBufLen) {	// Last read operation, sample num may < samplerate
					sampleNumEachCh = readLen * 8 / pWav->bits / pWav->channels;
					readLen = sampleNumEachCh*(pWav->bits >> 3)*pWav->channels;
				}
				conv16Float(pOriginAudioBuf, readLen, leftBuf, rightBuf, pWav->channels);
				if(!AnalyzeSamples(rgContext, leftBuf, rightBuf, sampleNumEachCh, pWav->channels)) {
					logger_err(m_logType, "Init replay gain analyse failed.\n");
					ret = -1;
					break;
				}
			}

			if(!m_audioClipEnd) {
				if(readLen == pcmBufLen) {
					m_tmpBenchData.audioEncTime += 1.f;
				} else {
					m_tmpBenchData.audioEncTime += readLen/(float)pcmBufLen;
				}
			}
		}
	}

	if(rgContext) {
		if(ret == 0) {
			audio_info_t* aInfo = m_pAudioDec->GetAudioInfo();
			float volGain = (float)GetTitleGain(rgContext);
			volGain += (m_fVolumeNormalDB - 89);
			aInfo->volGain = volGain;
			// If mapping 2 channel to 2 track
			if(m_srcAudioAttribs.size() == 1 && !m_auxDecoders.empty()) {	
				aInfo = m_auxDecoders[0]->GetAudioInfo();
				aInfo->volGain = volGain;
			}
		}
		free(leftBuf);
		free(rightBuf);
		FreeGainContext(rgContext);
	}

	if(!m_clipStartSet.empty()) m_audioClipEnd = 1;
	free(pOriginAudioBuf);
	//m_pAudioDec->CloseAudioReadHandle();
	return ret;
}

THREAD_RET_T CTransWorkerSeperate::transcodeAudio()
{
	if(!initAudioEncoders()) {
	    m_pAudioDec->CloseAudioReadHandle();
		logger_err(m_logType, "Audio encoders init failed.\n");
		return -1;
	}
	
	THREAD_RET_T ret = 0;
	size_t audioNum = m_audioEncs.size();

	// Select transcode methods according to conditions
	if(audioNum == 1) {		// Single audio stream encoding
		if(m_bDecodeNext || m_pSplitter || !m_clipStartSet.empty()) {	// Complex mode (join or split mode)
			ret = transcodeSingleAudioComplex();
		} else {							// Simple mode
			ret = transcodeSingleAudio();
		}
	} else if(audioNum > 1){				// Multi video streams encoding
		if(!m_pSplitter) {
			ret = transcodeMbrAudio();
		} else {
			ret = -1;
			logger_err(m_logType, "Splitting audio doesn't support in Mbr mode.\n");
		}
	} else {
		logger_info(m_logType, "No audio to transcode, only transcode video.\n"); 
	}

	return ret;
}

THREAD_RET_T CTransWorkerSeperate::transcodeMbrAudio()
{
	#define FAIL_INFO(err) logger_err(m_logType, err); ret=-1; break;
	int ret = 0;
	size_t audioNum = m_audioEncs.size();
	int pcmBufLen = m_pcmBufSize;
	std::vector<CBIThread::Handle> audioHandles;

	uint8_t* pOriginAudioBuf = NULL;
	uint8_t** pBufAfterAf = NULL;			// For multiple audio stream encoding
	CAudioResampler** pResamplers = NULL;	// For multiple audio stream encoding (resample)
	
	const size_t maxAuxNum = 5;
	uint8_t* pAuxBuf[maxAuxNum] = {NULL};	// Aux buf for aux decoder(5 is big enough)

	do {	
		pOriginAudioBuf = m_pcmBuf;		// Use buffer that the worker allocated
		pBufAfterAf = (uint8_t**)malloc(sizeof(uint8_t*)*audioNum);
		pResamplers = new CAudioResampler*[audioNum];
		if(!pBufAfterAf || !pResamplers) {
			FAIL_INFO("Allocate memory failed in transcodeAudio.\n");
		}
			
		for (size_t i=0; i<audioNum; ++i) {
			pBufAfterAf[i] = NULL;
			pResamplers[i] = NULL;
			m_audioEncs[i]->GeneratePipeHandle();
			CBIThread::Handle tmpAudioHandle = NULL;
			multi_thread_param_t* encodeParam = new multi_thread_param_t;
			encodeParam->worker = this;
			encodeParam->curStreamId = i;
			ret = CBIThread::Create(tmpAudioHandle, CTransWorkerSeperate::encodeAudioThread, encodeParam);
			if(ret == 0) {
				audioHandles.push_back(tmpAudioHandle);
#ifdef LOWER_ENCODE_THREAD
				CBIThread::SetPriority(tmpAudioHandle, THREAD_PRIORITY_BELOW_NORMAL);
#endif
			} else {
				FAIL_INFO("Create audio transcode thread failed.\n");
			}
		}
	} while (false);

	//bool bUpdateEveryTime = false;
	// Encoding loop
	while (ret >= 0 && pOriginAudioBuf && m_errCode == EC_NO_ERROR) {
		PROCESS_PAUSE_STOP;

		// Process multi source files decoding (Joining)
		if(m_bDecodeNext && m_pAudioDec && m_pAudioDec->GetAudioReadHandle() < 0 ) { 
			// If there is no video stream, then call decodeNext() here, else in video thread
			if(m_videoEncs.empty()) decodeNext(); 
			Sleep(50); 
			if(m_auxDecoders.empty()) continue; 
		} 
		if(m_lockReadAudio == 1) { 
			bool allDecoderRestarted = m_pAudioDec && m_pAudioDec->GetAudioReadHandle()>0;
			for (size_t i=0; i<m_auxDecoders.size(); ++i) { 
				allDecoderRestarted |= allDecoderRestarted && m_auxDecoders[i]->GetAudioReadHandle()>0;
			} 
			if(allDecoderRestarted) { 
				if(m_pAudioDec->ReadAudioHeader()) { 
					audio_info_t* pTmpAudioInfo = m_pAudioDec->GetAudioInfo(); 
					wav_info_t* pTmpWav = m_pAudioDec->GetWavInfo(); 
					checkAudioParam(pTmpAudioInfo, pTmpWav); 
				} else {break; }
				for(size_t i=0; i<m_auxDecoders.size(); ++i) { 
					if(m_auxDecoders[i]->ReadAudioHeader()) { 
						audio_info_t* pTmpAudioInfo = m_auxDecoders[i]->GetAudioInfo(); 
						wav_info_t* pTmpWav = m_auxDecoders[i]->GetWavInfo(); 
						checkAudioParam(pTmpAudioInfo, pTmpWav); 
					} else {break; }
				} 
				m_lockReadAudio = 0; 
			} 
		} 

		int readLen = 0;
		if(m_pAudioDec && m_pAudioDec->GetAudioReadHandle() > 0) {
			if((readLen = m_pAudioDec->ReadAudio(pOriginAudioBuf, pcmBufLen)) <= 0) {//read audio data (1 second data each)
				if(m_bDecodeNext) {
					m_pAudioDec->CloseAudioReadHandle();
				} else {	// Judge if all decoders are shut down
					bool allDecoderClosed = true;
					for (size_t i=0; i<m_auxDecoders.size(); ++i) {
						if(!m_auxDecoders[i]->GetDecodingEnd()) {
							allDecoderClosed = false; break;
						}
					}
					if(allDecoderClosed) break;		// if all decoders are shut down, then exit encoding
				}
				if(m_auxDecoders.empty()) continue;	//If there is no aux decoder, don't execute following code
			}
		}
		
#ifdef DEBUG_DUMP_RAWAUDIO
		if (m_rawaudio_output) {
			int i_output = m_rawaudio_output->Write((char*)pOriginAudioBuf, readLen);
			if (i_output < readLen) {
				printf("Dump out %d, want %d\n", i_output, readLen);
			}
		}
#endif

		for (size_t i=0; i<audioNum; ++i) {
			CAudioEncoder* pAudioEnc = m_audioEncs[i];
			int fdWr = pAudioEnc->GetWirteHandle();
			if(fdWr < 0) {
				FAIL_INFO("Get read handle failed.\n");
			}
			
			audio_info_t* pAInfo = pAudioEnc->GetAudioInfo();
			if(pAInfo->out_srate != pAInfo->in_srate) {		// Audio resample
				CAudioResampler* pResampler = pResamplers[i];
				int outDataLen = 0;
				if(!pResampler) {	// Init resampler
					pResampler = new CAudioResampler(pAInfo->in_srate, pAInfo->in_channels, pAInfo->out_srate, pAInfo->bits);
					pBufAfterAf[i] = (uint8_t*)malloc(readLen);
					memset(pBufAfterAf[i], 0, readLen);
					if(!pResampler || !pBufAfterAf[i]) {
						FAIL_INFO("Allocate memory failed in transcodeAudio.\n");
					}
					pResampler->InitResampler();
					pResamplers[i] = pResampler;
				} 
				// Filter audio data
				pResampler->Process(pOriginAudioBuf, readLen, pBufAfterAf[i], outDataLen);
				if(outDataLen > 0) {
					if(_write(fdWr, pBufAfterAf[i], outDataLen) < 0) {
						FAIL_INFO("Write pcm data for audio stream failed.\n");
					}
				}
			} else {		// Encoding without resample
				int srcIdx = pAudioEnc->GetSourceIndex();
				if(srcIdx > 0) {	// It's a aux decoder
					int auxBufSize = pAudioEnc->GetFrameLen();
					if(auxBufSize <= 0) {
						auxBufSize = pAInfo->out_srate*pAInfo->out_channels*(pAInfo->bits>>3);
						auxBufSize /= m_audioFractionOfSecond;
						EnsureMultipleOfDivisor(auxBufSize, pAInfo->out_channels*(pAInfo->bits>>3));
					}
					if(pAuxBuf[srcIdx-1] == NULL) {
						pAuxBuf[srcIdx-1] = (uint8_t*)malloc(auxBufSize);
						if(!pAuxBuf[srcIdx-1]) {
							FAIL_INFO("Allocate memory failed in transcodeAudio.\n");
						}
					}
					CDecoder* pAuxDecoder = m_auxDecoders[srcIdx-1];
					int auxLen = 0;
					if(pAuxDecoder->GetAudioReadHandle() > 0) { // Process joining media
						auxLen = pAuxDecoder->ReadAudio(pAuxBuf[srcIdx-1], auxBufSize);
						if(auxLen <= 0) {		
							if(m_bDecodeNext) {	// Process joining media
								pAuxDecoder->CloseAudioReadHandle();
							} else {			// Signal aux decoder to end
								pAuxDecoder->SetDecodingEnd(true);
							}
						}
					}
					if(auxLen > 0 && SafeWrite(fdWr, pAuxBuf[srcIdx-1], auxLen) < 0) {
						FAIL_INFO("Write pcm data for audio stream failed.\n");
					}
				} else {
					if(readLen > 0 && SafeWrite(fdWr, pOriginAudioBuf, readLen) < 0) {
						FAIL_INFO("Write pcm data for audio stream failed.\n");
					}
				}
			}
		}
	}

	//Finshed or in error now
	//m_pAudioDec->CloseAudioReadHandle();
	//for (size_t i=0; i<m_auxDecoders.size(); ++i) {
	//	m_auxDecoders[i]->CloseAudioReadHandle();
	//} 

	for (size_t i=0; i<audioNum; ++i) {		// Close write handle
		m_audioEncs[i]->CloseWriteHandle();
	}

	// Wait for multiple audio encode thread to exit
	for (size_t i=0; i<audioHandles.size(); ++i) {
		CBIThread::Join(audioHandles[i]);
	}

	if(pResamplers) {	// Release resamplers
		for (size_t i=0; i<audioNum; ++i) {
			delete pResamplers[i];
		}
		delete[] pResamplers;
	}

	if(pBufAfterAf) {	// Release buffers
		for (size_t i=0; i<audioNum; ++i) {
			free(pBufAfterAf[i]);
		}
		free(pBufAfterAf);
	}

	for (size_t i=0; i<maxAuxNum; ++i) {
		free(pAuxBuf[i]);
	}

	if(ret < 0) {
		SetErrorCode(EC_AUDIO_ENCODER_ERROR);
		logger_err(m_logType, "TranscodeAudio failed.\n");
	} else {
		logger_status(m_logType, "Multiple audio streams transcoding have completed.\n");
	}
	
#undef FAIL_INFO
	return ret;
}

THREAD_RET_T CTransWorkerSeperate::transcodeSingleAudio()
{
	int ret = 0; 
	CAudioEncoder* pSingleEncoder = m_audioEncs[0];
	
	// Encoding loop
	uint32_t bytesPerSec = m_pAudioDec->GetWavInfo()->bytes_per_second;
	while (ret >= 0 && m_errCode == EC_NO_ERROR) {
		PROCESS_PAUSE_STOP;
		int readLen = 0;
		if(m_pAudioDec->GetAudioReadHandle() > 0) {
			// Read audio data (1/fraction second data once)
			if((readLen = m_pAudioDec->ReadAudio(m_pcmBuf, m_pcmBufSize)) <= 0) {
				break;
			}
		}

		CHECK_AUDIO_ENCODER(pSingleEncoder);
		m_encAudioBytes += readLen;
		m_tmpBenchData.audioEncTime = (float)(m_encAudioBytes/(double)bytesPerSec);
		
		int64_t encodeBytes = pSingleEncoder->EncodeBuffer(m_pcmBuf, readLen);
		if(encodeBytes > 0) {
			if(m_videoEncs.empty()) {
				if((int)(m_tmpBenchData.audioEncTime)%4 == 0) benchmark();
			} 
		} else {
			ret = -1;
		}
	}
	
	// Judge if need append blank audio samples to the end
	appendBlankAudio(pSingleEncoder);

	//Finshed or failed now
	//m_pAudioDec->CloseAudioReadHandle();
    pSingleEncoder->Stop();
	//free(pOriginAudioBuf); 

	if(ret < 0) {
		SetErrorCode(EC_AUDIO_ENCODER_ERROR);
		m_pAudioDec->Cleanup();
		logger_err(m_logType, "TranscodeAudio failed.\n");
	} else {
		logger_status(m_logType, "Audio transcode have completed.\n");
	}
	
	return ret;
}

THREAD_RET_T CTransWorkerSeperate::transcodeSingleAudioComplex()
{
	int ret = 0;
	CAudioEncoder* pSingleEncoder = m_audioEncs[0];
	
	uint32_t bytesPerSec = m_pAudioDec->GetWavInfo()->bytes_per_second;
	bool bUpdateEveryTime = false;
	size_t clipsCount = m_clipStartSet.size();

	// Encoding loop
	while (ret >= 0 && m_errCode == EC_NO_ERROR) {
		PROCESS_PAUSE_STOP;
		// Process multi source files decoding (Joining)
		if(m_pAudioDec && m_pAudioDec->GetAudioReadHandle() < 0 ) { 
			if(m_bDecodeNext) { 
				if(m_videoEncs.empty()) decodeNext(); 
				Sleep(50);  
			} else {
				break;
			}
		} 
		if(m_lockReadAudio == 1) { 
			if(m_pAudioDec && m_pAudioDec->GetAudioReadHandle()>0) { 
				if(m_pAudioDec->ReadAudioHeader()) { 
					audio_info_t* pTmpAudioInfo = m_pAudioDec->GetAudioInfo(); 
					wav_info_t* pTmpWav = m_pAudioDec->GetWavInfo(); 
					checkAudioParam(pTmpAudioInfo, pTmpWav); 
				} else {break; }
				m_lockReadAudio = 0; 
			} else {
				continue;
			}
		} 

		// Process Splitting
		bool bDiscardData = false;
		if(m_pSplitter) {
			bDiscardData = m_pSplitter->BeginSplitAudio();
		}

		int readLen = 0;
		if(m_pAudioDec && m_pAudioDec->GetAudioReadHandle()>0) { 
			if((readLen = m_pAudioDec->ReadAudio(m_pcmBuf, m_pcmBufSize)) <= 0) {
				if(!m_bDecodeNext) break;
				m_pAudioDec->CloseAudioReadHandle();
				continue;
			}
		}

		if(!m_audioClipEnd) {
			m_encAudioBytes += readLen;
			m_tmpBenchData.audioEncTime = (float)((double)m_encAudioBytes/(double)bytesPerSec);
		}

		if(audioClipIndex < clipsCount) {
			// When audio time is smaller than current clip start, then discard data
			if(m_tmpBenchData.audioEncTime*1000 < m_clipStartSet[audioClipIndex]) {
				bDiscardData = true;
			} else if(m_tmpBenchData.audioEncTime*1000 > m_clipEndSet[audioClipIndex]) {
				// When reach to clip end, then change to next clip
				audioClipIndex++;	
			}
		} else if(clipsCount > 0) {		//audioClipIndex = clipsCount
			// Reach last clip's end, should end
			// if video decoder exist and it's equal to audio decoder, don't close
			// decoder in audio encoding thread (close it in video encoding thread)
			m_audioClipEnd = 1;
			if(m_pVideoDec != m_pAudioDec || m_videoClipEnd) {
				m_pAudioDec->Cleanup();
				break;
			} else {
				bDiscardData = true;
			}
		}

		if(m_videoEncs.empty() && (int)(m_tmpBenchData.audioEncTime)%4 == 0) benchmark();
		if(bDiscardData) continue;	// Don't process data, just discard
		
		CHECK_AUDIO_ENCODER(pSingleEncoder);
		int64_t encodeBytes = pSingleEncoder->EncodeBuffer(m_pcmBuf, readLen);
		if(encodeBytes > 0) {
			if(m_pSplitter) {
				if(bUpdateEveryTime || (int)(m_tmpBenchData.audioEncTime)%2 == 0) {
					m_pSplitter->UpdateAudioTime((int)(m_tmpBenchData.audioEncTime*1000));
					/*if(m_pSplitter->GetUnitType() == SPLIT_UNIT_BYSIZE) {
						int64_t tmpAudioSize = StatFileSize(pSingleEncoder->GetOutFileName().c_str());
						m_pSplitter->UpdateAudioSize(tmpAudioSize);
					}*/
					bUpdateEveryTime = m_pSplitter->EndSplitAudio();
				}
				
			} 
		} else {
			ret = -1;
		}
	}
	
	// Judge if need append blank audio samples to the end
	appendBlankAudio(pSingleEncoder);
	//Finshed or failed now
	//m_pAudioDec->CloseAudioReadHandle();
	pSingleEncoder->Stop();
	if(clipsCount > 0) m_audioClipEnd = 1;

	if(ret < 0) {
		SetErrorCode(EC_AUDIO_ENCODER_ERROR);
		m_pAudioDec->Cleanup();
		logger_err(m_logType, "TranscodeAudio failed.\n");
	} else {
		logger_status(m_logType, "Audio transcode have completed.\n");
	}
	
	return ret;
}

THREAD_RET_T WINAPI CTransWorkerSeperate::encodeAudioThread(void* encodeParam)
{
	multi_thread_param_t* curParam = static_cast<multi_thread_param_t*>(encodeParam);
	CTransWorkerSeperate* theWorker = curParam->worker;
	size_t audioIdex = curParam->curStreamId;
	
	if(curParam) {
		delete curParam;
	}

	if(theWorker) {
		return theWorker->encodeAudio(audioIdex);
	} else {
		logger_err(theWorker->m_logType, "Worker %d audio stream %d transcode failed.\n", theWorker->m_id, (int)curParam->curStreamId);
		return -1;
	}
}

THREAD_RET_T CTransWorkerSeperate::encodeAudio(size_t audioIdx)
{
	int encodeRet = 0;
	
	uint8_t* pOriginAudioBuf = NULL;
	int pcmBufLen = 0;
	int32_t bytesPerSec = 0;
	CAudioEncoder* pEncoder = m_audioEncs[audioIdx];

	if(pEncoder) {
		audio_info_t* pAInfo = pEncoder->GetAudioInfo();
		bytesPerSec = pAInfo->out_srate*pAInfo->out_channels*(pAInfo->bits>>3);
		pcmBufLen = pEncoder->GetFrameLen();
		if(pcmBufLen <= 0) {
			pcmBufLen = bytesPerSec;
			pcmBufLen /= m_audioFractionOfSecond;
		}
		
		pOriginAudioBuf = (uint8_t*)malloc(pcmBufLen);
		if(!pOriginAudioBuf) {
			logger_err(m_logType, "Allocate memory failed in encodeAudio.\n");
			return -1;
		}
	} else {
		logger_err(m_logType, "Allocate memory failed in encodeAudio.\n");
		return -1;
	}

	int bytesRead = -1;
	
	while (encodeRet >= 0) {
		PROCESS_PAUSE_STOP;
		int fdRead = pEncoder->GetReadHandle();
		if(fdRead < 0) {
			encodeRet = -1; break;
		}
		if((bytesRead = SafeRead(fdRead, pOriginAudioBuf, pcmBufLen)) <= 0 ) {
			if(m_bDecodeNext) continue;
			else break;
		}
		
		int64_t encodeBytes = pEncoder->EncodeBuffer(pOriginAudioBuf, bytesRead);
		if(audioIdx == 0 && encodeBytes > 0) {
			m_encAudioBytes += bytesRead;
			m_tmpBenchData.audioEncTime = (float)(m_encAudioBytes/(double)bytesPerSec);
			if(m_videoEncs.empty()) {
				if((int)(m_tmpBenchData.audioEncTime)%4 == 0) benchmark();
			} 
		}
	}

	pEncoder->Stop();
	pEncoder->CloseReadHandle();
	if(pOriginAudioBuf) {
		free(pOriginAudioBuf);
		pOriginAudioBuf = NULL;
	}

	return encodeRet;
}

THREAD_RET_T WINAPI CTransWorkerSeperate::encodeVideoThread(void* encodeParam)
{
	int encodeRet = -1;
	multi_thread_param_t* curParam = static_cast<multi_thread_param_t*>(encodeParam);
	CTransWorkerSeperate* theWorker = curParam->worker;
	if(theWorker) {
		encodeRet = theWorker->encodeVideo(curParam->curStreamId);
	}
	
	if(curParam) delete curParam;
	if(encodeRet < 0) {
		logger_err(theWorker->m_logType, "Worker %d video stream %d transcode failed.\n", theWorker->m_id, (int)curParam->curStreamId);
		if(theWorker) theWorker->SetErrorCode(EC_AUDIO_ENCODER_ERROR);
	}
	return encodeRet;
}

THREAD_RET_T CTransWorkerSeperate::encodeVideo(size_t videoIdx)
{
	#define FAIL_INFO(err) logger_err(m_logType, err); encodeRet=-1; break;
	int encodeRet = 0;
	CVideoEncoder* pVideoEncode = m_videoEncs.at(videoIdx);
	uint8_t* pVideoBuf = NULL;

	do {
		pVideoBuf = (uint8_t*)malloc(m_frameBufSize);
		if(!pVideoBuf) {
			FAIL_INFO("Allocate video buffer failed.\n");
		}

		double framesSec = 0.f;
		if(m_tmpBenchData.fpsNum > 0) {
			framesSec = (double)m_tmpBenchData.fpsDen/m_tmpBenchData.fpsNum;
		}

		// Encoding loop
		int fdRd = pVideoEncode->GetReadHandle();
		if(fdRd < 0) {
			FAIL_INFO( "Video read handle is invalid.\n");
		}
		while (true) {	// Video file reach it's end
			PROCESS_PAUSE_STOP;
			//1. read framebuf from ring buffer
			if(SafeRead(fdRd, pVideoBuf, m_frameBufSize) != m_frameBufSize) {
				break;
			}

			//2. then do filter
			uint8_t* pFrameBuf = pVideoEncode->FilterFrame(pVideoBuf, m_frameBufSize);
			if (pFrameBuf == NULL) {	//skiped or failed
				continue;
			}

			//3. encode at last
			int frameCount = pVideoEncode->EncodeFrame(pFrameBuf, -1 /*TODO:BufSize*/);
			if(frameCount <= 0) {
				FAIL_INFO("Encoding failed.\n");
			} else if(videoIdx == 0){
				m_encodedFrames++;
			}
			if(videoIdx == 0 && frameCount%UPDATE_FRAMES_INTERVAL == 0) {
				m_tmpBenchData.videoEncTime = (float)(m_encodedFrames*framesSec);
				benchmark();
			}
		}
	} while (false);

	pVideoEncode->StopThumbnail();
	pVideoEncode->Stop();
	if(pVideoBuf) free(pVideoBuf);
	if(encodeRet == 0) pVideoEncode->CloseReadHandle();
	//pVideoEncode->SetVideoFilter(NULL);			// 把VideoFilter从Video Encoder 中去掉

	#undef FAIL_INFO
	return encodeRet;
}

CAudioEncoder* CTransWorkerSeperate::createAudioEncoder(audio_encoder_t encType, audio_format_t aencFormat)
{
	CAudioEncoder* pAudioEnc = NULL;
	int audioEncNum = m_audioEncs.size();
	std::string tmpFile = m_streamFiles.GetStreamFileName(ST_AUDIO, aencFormat, audioEncNum);
	
	if(tmpFile.empty()) {
		logger_err(m_logType, "Can't get temp audio stream file name.\n");
		return NULL;
	}

	switch (encType) {
	case AE_FAAC:
#ifdef HAVE_LIBFAAC
		pAudioEnc = new CFaacEncode(tmpFile.c_str()); break;
#else
		pAudioEnc = new CFaacCLI(tmpFile.c_str()); break;
#endif
	case AE_DOLBY:
#if defined(HAVE_LIBEAC3)
		pAudioEnc = new CEac3Encode(tmpFile.c_str()); break;
#else
		pAudioEnc = new CFFmpegAudioEncoder(tmpFile.c_str()); break;
#endif
		break;
	case AE_MP3:
		pAudioEnc = new CMp3Encode(tmpFile.c_str()); break;
	case AE_FFMPEG:
		pAudioEnc = new CFFmpegAudioEncoder(tmpFile.c_str()); break;
#ifdef HAVE_WMENCODER
	case AE_WMA:
		pAudioEnc = new CWMAudioEncoder(tmpFile.c_str()); break;
#endif
	case AE_NEROREF:
		pAudioEnc = new CNeroAudioEncoder(tmpFile.c_str()); break;
	default:
		logger_err(m_logType, "unsupported Audio encoder (%d)\n", encType);
		break;
	}

	if(pAudioEnc != NULL) {
		pAudioEnc->SetWorkerId(GetId());
		m_audioEncs.push_back(pAudioEnc);
	}
	
	logger_status(m_logType, "Create Audio Encoder OK (encType:%d, size:%d)\n", encType, (int)m_audioEncs.size());
	return pAudioEnc;
}

CVideoEncoder* CTransWorkerSeperate::createVideoEncoder(video_encoder_t encType, video_format_t vencFormat, vf_type_t vfType)
{
	CVideoEncoder* pVideoEnc = NULL;
	std::string tmpFile;
	
	int videoEncNum = m_videoEncs.size();
	if(encType == VE_VFW) {		// VFW Matrox generate avi file
		tmpFile = m_streamFiles.GetStreamFileName(ST_VIDEO, vencFormat, videoEncNum, ".avi");
	} else {
		tmpFile = m_streamFiles.GetStreamFileName(ST_VIDEO, vencFormat, videoEncNum);
	}
	
	if(tmpFile.empty()) {
		logger_err(m_logType, "Can't get temp video stream file name.\n");
		return NULL;
	} 
	
	switch(encType) {
#ifdef PRODUCT_MEDIACODERDEVICE		// Device version only support H.264 format
#if defined(WIN32)
		case VE_CUDA264:
			//pVideoEnc = new CCuda264Encode(tmpFile.c_str());
			pVideoEnc = new CCudaCLIEncoder(tmpFile.c_str());
			m_bUseCudaEncoder = true;
			break;
#endif
		case VE_X264:
#ifdef HAVE_LIBX264
			pVideoEnc = new CX264Encode(tmpFile.c_str());
#else
			pVideoEnc = new CX264CLIEncoder(tmpFile.c_str());
#endif
			break;
#else 

#if defined(WIN32)
		case VE_CUDA264:
			#ifdef  HAVE_LIBCUDA264
			pVideoEnc = new CCuda264Encode(tmpFile.c_str());
			#else
			pVideoEnc = new CCudaCLIEncoder(tmpFile.c_str());
			#endif
			m_bUseCudaEncoder = true;
			break;
#endif
		case VE_X264:
#ifdef HAVE_MINDERIN_ENCODER		// For minderin h.264 encoder
			pVideoEnc = new CMindCLIEncoder(tmpFile.c_str());
			break;
#endif
			
#ifdef HAVE_LIBX264
			pVideoEnc = new CX264Encode(tmpFile.c_str());
#else
			pVideoEnc = new CX264CLIEncoder(tmpFile.c_str());
#endif
			break;
#ifdef HAVE_LIBX265
		case VE_X265:
			pVideoEnc = new CX265Encode(tmpFile.c_str());
			break;
#endif
		case VE_XVID:
			pVideoEnc = new CXvidEncode(tmpFile.c_str());
			break;
		case VE_FFMPEG:
			pVideoEnc = new CFFmpegVideoEncoder(tmpFile.c_str());
			break;
#ifdef HAVE_VFWENCODER
		case VE_VFW:
			pVideoEnc = new CVfwEncoder(tmpFile.c_str());
			break;
#endif
#ifdef HAVE_WMENCODER
		case VE_WM:
			pVideoEnc = new CWVC1Encoder(tmpFile.c_str());
			break;
#endif

#ifdef HAVE_VC1ENCODER
		case VE_VC1:
			pVideoEnc = new CWVC1Encoder(tmpFile.c_str());
			break;
#endif

#if defined(HAVE_MII_ENCODER)
		case VE_MII:
			pVideoEnc = new CMiiEncode(tmpFile.c_str());
			break;
#endif

#if defined(HAVE_CAVS)
		case VE_CAVS:
			pVideoEnc = new CCAVSVideoEncoder(tmpFile.c_str());
			break;
#endif

#endif
		default:
			logger_err(m_logType, "Unsupported Video encoder (%d)\n", encType);
			break;
	}
	
	if(pVideoEnc != NULL) {
		pVideoEnc->SetWorkerId(GetId());
		pVideoEnc->SetVideoFilterType(vfType);
		m_videoEncs.push_back(pVideoEnc);
	}

	logger_status(m_logType, "Create Video Encoder OK (encType:%d, size:%d)\n", encType, (int)m_videoEncs.size());
	return pVideoEnc;
}

bool CTransWorkerSeperate::createVideoFilter(CVideoEncoder* pVideoEncode)
{
	bool ret = true;
	vf_type_t reqVfType = pVideoEncode->GetVideoFilterType();
	vf_type_t realVfType = reqVfType;
	
	do {
		if(reqVfType == VF_ENCODER) break;
		// Check whether need video filter (Only if source file info exists)
		video_info_t* pVinfo = pVideoEncode->GetVideoInfo();
		if(m_srcVideoAttrib && (reqVfType == VF_NONE || reqVfType == VF_AUTO) &&
			pVinfo != NULL && (pVinfo->res_out.width > 0 || pVinfo->fps_out.num > 0)) {
			if(m_srcVideoAttrib->width != pVinfo->res_out.width || 
				m_srcVideoAttrib->height != pVinfo->res_out.height ||
				m_srcVideoAttrib->fps_num != pVinfo->fps_out.num ||
				m_srcVideoAttrib->fps_den != pVinfo->fps_out.den) {
				if(m_videoEncs.size() == 1) {
					realVfType = VF_DECODER ;
				} else {
					realVfType = VF_CPU;
				}
			}
		}

		if(!m_srcVideoAttrib && (pVinfo->res_out.width > 0 || pVinfo->fps_out.num > 0) ) {
			realVfType = VF_DECODER ;
		}

		if (realVfType == VF_NONE) break;

		if (realVfType == VF_DECODER) {
#if 1
			if(m_videoEncs.size() == 1) {
				m_bEnableDecoderVf = true;
				break;
			} else {
				logger_warn(m_logType, "Can't use decoder to do video filter when multi video output, change to CPU filter.\n");
				realVfType = VF_CPU;
			}
#else
			m_bEnableDecoderVf = true;
			break;
#endif
		}

		/*
		CVideoFilter* pFilter = CTransnodeUtils::CreateVideoFilter();
		if(!pFilter) {
			ret = false;
			break;
		}

		switch (realVfType)
		{
		case VF_CPU:								//Try init VFCPU;
			ret = pFilter->Init(FT_Resample | FS_NoCuda, 0); 
			break;
		case VF_CUDA:								//Try init VFCUDA
			ret = pFilter->Init(FT_Resample, 0); 
			break;
		default:
			ret = false;
			break;
		}

		if (ret == false) {
			CTransnodeUtils::DeleteVideoFilter(pFilter);
			break;
		}

		pVideoEncode->SetVideoFilter(pFilter);
		m_bEnableDecoderVf = false;*/
	} while (false);
	
	pVideoEncode->SetVideoFilterType(realVfType);
	if(!ret) {
		logger_err(m_logType, "Try to use Video Filter %d, but failed\n", realVfType);
	}

	return ret;
}

bool CTransWorkerSeperate::initVideoFilter(CVideoEncoder* pVideoEncode)
{
	if (pVideoEncode == NULL) return false;

	CVideoFilter* pFilter = pVideoEncode->GetVideoFilter();
	if(pFilter) {
		video_info_t* pvinfo = pVideoEncode->GetVideoInfo();
		if(!pvinfo) {
			logger_err(m_logType, "Video info is not valid.\n");
			return false;
		}
		const int frameWindow = 1/*pvinfo->fps_in.num/pvinfo->fps_in.den*/;
		float fpsRatio = pvinfo->fps_out.num/(float)pvinfo->fps_out.den*pvinfo->fps_in.den/(float)pvinfo->fps_in.num;

		// Try to prepare filter
		bool ret = pFilter->Prepare(0, frameWindow, pvinfo->res_in.width, pvinfo->res_in.height,
			pvinfo->res_out.width, pvinfo->res_out.height, fpsRatio);
		if(!ret && pVideoEncode->GetVideoFilterType() == VF_CUDA) {	// If CUDA filter prepare failed, downgrade to cpu filter 
			pFilter->Uninit();
			pFilter->Init(FT_Resample | FS_NoCuda | FT_Pullup, 0);
			ret = pFilter->Prepare(0, frameWindow, pvinfo->res_in.width, pvinfo->res_in.height,
				pvinfo->res_out.width, pvinfo->res_out.height, fpsRatio);
		}

		if(!ret) {	// If filter init failed, clean filter.
			CTransnodeUtils::DeleteVideoFilter(pFilter);
			pVideoEncode->SetVideoFilter(NULL);
			logger_err(m_logType, "Video prepare failed.\n");
			return false;
		}
	}

	return true;
}

void CTransWorkerSeperate::uninitVideoFilter(CVideoEncoder* pVideoEncode)
{
	CVideoFilter* pFilter = pVideoEncode->GetVideoFilter();
	if (pFilter == NULL) return;
	CTransnodeUtils::DeleteVideoFilter(pFilter);
	pVideoEncode->SetVideoFilter(NULL);
}

#define MAX_AUDIO_TRACK_NUM 2
#define CREATE_AUX_AUDIO_ENCODER(bCreate, idx)  CAudioEncoder* pEnc = NULL; \
	if(bCreate) { \
	pEnc = createAudioEncoder(pFirstAudioInfo->encoder_type, pFirstAudioInfo->format); \
	CXMLPref* pTmpPref = pFirstPref->Clone(); \
	pEnc->SetAudioInfo(*pFirstAudioInfo, pTmpPref); \
	m_auxAudioPrefs.push_back(pTmpPref); \
	CFileQueue::queue_item_t fileItem = {pEnc->GetOutFileName(), ST_AUDIO,NULL,pEnc->GetAudioInfo()}; \
	m_streamFiles.Enqueue(fileItem); \
	if(m_muxers[0]) m_muxers[0]->AddAudioStream(idx); \
	} else {  \
		pEnc = m_audioEncs[idx]; \
	}   pEnc->SetSourceIndex(idx); 

bool CTransWorkerSeperate::initSrcSubtitleAttrib(StrPro::CXML2* mediaInfo, CXMLPref* pVideoPref)
{
	mediaInfo->goRoot();
	bool findSuitableBmpSub = false;
	void* subNode = mediaInfo->findChildNode("subtitle");
	int subIndex = pVideoPref->GetInt("overall.subtitle.sid");
	while(subNode) {
		int aIndex = mediaInfo->getChildNodeValueInt("index");
		if(subIndex == aIndex) {
			const char* pSubType = mediaInfo->getChildNodeValue("codec");
			if(pSubType && (!_stricmp(pSubType, "pgssub") || !_stricmp(pSubType, "dvdsub"))) {
				findSuitableBmpSub = true;
			} 
			break;
		}
		//mediaInfo->getChildNodeValue("lang");
		subNode = mediaInfo->findNextNode("subtitle");
    }

	if(!findSuitableBmpSub) {	// Disable embed subtitle
		pVideoPref->SetInt("overall.subtitle.sid", -1);
	}
	return true;
}

bool CTransWorkerSeperate::initSrcAudioAttrib(StrPro::CXML2* mediaInfo)
{
	// Parse all source audio track attributes
	mediaInfo->goRoot();
	void* audioNode = mediaInfo->findChildNode("audio");
	attr_audio_t* pAudioAttrib = NULL;
	size_t audioIdx = 0;
	while(audioNode) {
		if(m_srcAudioAttribs.size() > audioIdx) {
			pAudioAttrib = m_srcAudioAttribs[audioIdx];
		} else {
			pAudioAttrib = new attr_audio_t;
			m_srcAudioAttribs.push_back(pAudioAttrib);
		}

		memset(pAudioAttrib, 0, sizeof(attr_audio_t));
		pAudioAttrib->id = -1;
		parseMediaAudioInfoNode(mediaInfo, pAudioAttrib);
		audioNode = mediaInfo->findNextNode("audio");
		audioIdx++;
    }
	if(m_audioEncs.empty()) return true;

	bool srcHasMultiAudioTrack = (audioIdx > 1);

	CAudioEncoder* pFirstAudio = m_audioEncs[0];
	CXMLPref* pFirstPref = pFirstAudio->GetAudioPref();
	audio_info_t* pFirstAudioInfo = pFirstAudio->GetAudioInfo();

	// Get output audio track setting
	int outAudioTrackNum = pFirstPref->GetInt("extension.audio.tracknum");
	int track1Index = pFirstPref->GetInt("extension.audio.track1");
	int track2Index = pFirstPref->GetInt("extension.audio.track2");
	// Get audio channel map to track info
	bool bMapCh2Track = pFirstPref->GetBoolean("extension.channelmap.enable");
	bool bLeftToTrack1 = true;
	if(bMapCh2Track) {
		int leftMapIndex = pFirstPref->GetInt("extension.channelmap.left");
		if(leftMapIndex == 1) {
			bLeftToTrack1 = false;
		}
	}

	if(outAudioTrackNum == 0) {				// Single audio track output
		outAudioTrackNum = 1;
		bMapCh2Track = false;
	} else if (outAudioTrackNum == 1) {		// Maintain source audio track output
		outAudioTrackNum = audioIdx;		// Original audio track num
		bMapCh2Track = false;
	} //else if (outAudioTrackNum == 2) {	// Dual audio track output

	if(srcHasMultiAudioTrack) {
		bMapCh2Track = false;		// If has multi audio track, disable mapCh2Track
	} else if(pAudioAttrib && pAudioAttrib->channels < 2){
		bMapCh2Track = false;		// If audio track channels < 2, disable mapCh2Track
	}

	m_bMultiAudioTrack = outAudioTrackNum > 1;
	// Parse Other Audio info
	if(m_bMultiAudioTrack) {		// Output multi audio track
		if(bMapCh2Track) {			// Process channels mapping to track 1
			pFirstPref->SetBoolean("audiofilter.channels.enabled", true);
			int channelNum = pFirstAudioInfo->out_channels;
			if(channelNum == 0) channelNum = 2;
			pFirstPref->SetInt("audiofilter.channels.output", channelNum);
			pFirstPref->SetInt("audiofilter.channels.routes", channelNum);
			int sourceChannelNo = 1;
			if(!bLeftToTrack1) {
				sourceChannelNo = 2;
			} 
			for (int i=0; i<channelNum; ++i) {
				char strKey[64] = {0};
				sprintf(strKey, "audiofilter.channels.channel%d", i+1);
				pFirstPref->SetInt(strKey, sourceChannelNo);
			}
		}

		bool bCreateAudioEnc = m_audioEncs.size() < 2;
		for(int aIdx = 1; aIdx < outAudioTrackNum; ++aIdx) {
			CREATE_AUX_AUDIO_ENCODER(bCreateAudioEnc, aIdx);
			
			if (bMapCh2Track) {	// Process channels mapping to track 2
				CXMLPref* pPref = pEnc->GetAudioPref();
				audio_info_t* pinfo = pEnc->GetAudioInfo();
				int channelNum = pinfo->out_channels;
				if(channelNum == 0) channelNum = 2;
				int sourceChannelNo = 1;
				if(bLeftToTrack1) {
					sourceChannelNo = 2;
				}
				for (int i=0; i<channelNum; ++i) {
					char strKey[64] = {0};
					sprintf(strKey, "audiofilter.channels.channel%d", i+1);
					pPref->SetInt(strKey, sourceChannelNo);
				}
			}
		}
	}

	// Set audio source param
	if(!m_srcAudioAttribs.empty()) {
		for(size_t i=0; i<m_audioEncs.size(); ++i) {
			CAudioEncoder* pAudioEnc = m_audioEncs[i];
			if(!pAudioEnc) continue;

			attr_audio_t* pAttrib = NULL;
			if(i >= m_srcAudioAttribs.size()) {		// Only one audio track, but map2chTo2Track
				pAttrib = m_srcAudioAttribs[0];
			} else {	// Select source audio track index for every output audio track
				if(i == 0) {
					if(track1Index >= 0 && m_srcAudioAttribs.size() > track1Index) {
						pAttrib = m_srcAudioAttribs[track1Index];
					}
				} else if (i == 1) {
					if(track2Index >= 0 && m_srcAudioAttribs.size() > track2Index) {
						pAttrib = m_srcAudioAttribs[track2Index];
					}
				}

				// Only one audio encoder, utput one audio track(Most situation)
				if(!pAttrib) {
					pAttrib = m_srcAudioAttribs[i];
					while(pAttrib->samplerate == 0) {	// If cur audio track is invalid, try next
						i++;
						if(i < m_srcAudioAttribs.size()) {
							pAttrib = m_srcAudioAttribs[i];
						} else {
							break;
						}
					}
				}
			}
			audio_info_t* pinfo = pAudioEnc->GetAudioInfo();
			setAudioEncAttrib(pinfo, pAudioEnc->GetAudioPref(), pAttrib);
		}
	}

	return true;
}
#undef CREATE_AUX_AUDIO_ENCODER

bool CTransWorkerSeperate::initSrcVideoAttrib(StrPro::CXML2* mediaInfo)
{
	// Parse video info
	mediaInfo->goRoot();
	// Find first invalid video（Some ts file will contain several invalid video tracks）
	size_t videoIdx = 0;
	void* videoNode = mediaInfo->findChildNode("video");
	while(videoNode) {
		if(!m_srcVideoAttrib) {
			m_srcVideoAttrib = new attr_video_t;
		}	
		memset(m_srcVideoAttrib, 0, sizeof(attr_video_t));
		m_srcVideoAttrib->id = videoIdx;
		int dar_num = mediaInfo->getChildNodeValueInt("dar_num");
		parseMediaVideoInfoNode(mediaInfo, m_srcVideoAttrib);
		// 有flv视频有Sorenson Spark和H.264两个视频轨，其中有效的一个dar是有效值，另外一个为0:0,无效。
		if(m_srcVideoAttrib->width > 0 && dar_num > 0) break;
		videoNode = mediaInfo->findNextNode("video");
		videoIdx++;
	}

	// Parse source media container format
	mediaInfo->goRoot();
	if(mediaInfo->findChildNode("general") && m_srcVideoAttrib) {
		memset(m_srcVideoAttrib->fileFormat, 0, CODEC_NAME_LEN);
		strncpy(m_srcVideoAttrib->fileFormat, SAFESTRING(mediaInfo->getChildNodeValue("container")), CODEC_NAME_LEN);
	}

	// Set video source param
	if(m_srcVideoAttrib) {
		for(size_t i=0; i<m_videoEncs.size(); ++i) {
			CVideoEncoder* pVideoEnc = m_videoEncs[i];
			if(!pVideoEnc) continue;
			setVideoEncAttrib(pVideoEnc->GetVideoInfo(), pVideoEnc->GetVideoPref(), m_srcVideoAttrib);
		}
	}
	return true;
}

bool CTransWorkerSeperate::initAVSrcAttrib(StrPro::CXML2* mediaInfo)
{
	if(!m_audioEncs.empty() || m_bCopyAudio) {
		initSrcAudioAttrib(mediaInfo);
	}
	if(!m_videoEncs.empty() || m_bCopyVideo) {
		initSrcVideoAttrib(mediaInfo);
	}
	if(!m_videoEncs.empty()) {
		initSrcSubtitleAttrib(mediaInfo, m_videoEncs[0]->GetVideoPref());
	}
	return true;
}

//#define HALF_HOUR_DUR 1800000

bool CTransWorkerSeperate::doMux()
{
	int ret = MUX_ERR_SUCCESS;
	if(m_pSplitter) {	// If need split media
		m_muxingCount++;
		std::string postfix = m_pSplitter->GetPostfix();
		m_streamFiles.ModifyDestFileNames(postfix.c_str(), m_muxingCount, m_pSplitter->GetPrefix().c_str());
	}

	for (size_t i=0; i<m_muxers.size(); ++i) {
		CMuxer* pmuxer = m_muxers.at(i);
		pmuxer->SetFileQueue(&m_streamFiles);
		ret = pmuxer->Mux();	// Block function call
		if(ret == MUX_ERR_NOT_ENOUGH_SPACE) {
			SetErrorCode(EC_NOT_ENOUGH_DISK_SPACE);
		} else if(ret == MUX_ERR_INVALID_FILE) {
			SetErrorCode(EC_BAD_AV_DATA);
		} else if(ret != MUX_ERR_SUCCESS) {
			SetErrorCode(EC_MUXER_ERROR);
		} 
	}

	std::vector<std::string> destFiles = m_streamFiles.GetDestFiles();
	if(destFiles.empty()) {
		logger_err(m_logType, "Destinate files is empty.\n");
		return false;
	}

	// Backup corrupt mp4 file and h264/aac files(Debug purpose)
	if(ret == MUX_ERR_INVALID_FILE) {
		std::string mp4File = destFiles[0];
		std::string videoFile = m_streamFiles.GetFirst(ST_VIDEO)->fileName;
		std::string audioFile = m_streamFiles.GetFirst(ST_AUDIO)->fileName;
		std::string leafName;
		if(StrPro::StrHelper::getFileLeafName(mp4File.c_str(), leafName)) {
			TsMoveFile(mp4File.c_str(), leafName.c_str());
		}
		if(StrPro::StrHelper::getFileLeafName(videoFile.c_str(), leafName)) {
			TsMoveFile(videoFile.c_str(), leafName.c_str());
		}
		if(StrPro::StrHelper::getFileLeafName(audioFile.c_str(), leafName)) {
			TsMoveFile(audioFile.c_str(), leafName.c_str());
		}
	}

	if(ret != MUX_ERR_SUCCESS) return false;

	//CPlaylistGenerator* playlist = CTransnodeUtils::GetPlaylist(m_playlistKey);
	if(m_pPlaylist) m_pPlaylist->AddPlaylistItem(destFiles[0]);

	if(m_pSplitter) {	// If need split media
		//m_pSplitter->AddDestSegFile(destFiles.at(0).c_str());
		if(!m_videoEncs.empty()) {
			if (!m_streamFiles.ModifyVideoStreamFile(m_muxingCount-1)) {
				logger_warn(m_logType, "ModifyVideoStreamFile() return false.\n");
			}
		}
		if(!m_audioEncs.empty()) {
			if (!m_streamFiles.ModifyAudioStreamFile(m_muxingCount-1)) {
				logger_warn(m_logType, "ModifyAudioStreamFile() return false.\n");
			}
		}
	} 
	return true;
}

void CTransWorkerSeperate::benchmark()
{
	if(m_lastTime == 0) {
		m_lastTime = m_startTime;
		return;
	}
	static const int outputInterval = 3000;

	int64_t curTime = GetTickCount();
	int64_t elapsedTime = curTime - m_startTime;
	if(elapsedTime < 2000) return;		// less than 2 seconds
	int64_t deltaTime = curTime - m_lastTime;
	if(deltaTime < outputInterval) return;		// less than 2 seconds

	if(!m_videoEncs.empty()) {		// If there is video encoding
		float encodeFps = (m_encodedFrames-m_tmpBenchData.lastFrameNum)*1000.f/deltaTime;
		float averageFps = m_encodedFrames*1000.f / elapsedTime;
		
		char fpsStr[16] = {0};
		sprintf(fpsStr, "%1.1f FPS", encodeFps);
		m_strEncodeSpeed = fpsStr;
		#ifndef PRODUCT_TRANSERVER
		logger_info(m_logType, "Worker %d speed: %1.2f(%1.2f), progress:%1.1f%%, encoded frames: %d.\n", m_id, encodeFps, averageFps, m_progress, m_encodedFrames);
		#endif
#ifdef ENABLE_HTTP_STREAMING		// Control streaming speed(not too fast)
		int deltaTime = (int)(m_tmpBenchData.videoEncTime*1000 - elapsedTime) - DEFAULT_SEGMENT_DURATION*2000;
		if(deltaTime > 0) Sleep(deltaTime);
#endif
		m_tmpBenchData.lastFrameNum = m_encodedFrames;
		m_progress = m_tmpBenchData.videoEncTime*100000 / m_tmpBenchData.mainDur;
	} else if(!m_audioEncs.empty()) {	// If there is only audio encoding
		float audioSpeedX = m_tmpBenchData.audioEncTime*1000/elapsedTime;
		char speedStr[16] = {0};
		sprintf(speedStr, "%1.1f X", audioSpeedX);
		m_strEncodeSpeed = speedStr;
		#ifndef PRODUCT_TRANSERVER
		logger_info(m_logType, "Worker %d speed: %1.2fX, progress:%1.1f%%.\n", m_id, audioSpeedX, m_progress);
		#endif
		
		m_progress = m_tmpBenchData.audioEncTime*100000 / m_tmpBenchData.mainDur;
	}
	
	// Update elapsed time (secs)
	m_elapseSecs = (int)(elapsedTime/1000.f);
	m_lastTime = curTime;
}

void CTransWorkerSeperate::CleanUp()
{
	deleteDecoders();
	// Multi audio track process related
	while(m_auxAudioPrefs.size()) {
		CXMLPref* pPref = m_auxAudioPrefs.back();
		m_auxAudioPrefs.pop_back();
		delete pPref;
	}
	
	cleanAudioEncoders();
	cleanVideoEncoders();
	cleanMuxers();

	if(m_pSplitter) {
		delete m_pSplitter;
		m_pSplitter = NULL;
	}

	if(m_pImgTail) {
		delete m_pImgTail;
		m_pImgTail = NULL;
	}

	if(m_pCopyVideoInfo) {
		delete m_pCopyVideoInfo;
		m_pCopyVideoInfo = NULL;
	}

	if(m_pCopyAudioInfo) {
		delete m_pCopyAudioInfo;
		m_pCopyAudioInfo = NULL;
	}

	// Reset parameters
	m_muxingCount = 0; m_lockReadAudio = 0; m_lockReadVideo = 0;
	m_bDecodeNext = false;  m_encoderPass = 1; m_bCopyAudio = false;
	m_bCopyVideo = false; m_bMultiAudioTrack = false;
	CTransWorker::CleanUp();
}

void CTransWorkerSeperate::cleanAudioEncoders()
{
	while (m_audioEncs.size()) {
		CAudioEncoder *ae = m_audioEncs.back();
		m_audioEncs.pop_back();
		delete ae;
	}
	m_audioEncs.clear();
}

void CTransWorkerSeperate::cleanVideoEncoders()
{
	while (m_videoEncs.size()) {
		CVideoEncoder *ve = m_videoEncs.back();
		m_videoEncs.pop_back();
		uninitVideoFilter(ve);
		delete ve;
	}
	m_videoEncs.clear();
}

void CTransWorkerSeperate::cleanMuxers()
{
	while (m_muxers.size()) {
		CMuxer *pmuxer = m_muxers.back();
		m_muxers.pop_back();
		delete pmuxer;
	}
	m_streamFiles.ClearDestFiles();
}

//temp solution
bool CTransWorkerSeperate::ParseSetting()
{	
	int i = 0;
	bool ret = true;
#define FAIL_INFO(err) logger_err(m_logType, err); ret=false; break;

	do {
		CStreamPrefs* pStreamPref = m_pRootPref->GetStreamPrefs();
		if (!pStreamPref) {
			FAIL_INFO("Stream pref is NULL.\n");
		}

		if(!(ret = parseBasicInfo())) {
			break;
		}

		// Clean up outdated temp file
		m_streamFiles.CleanTempDir();

		// Clear relative dir for current task
		m_streamFiles.SetRelativeDir("");
		
		//char* tmpStr = NULL;
		CXMLPref* pTaskPref = NULL;
		for(i = 0; i < pStreamPref->GetAudioCount(); ++i) {
			CXMLPref* audioPref = pStreamPref->GetAudioPrefs(i);
			if(pTaskPref == NULL) pTaskPref = audioPref;
			//audioPref->Dump(&tmpStr);
			m_bAutoVolumeGain = audioPref->GetBoolean("audiofilter.volume.autoGain");
			m_fVolumeNormalDB = audioPref->GetFloat("audiofilter.volume.standard");
			m_bCopyAudio = audioPref->GetBoolean("overall.audio.copy");
			if(m_bCopyAudio) break;
			if(!this->setAudioPref(audioPref)) {
				FAIL_INFO("Set audio pref failed.\n");
			}
		}
		
		// If video format is FLV/H263P, use ffmpeg to mux
		bool isFFMPEGVideo = false;
		for(i = 0; i < pStreamPref->GetVideoCount(); ++i) {
			CXMLPref* videoPref = pStreamPref->GetVideoPrefs(i);
			if(pTaskPref == NULL) pTaskPref = videoPref;
			//videoPref->Dump(&tmpStr);
			video_format_t  encFormat = (video_format_t)videoPref->GetInt("overall.video.format");
			if(encFormat == VC_FLV /*|| encFormat == VC_H263*/ || encFormat == VC_H263P) isFFMPEGVideo = true;

			m_bCopyVideo = videoPref->GetBoolean("overall.video.copy");
			m_bLosslessClip = videoPref->GetBoolean("overall.task.losslessClip");

			if(m_bCopyVideo || m_bLosslessClip) break;
			if(!this->setVideoPref(videoPref)) {
				FAIL_INFO("Set video pref failed.\n");
			}
		}
		
		// Parse watch folder path before set muxer preference
		parseWatchfoderConfig(pTaskPref);
		for (i = 0; i<pStreamPref->GetMuxerCount(); ++i) {
			CXMLPref* muxerPref = pStreamPref->GetMuxerPrefs(i);
			if(pTaskPref == NULL) pTaskPref = muxerPref;

			if(isFFMPEGVideo) {		// If video stream target format is flv, then use ffmpeg to mux
				muxerPref->SetInt("overall.container.muxer", MUX_FFMPEG);
			}
			
			if(!this->setMuxerPref(muxerPref)) {
				FAIL_INFO("SetMuxerPref Failed.\n");
			}
		}
		
		if(pTaskPref == NULL) {
			FAIL_INFO("No Audio/Video/Muxer pref, bad task preset.\n");
		}

		// Parse playlist setting
		parsePlaylistConfig(pTaskPref);

		// If keep temp file
		if(pTaskPref->GetBoolean("overall.task.keepTemp")) {
			m_streamFiles.SetKeepTemp(true);
		} else {
			m_streamFiles.SetKeepTemp(false);
		}

		// Parse clips setting
		parseClipConfig(pTaskPref);
		
		// Parse media info and get duration
		StrPro::CXML2* pMediaPref = m_pRootPref->GetSrcMediaInfoDoc();
		// Init audio and video info according to mediainfo
		if(!pMediaPref || !setSourceAVInfo(pMediaPref)) {
			SetErrorCode(EC_INVILID_VIDEO_ATTRIB);
			ret = false;  break;
		}

		if(!parseDurationInfo(pTaskPref, pMediaPref)) {
			ret = false;  break;
		}
		// After parse duration, if use decoder to trim then clear clip sets.
		//if(pTaskPref->GetBoolean("overall.task.decoderTrim")) {
		//	m_clipStartSet.clear();
		//	m_clipEndSet.clear();
		//}

		m_streamFiles.RewindSrcFiles();
		m_streamFiles.RewindDestFiles();

		// Parsing segment settings
		parseSegmentConfig(pTaskPref);

		// Parsing image tail
		parseImageTailConfig(pTaskPref);

		// Ignore error code (if the task pref set it then use the value from taks preset
		//  else use config value)
		int ignoreErrIdx = pTaskPref->GetInt("overall.task.ignoreError");
		if(ignoreErrIdx >= 0) {
			CWorkManager::GetInstance()->SetIgnoreErrorCode(ignoreErrIdx);
		}
	} while (false);
	
	if(!ret) {
		SetState(TASK_ERROR);
		SetErrorCode(EC_INVALID_SETTINGS);
	}
#undef FAIL_INFO
	return ret;
}

// New decoder selection strategy(Mainly use ffmpeg 2012-09-18)
void CTransWorkerSeperate::autoSelectBestDecoder2(const char* srcFile, CXMLPref* pVideoPref, CXMLPref* pAudioPref)
{
#ifdef NOT_USE_AVS
	return;
#endif
	m_videoDecType = VD_FFMPEG;
	m_audioDecType = AD_FFMPEG;
	return;

	// Change only audio decoder
	/*if(m_videoEncs.empty()) {
		m_audioDecType = AD_FFMPEG;
		return;
	}

	// Select decoder according to file extension
	const char* ext = strrchr(srcFile, '.');
	if(!ext) return;

	// If video is vfr, use FFMpeg to decode
	if(m_srcVideoAttrib && m_srcVideoAttrib->is_vfr) {	// This situation has top priority
		m_videoDecType = VD_FFMPEG;
		m_audioDecType = AD_FFMPEG;
		return;
	}

	//|| (m_srcVideoAttrib && m_srcVideoAttrib->interlaced > 0
	//	&& !_stricmp(m_srcVideoAttrib->codec, "AVC"))  (This situation has been fixed in ffmpeg)
	if(!_stricmp(ext,".iso") ) {
		// FFMpeg decode interlaced h264 in an awkard way
		pVideoPref->SetBoolean("videosrc.mencoder.lavfdemux", true);
		m_videoDecType = VD_MENCODER;
		m_audioDecType = AD_MENCODER;
	} else {
		m_videoDecType = VD_FFMPEG;
		m_audioDecType = AD_FFMPEG;
		//if(m_srcVideoAttrib && m_srcVideoAttrib->interlaced > 0
		//&& !_stricmp(m_srcVideoAttrib->codec, "AVC")) {
		//	pVideoPref->SetString("videosrc.ffmpeg.tinterlace", "interleave_top");
		//}
	}*/
}

void CTransWorkerSeperate::autoSelectBestDecoder(const char* srcFile, CXMLPref* pVideoPref, CXMLPref* pAudioPref)
{
#ifdef NOT_USE_AVS
	return;
#endif

	// Change only audio decoder
	if(m_videoEncs.empty()) {
		m_audioDecType = AD_MPLAYER;
		return;
	}

	// Select decoder according to file extension
	const char* ext = strrchr(srcFile, '.');
	if(!ext) return;

	bool bUseAvs = false;
	// If video is vfr, use avisynth to decode
	if(m_srcVideoAttrib && m_srcVideoAttrib->is_vfr && _stricmp(ext,".mkv") &&
		_stricmp(ext,".avi") && _stricmp(ext,".mpg") && _stricmp(ext,".mpeg")) {	// This situation has top priority
		m_videoDecType = VD_AVS;
		m_audioDecType = AD_AVS;
		return;
	}

	attr_audio_t* pAudioAttrib = NULL;
	if(!m_srcAudioAttribs.empty()) {
		pAudioAttrib = m_srcAudioAttribs[0];
	}
	if(!_stricmp(ext,".flv") || !_stricmp(ext,".f4v") || !_stricmp(ext,".hlv")) {
		if(m_srcVideoAttrib && m_srcVideoAttrib->originFps > 0.1f) {
			bUseAvs = true;		// originFps(read from video stream) is different from source fps (read from header)
		} else {
			pVideoPref->SetBoolean("videosrc.mencoder.useBackup", true);	// Use mencoder_ww decode flv
		}
	} else if(/*!_stricmp(ext,".ts") || */!_stricmp(ext,".m2t")){
		if(pAudioAttrib && *(pAudioAttrib->codec) && 
			( (!_stricmp(pAudioAttrib->codec, "MPEG Audio") && m_srcVideoAttrib && !_stricmp(m_srcVideoAttrib->codec, "MPEG Video")) 
			  || !_stricmp(pAudioAttrib->codec, "AC-3")) ) {
            //bUseAvs = false;
		} else {
			bUseAvs = true;
		}
	} else if (!_stricmp(ext,".mkv") ) {
		pVideoPref->SetBoolean("videosrc.mencoder.useBackup", true);
		if(pAudioAttrib && !_stricmp(pAudioAttrib->codec, "Cooker") ||
		  (m_srcVideoAttrib && (!_stricmp(m_srcVideoAttrib->codec, "RV40") || !_stricmp(m_srcVideoAttrib->codec, "RV30")))) {
			//pVideoPref->SetBoolean("videosrc.mencoder.lavfdemux", true);
		}
	} else if (!_stricmp(ext,".mov")) {
		bool useBackupVersion = true;
		if(m_srcVideoAttrib && m_srcVideoAttrib->has_origin_res) {
			useBackupVersion = false;
		}
		pVideoPref->SetBoolean("videosrc.mencoder.useBackup", useBackupVersion);
		pVideoPref->SetBoolean("videosrc.mencoder.lavfdemux", true);
	} else if(!_stricmp(ext,".ts")) {
		if(m_srcVideoAttrib && !_stricmp(m_srcVideoAttrib->standard, "Component") && m_srcVideoAttrib->interlaced > 0) {
			bUseAvs = true;
		}
		if(m_srcVideoAttrib && !_stricmp(m_srcVideoAttrib->codec, "AVC")) {
			pVideoPref->SetBoolean("videosrc.mencoder.lavfdemux", true);
		}
		pVideoPref->SetBoolean("videosrc.mencoder.useBackup", true);
	} else if(!_stricmp(ext,".mp4")) {
		if(m_srcVideoAttrib && m_srcVideoAttrib->interlaced > 0) {
			pVideoPref->SetBoolean("videosrc.mencoder.useBackup", true);
		}
	} else if(!_stricmp(ext,".rmvb")) {
		pVideoPref->SetBoolean("videosrc.mencoder.useBackup", true);
	} else if(!_stricmp(ext,".m2ts")) {
		pVideoPref->SetBoolean("videosrc.mencoder.useBackup", true);
		pVideoPref->SetBoolean("videosrc.mencoder.lavfdemux", true);
	}
	
	if(bUseAvs) {
		m_videoDecType = VD_AVS;
		m_audioDecType = AD_AVS;
	} else {
		m_videoDecType = VD_MENCODER;
		m_audioDecType = AD_MENCODER;
		// Force video decoder
		if(m_srcVideoAttrib && !_stricmp(m_srcVideoAttrib->codec, "VC-1") && !m_srcVideoAttrib->interlaced) {
			pVideoPref->SetString("videosrc.mencoder.forcevc", "ffvc1,");
		}
	}
}

bool CTransWorkerSeperate::parsePlaylistConfig(CXMLPref* prefs)
{
	std::string dstFile = m_streamFiles.GetDestFiles().at(0);
	std::string dstFolder, dstTitle, dstExt;
	StrPro::StrHelper::splitFileName(dstFile.c_str(), dstFolder, dstTitle, dstExt);

	const char* listType = prefs->GetString("extension.playlist.type");
	if(listType && *listType) {
		const char* listName = prefs->GetString("extension.playlist.name");
		bool bLive = prefs->GetBoolean("extension.playlist.live");
		const char* postfix = prefs->GetString("extension.playlist.postfix");
		
		if(listName && *listName) dstTitle = listName;
		if(postfix && *postfix) dstTitle += postfix;
		if(!m_pPlaylist) m_pPlaylist = new CPlaylistGenerator;
		m_pPlaylist->SetPlaylistName(dstTitle.c_str());
		m_pPlaylist->SetbLive(bLive);
		m_pPlaylist->SetPlaylistType(listType);
		m_pPlaylist->SetFilePath(dstFolder.c_str());
	}
	return true;
}

bool CTransWorkerSeperate::parseClipConfig(CXMLPref* prefs)
{
	int clipsCount = prefs->GetInt("overall.clips.count");
	for(int i=0; i<clipsCount; ++i) {
		char clipKey[32] = {0};
		sprintf(clipKey, "overall.clips.start%d", i+1);
		int startMs = prefs->GetInt(clipKey);	
		//startMs -= (startMs%200);	// 200ms tolerance
		memset(clipKey, 0, 32);
		sprintf(clipKey, "overall.clips.end%d", i+1);
		int endMs = prefs->GetInt(clipKey);		
		//endMs -= (endMs%200);		// 200ms tolerance
		m_clipStartSet.push_back(startMs);
		m_clipEndSet.push_back(endMs);
	}

	if(clipsCount <= 0) {	// If no clip in config, then read from extra file
		const char* srcFile = m_streamFiles.GetFirstSrcFile();
		if(srcFile && *srcFile) {
			std::string clipFile = srcFile;
			clipFile += "_clip.txt";
			ReadExtraClipConfig(clipFile.c_str(), m_clipStartSet, m_clipEndSet);
		}
	}
	return true;
}
bool CTransWorkerSeperate::parseSegmentConfig(CXMLPref* prefs)
{
	const char* segmentType = prefs->GetString("extension.split.type");
	int threshold = prefs->GetInt("extension.split.threshold");
	bool needSegment = true;
	if(threshold > 0 && m_tmpBenchData.mainDur < threshold*1000) {
		needSegment = false;
	}
	if(segmentType && (*segmentType) && strcmp(segmentType, "none") != 0 && needSegment) {
		m_pSplitter = new CMediaSplitter();
		if(!strcmp(segmentType, "average")) {
			m_pSplitter->SetSegmentType(SEG_TYPE_AVERAGE);
		} else if(!strcmp(segmentType, "normal")) {
			m_pSplitter->SetSegmentType(SEG_TYPE_NORMAL);
		}
		int segSize = prefs->GetInt("extension.split.segSize");
		if(segSize < 0) segSize = 0;
		const char* unitType = prefs->GetString("extension.split.unitType");
		if(unitType && !strcmp(unitType, "time")) {
			segSize *= 1000;
			m_pSplitter->SetUnitType(SEG_UNIT_BYTIME);
		} else if(unitType && !strcmp(unitType, "size")) {
			segSize *= 1024;
			m_pSplitter->SetUnitType(SEG_UNIT_BYSIZE);
		}
		m_pSplitter->SetSegInterval(segSize);
		const char* prefix = prefs->GetString("extension.split.prefix");
		if(prefix && *prefix) {
			m_pSplitter->SetPrefix(prefix);
		}
		m_pSplitter->SetSrcUrl(m_streamFiles.GetFirstSrcFile());
		m_pSplitter->SetDuration(m_tmpBenchData.mainDur);
		const char* subfix = prefs->GetString("extension.split.subfix");
		m_pSplitter->SetPostfix(subfix);
			
		int lastSegPercent = prefs->GetInt("extension.split.lastSegPercent");
		if(lastSegPercent >= 0) {
			m_pSplitter->SetLastSegDeltaPercent(lastSegPercent);
		}
		m_pSplitter->Init();
	}
	return true;
}

bool CTransWorkerSeperate::parseImageTailConfig(CXMLPref* prefs)
{
	const char* imagePath = prefs->GetString("videofilter.imagetail.imagepath");
	const char* imageFolder = prefs->GetString("videofilter.imagetail.imagefolder");
	if((imagePath && *imagePath) || (imageFolder && *imageFolder)) {
		m_pImgTail = new CImageSrc();
		if(imagePath && *imagePath) {
			m_pImgTail->SetStaticImage(imagePath);
			int imageDur = prefs->GetInt("videofilter.imagetail.duration");
			if(imageDur > 0) {
				m_pImgTail->SetStaticImageDuration(imageDur);
			}
		} else {
			m_pImgTail->SetImageFolder(imageFolder);
		}
		int cropMode = prefs->GetInt("videofilter.imagetail.cropmode");
		if(cropMode >= 0) {
			m_pImgTail->SetCropMode(cropMode);
		}
	}
	return true;
}

bool CTransWorkerSeperate::parseWatchfoderConfig(CXMLPref* prefs)
{
	// Get relative dir in watch folder
	const char* watchFolder = prefs->GetString("extension.watchfolder.path");
	if(watchFolder && *watchFolder) {
		StrPro::CCharset charConvert;
		std::string watchFolderPath;
		char* ansiFolder = charConvert.UTF8toANSI(watchFolder);
		if(ansiFolder) {
			watchFolderPath = ansiFolder;
			free(ansiFolder);
		}
		
		const char* srcFile = m_streamFiles.GetFirstSrcFile();
		std::string relativeDir;
		StrPro::StrHelper::getFileFolder(srcFile, relativeDir);
		if(!relativeDir.empty()) {		
			size_t relDirLen = relativeDir.size();
			if (relDirLen > watchFolderPath.size()) {
				relativeDir.erase(0, watchFolderPath.size()+1);
				if(!relativeDir.empty()) {
					if(relativeDir.back() == '\\' || relativeDir.back() == '/') {
						relativeDir.pop_back();
					}
					m_streamFiles.SetRelativeDir(relativeDir.c_str());
				}
			} 
		}
	}
	
	return true;
}

bool CTransWorkerSeperate::Stop()
{
	for (size_t i=0; i< m_auxDecoders.size(); ++i) {
		if(m_auxDecoders[i]) {
			m_auxDecoders[i]->Cleanup();
		}
	}
	if(m_pAudioDec) {
		m_pAudioDec->Cleanup();
	}
	if(m_pVideoDec && m_pVideoDec != m_pAudioDec) {
		m_pVideoDec->Cleanup();
	}
	return CTransWorker::Stop();
}

bool CTransWorkerSeperate::decodeNext()
{
	// If previous audio or video decoding not end.
	if(m_pAudioDec->GetAudioReadHandle() > 0 || m_pVideoDec->GetVideoReadHandle() > 0) return true;
	for (size_t i=0; i<m_auxDecoders.size(); ++i) {
		if(m_auxDecoders[i]->GetAudioReadHandle() > 0) return true;
	}

	m_lockReadAudio = 1;		// Lock reading audio
	m_lockReadVideo = 1;		// Lock reading video
	//Sleep(100);

	deleteDecoders();
	const char* srcFile = m_streamFiles.GetCurSrcFile();
	if(srcFile && *srcFile != '\0') {
		std::map<std::string, StrPro::CXML2*>& subMediaInfos = m_pRootPref->GetSubMediaInfoNodes();
		StrPro::CXML2* pxml = subMediaInfos[std::string(srcFile)];
		if(pxml) initAVSrcAttrib(pxml);
		
		startDecoder(srcFile);
		startAuxDecoders(srcFile);
	}
	
	m_bDecodeNext = !m_streamFiles.IsAllSrcFilesConsumed();
	return true;
}

bool CTransWorkerSeperate::processStreamCopy()
{
	const char* srcFile = m_streamFiles.GetFirstSrcFile();
	if(!srcFile || !(*srcFile) || m_muxers.empty()) return false;
	// Copy decoder's preset
	CXMLPref* pMuxerPref = NULL;
	pMuxerPref = m_muxers[0]->GetPref();
	
	const char* srcExt = strrchr(srcFile, '.');
	if(srcExt && !_stricmp(srcExt, ".mp4") && m_bCopyAudio && m_bCopyVideo) {	// if it's mp4 file
		if(m_muxers[0]->GetMuxerType() == MUX_MP4) {
			// Use mp4box simply remux
			std::string remuxStr = MP4BOX" -add \"";
			remuxStr += srcFile;
			remuxStr += "\" -inter 400 -new \"";
			remuxStr += m_streamFiles.GetDestFiles().at(0);
			remuxStr += "\"";
			#ifdef DEBUG_EXTERNAL_CMD
			logger_info(m_logType, "Remux cmd: %s.\n", remuxStr.c_str());
			#endif
			int retCode = CProcessWrapper::Run(remuxStr.c_str());

			// Cleanup muxers
			while (m_muxers.size()) {
				CMuxer *pmuxer = m_muxers.back();
				m_muxers.pop_back();
				delete pmuxer;
			}
			return (retCode == 0);
		}
	}

	bool ret = true;
	if(m_bCopyAudio) {
		m_pAudioDec = new CDecoderCopy;
		m_pAudioDec->SetEnableAudio(true);
		m_pAudioDec->BindAudioPref(pMuxerPref);
		ret = copyStream(srcFile, true);
	}
	if(!ret) return ret;

	if(m_bCopyVideo) {
		m_pVideoDec = new CDecoderCopy;
		m_pVideoDec->SetEnableVideo(true);
		m_pVideoDec->BindVideoPref(pMuxerPref);
		ret = copyStream(srcFile, false);
	}
	if(!ret) return ret;

	return ret;
}

bool CTransWorkerSeperate::copyStream(const char* srcFile, bool isAudio)
{
	CDecoder* pDecoder = NULL;
	if(isAudio) pDecoder = m_pAudioDec;
	else pDecoder = m_pVideoDec;
	if(!pDecoder) return false;

	std::string tmpFile;
	attr_audio_t* pAudioAttrib = NULL;
	if(isAudio && !m_srcAudioAttribs.empty()) {
		pAudioAttrib = m_srcAudioAttribs[0];
		audio_format_t audioFormat = AC_MP3;
		if(!_stricmp(pAudioAttrib->codec, "wmav2")) audioFormat = AC_WMA8;
		else if(!_stricmp(pAudioAttrib->codec, "aac")) audioFormat = AC_AAC_LC;
		else if(!_stricmp(pAudioAttrib->codec, "eac3")) {
			audioFormat = AC_EAC3;
			//if(!m_muxers.empty() && m_muxers[0]->GetMuxerType() == MUX_MP4) {
			//	m_muxers[0]->SetUseDMGMuxMp4(true);
			//}
		}
		else if(!_stricmp(pAudioAttrib->codec, "mp3")) audioFormat = AC_MP3;
		else if(!_stricmp(pAudioAttrib->codec, "mp2")) audioFormat = AC_MP2;
		else if(!_stricmp(pAudioAttrib->codec, "ac3")) audioFormat = AC_AC3;
		else if(!_stricmp(pAudioAttrib->codec, "dca")) audioFormat = AC_DTS;
		tmpFile = m_streamFiles.GetStreamFileName(ST_AUDIO, audioFormat);
		m_pCopyAudioInfo = new audio_info_t;
		memset(m_pCopyAudioInfo, 0, sizeof(audio_info_t));
		m_pCopyAudioInfo->format = audioFormat;
		m_pCopyAudioInfo->out_channels = pAudioAttrib->channels;
		m_pCopyAudioInfo->out_srate = pAudioAttrib->samplerate;
		CFileQueue::queue_item_t fileItem = {tmpFile, ST_AUDIO, NULL, m_pCopyAudioInfo};
		m_streamFiles.Enqueue(fileItem);
	}

	if(!isAudio && m_srcVideoAttrib) {
		video_format_t videoFormat = VC_MPEG2;
		if(!_stricmp(m_srcVideoAttrib->codec, "wmv2")) videoFormat = VC_WMV8;
		else if(!_stricmp(m_srcVideoAttrib->codec, "h264")) videoFormat = VC_H264;
		else if(!_stricmp(m_srcVideoAttrib->codec, "mpeg4")) videoFormat = VC_XVID;
		else if(!_stricmp(m_srcVideoAttrib->codec, "mpeg2video")) videoFormat = VC_MPEG2;
			
		tmpFile = m_streamFiles.GetStreamFileName(ST_VIDEO, videoFormat);
		m_pCopyVideoInfo = new video_info_t;
		memset(m_pCopyVideoInfo, 0, sizeof(video_info_t));
		m_pCopyVideoInfo->format = videoFormat;
		m_pCopyVideoInfo->fps_out.num = m_srcVideoAttrib->fps_num;
		m_pCopyVideoInfo->fps_out.den = m_srcVideoAttrib->fps_den;
		CFileQueue::queue_item_t fileItem = {tmpFile, ST_VIDEO, m_pCopyVideoInfo, NULL};
		m_streamFiles.Enqueue(fileItem);

		// Check if file format is TS, no need to add annexb stream filter
		if(_stricmp(m_srcVideoAttrib->fileFormat, "mpegts") == 0) {
			pDecoder->SetForceAnnexbCopy(false);
		}
	}

	pDecoder->SetDumpFileName(tmpFile);
	if(srcFile) {
		bool success = pDecoder->Start(srcFile);
		return success;
	} 
	return false;
}

// Prevent avsinput crash when force it to end(Transcode part of media file)
void CTransWorkerSeperate::closeDecoders()
{
	if(m_pAudioDec == m_pVideoDec) {
		if(m_pAudioDec) m_pAudioDec->Cleanup();
	} else {
		if(m_pAudioDec) m_pAudioDec->Cleanup();
		if(m_pVideoDec) m_pVideoDec->Cleanup();
	}
	for (size_t i=0; i< m_auxDecoders.size(); ++i) {
		if(m_auxDecoders[i]) {
			m_auxDecoders[i]->Cleanup();
		}
	}
}

void CTransWorkerSeperate::deleteDecoders()
{
	if(m_pAudioDec == m_pVideoDec) {
		if(m_pAudioDec) delete m_pVideoDec;
		m_pVideoDec = m_pAudioDec = NULL;
	} else {
		if(m_pVideoDec) delete m_pVideoDec;
		if(m_pAudioDec) delete m_pAudioDec;
		m_pVideoDec = m_pAudioDec = NULL;
	}

	for (size_t i=0; i< m_auxDecoders.size(); ++i) {
		if(m_auxDecoders[i]) {
			m_auxDecoders[i]->Cleanup();
			delete m_auxDecoders[i];
			m_auxDecoders[i] = NULL;
		}
	}
	m_auxDecoders.clear();
}
