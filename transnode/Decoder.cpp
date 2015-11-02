#include "Decoder.h"
#include "WorkManager.h"
#include "util.h"
#include "MEvaluater.h"
#include "yuvUtil.h"
#include "logger.h"

CDecoder::CDecoder(void) : m_bDecAudio(false), m_bDecVideo(false), m_bEnableVf(false), m_bEnableAf(false), 
 m_bCopyAudio(false), m_bCopyVideo(false), m_bLastPass(false),
 m_pAInfo(NULL), m_pVInfo(NULL),  m_pVideoPref(NULL),m_pAudioPref(NULL),
 m_pFileQueue(NULL), m_workerId(0), m_fdReadAudio(-1), m_fdReadVideo(-1), m_exitCode(-1)
{
	memset(&m_yuvinfo, 0, sizeof(m_yuvinfo));
	memset(&m_wavinfo, 0, sizeof(m_wavinfo));

	// Set decoder exe to low priority
	m_proc.flags = SF_LOWER_PRIORITY;
	m_bDecodeEnd = false;
	m_bForceResample = false;
	//m_duration = 0;
}

CDecoder::~CDecoder(void)
{
	Cleanup();
}

void CDecoder::Cleanup() 
{
	// Get exit code
    if(m_proc.IsProcessRunning(&m_exitCode)) {
		m_proc.Cleanup();
		//logger_info(LOGM_TS_VD, "Decoder exit code:%d.\n", m_exitCode);
	}
	CloseAudioReadHandle();
	CloseVideoReadHandle();
}

void CDecoder::SetErrorCode(int code)
{
	CWorkManager* pWorkManager = CWorkManager::GetInstance();
	pWorkManager->SetErrorCode((error_code_t)code, m_workerId);
}

bool CDecoder::ReadAudioHeader()
{
	int readSize = SafeRead(m_fdReadAudio, (uint8_t*)&m_wavinfo, sizeof(m_wavinfo));
	int readTryCount = 0;
	while(readSize <= 0 && readTryCount < 8) {
		Sleep(200);
		readSize = SafeRead(m_fdReadAudio, (uint8_t*)&m_wavinfo, sizeof(m_wavinfo));
		readTryCount++;
	}
	if(readSize <= 0) {
		logger_err(LOGM_TS_VD, "Read audio header failed, return value:%d.\n", readSize);
		return false;
	}
	return true;
}

bool CDecoder::ReadVideoHeader()
{
	int readSize = SafeRead(m_fdReadVideo, (uint8_t*)&m_yuvinfo, sizeof(m_yuvinfo));
	int readTryCount = 0;
	while(readSize <= 0 && readTryCount < 8) {
		Sleep(200);
		readSize = SafeRead(m_fdReadVideo, (uint8_t*)&m_yuvinfo, sizeof(m_yuvinfo)); 
		readTryCount++;
	}
	if(readSize <= 0) {
		logger_err(LOGM_TS_VD, "Read video header failed, return value:%d.\n", readSize);
		return false;
	}
	return true;
}

int CDecoder::ReadAudio(uint8_t* pBuf, int bufSize)
{
	return SafeRead(m_fdReadAudio, pBuf, bufSize);
}

int CDecoder::ReadVideo(uint8_t* pBuf, int bufSize)
{
	return SafeRead(m_fdReadVideo, pBuf, bufSize);
}

void CDecoder::CloseAudioReadHandle()
{
	if(m_fdReadAudio > 0) {
		_close(m_fdReadAudio);
		m_fdReadAudio = -1;
	} 
}

void CDecoder::CloseVideoReadHandle()
{
	if(m_fdReadVideo > 0) {
		_close(m_fdReadVideo);
		m_fdReadVideo = -1;
	} 
}

std::string CDecoder::GetDVDPlayCmd(const char* dvdStr)
{
	std::vector<std::string> dvdParams;
	StrPro::StrHelper::splitString(dvdParams, dvdStr);
	std::string dvdCmd("dvd://");
	if(!dvdParams[1].empty() && dvdParams[1] != "0") {		// Track id or title
		dvdCmd += dvdParams[1];								
	}

	std::string devicePath = dvdParams[5];					// Device path (ex. G:)
	if(devicePath.length() == 1) {
		devicePath += ":";
	}
	dvdCmd += " -dvd-device ";
	dvdCmd += devicePath;

	if(!dvdParams[2].empty() && dvdParams[2] != "-1") {		// Chapter id
		dvdCmd += " -chapter ";
		dvdCmd += dvdParams[2]+"-";
		dvdCmd += dvdParams[2];
	}

	if(!dvdParams[3].empty() && dvdParams[3] != "-1") {		// Audio id
		dvdCmd += " -aid ";
		dvdCmd += dvdParams[3];
	}
	return dvdCmd;
}

std::string CDecoder::GetVCDPlayCmd(const char* vcdStr)
{
	std::vector<std::string> vcdParams;
	StrPro::StrHelper::splitString(vcdParams, vcdStr);
	std::string vcdCmd("vcd://");
	if(!vcdParams[1].empty() && vcdParams[1] != "0") {		// Track id or title
		vcdCmd += vcdParams[1];								
	}
	std::string devicePath = vcdParams[2];					// Device path (ex. G:)
	if(devicePath.length() == 1) {
		devicePath += ":";
	}
	vcdCmd += " -cdrom-device ";
	vcdCmd += devicePath;
	return vcdCmd;
}

std::string CDecoder::GetSubFile(const char* srcFile)
{
	if(!srcFile || !m_pFileQueue) return "";

	const char* overlaySubFile = m_pVideoPref->GetString("videofilter.overlay.text");
	if (overlaySubFile && FileExist(overlaySubFile)) {
		return overlaySubFile;
	}
	const char* pSubFile = m_pFileQueue->GetSubTitleFile();
	if(pSubFile && *pSubFile != '\0') {
		return pSubFile;
	} else {
		int fuzziNess = m_pVideoPref->GetInt("overall.subtitle.fuzziness");
		if(fuzziNess >= 0) {
			std::string srcPath, srcName, ext;
			StrPro::StrHelper::splitFileName(srcFile, srcPath, srcName, ext);
			const char* subExt[] = {".srt", ".ass", ".ssa", ".idx"};
			std::string subFile;
			int subFormatCount = sizeof(subExt)/sizeof(subExt[0]);
			int i = 0;
			for (i=0; i<subFormatCount; ++i) {
				subFile = srcPath + srcName + subExt[i];
				if(FileExist(subFile.c_str())) break;
			}

			// Subtitle file not found
			if(i == subFormatCount) subFile.clear();

			return subFile;
		}
	}

	return "";
}

void CDecoder::AutoCropOrAddBand(const char* fileName)
{
	int cropMode = m_pVideoPref->GetInt("videofilter.crop.mode");
	CYuvUtil::FrameRect rect = {0, 0, 0, 0};
	
	if(m_pVInfo->dest_dar.num<=0 || m_pVInfo->dest_dar.den<=0) return;
	if(m_pVInfo->src_dar.num<=0 || m_pVInfo->src_dar.den<=0) return;
	float destDar = (float)m_pVInfo->dest_dar.num/m_pVInfo->dest_dar.den;
	float srcDar = (float)m_pVInfo->src_dar.num/m_pVInfo->src_dar.den;

	switch(cropMode) {
	case 2:			// Crop to fit
		rect = CYuvUtil::CalcAutoCrop(srcDar, destDar, m_pVInfo->res_in.width, m_pVInfo->res_in.height);
		m_pVideoPref->SetInt("videofilter.crop.left", rect.x);
		m_pVideoPref->SetInt("videofilter.crop.top", rect.y);
		m_pVideoPref->SetInt("videofilter.crop.width", rect.w);
		m_pVideoPref->SetInt("videofilter.crop.height", rect.h);
		break;
	case 3:			// Expand to fit (expand after scale)
		//rect = CYuvUtil::CalcAutoExpand(srcDar, destDar, m_pVInfo->res_in.width, m_pVInfo->res_in.height);
		rect.w = m_pVInfo->res_out.width;
		rect.h = m_pVInfo->res_out.height;
		rect.x = -1; rect.y = -1;
		m_pVideoPref->SetBoolean("videofilter.expand.enabled", true);
		m_pVideoPref->SetInt("videofilter.expand.x", rect.x);
		m_pVideoPref->SetInt("videofilter.expand.y", rect.y);
		m_pVideoPref->SetInt("videofilter.expand.width", rect.w);
		m_pVideoPref->SetInt("videofilter.expand.height", rect.h);
		break;
	}
}

bool CDecoder::Start(const char* sourceFile)
{
        // specify fontconfig ENV for ”ffmpeg -vf ass“ or ”ffmpeg -vf subtitles“,
        // so that they use "codecs/fonts/fonts.conf" rather than "/etc/fonts/fonts.conf"
        const char *fc_env[] = {
		"FONTCONFIG_PATH=./codecs/fonts", 
		"FONTCONFIG_FILE=fonts.conf", 
		0
	}; 

	if(!sourceFile || *sourceFile == '\0') return false;
	int fdAWrite = -1, fdVWrite = -1;
	m_bDecodeEnd = false;

	if(m_bDecAudio) {		// Read PCM data and create audio encoders
//#ifdef WIN32
		CProcessWrapper::MakePipe(m_fdReadAudio, fdAWrite, AUDIO_PIPE_BUFFER, false, true);
//#else
        //m_proc.flags |= SF_USE_AUDIO_PIPE;
        //m_proc.flags |= SF_INHERIT_WRITE;
//#endif
	}
		
	if(m_bDecVideo) {		// Read YUV data and create video encoders
//#ifdef WIN32
		CProcessWrapper::MakePipe(m_fdReadVideo, fdVWrite, VIDEO_PIPE_BUFFER, false, true);
//#else
//		m_proc.flags |= SF_USE_VIDEO_PIPE;
//		m_proc.flags |= SF_INHERIT_WRITE;
//#endif
	}
 
	if(m_cmdString.empty()) {
		m_cmdString  = GetCmdString(sourceFile);
	}

	// Replace pipe fd holder with real pipe handle
	std::string finalCmdStr = m_cmdString;
	char fdStr[6] = {0};
	sprintf(fdStr, "%d", fdAWrite);
	ReplaceSubString(finalCmdStr, "$(fdAudioWrite)", fdStr);
	memset(fdStr, 0, 6);
	sprintf(fdStr, "%d", fdVWrite);
	ReplaceSubString(finalCmdStr, "$(fdVideoWrite)", fdStr);
	
#ifdef DEBUG_EXTERNAL_CMD
	logger_info(LOGM_TS_VD, "Cmd: %s.\n", finalCmdStr.c_str());
#endif
	bool success = m_proc.Spawn(finalCmdStr.c_str(), 0, fc_env);
	if(success) {
		success = (m_proc.Wait(DECODER_START_TIME) == 0);
		if(!success) {	// If process exit but exit code is 0, the process succeed.
			int exitCode = -1;
			m_proc.IsProcessRunning(&exitCode);
			if(exitCode == 0) success = true;
		}
	}
	if (!success) {
		// Specify audio id somtimes will fail(ex.Ts file), then remove it and try again.
		m_proc.Cleanup();
		size_t aidIdx = finalCmdStr.find("-aid");
		if(aidIdx != finalCmdStr.npos) {	// Remove -aid option
			size_t firstSpace = finalCmdStr.find_first_of(' ', aidIdx);
			if(firstSpace != finalCmdStr.npos) {
				size_t secondSpace = finalCmdStr.find_first_of(' ', firstSpace+1);
				if(secondSpace != finalCmdStr.npos) {
					finalCmdStr.erase(aidIdx, secondSpace-aidIdx+1);
				} else {
					finalCmdStr.erase(aidIdx);
				}
			}
			success = m_proc.Spawn(finalCmdStr.c_str(), 0, fc_env) && m_proc.Wait(DECODER_START_TIME) == 0;
		}
	}

	if(fdAWrite != -1) {
		_close(fdAWrite);
	}
	if(fdVWrite != -1) {
		_close(fdVWrite);
	}

	return success;
}
