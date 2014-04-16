#include <sstream>
#include <time.h>
#ifdef WIN32
#include <direct.h>
#endif
#include "MEvaluater.h"
#include "FileQueue.h"
#include "muxers.h"
#include "processwrapper.h"
#include "StrHelper.h"
#include "logger.h"
#include "bitconfig.h"

#include "./flvmuxer/FileMixer.h"

#ifdef HAVE_WMENCODER
#include "WMMuxer.h"
#endif

#ifdef HAVE_MII_ENCODER
#include "MgpMachine.h"
#pragma comment(lib, "MgpWrapperLib.lib")
#endif

#include "encoderCommon.h"
#include "util.h"

#define THREAD_FAIL_CODE 1000
#define MOVE_ERROR_FORMAT "Failed to move temp file %s to target file %s, try to copy.\n"
#define WAIT_INTERVAL_MS 20

using namespace std;

std::string CMuxer::GenDestFileName(const char* srcFileName, size_t destIndex, const char* srcDir)
{
	const char* tempExt = GetOutExt(false);
	const char* relativeDir = m_pFileQueue->GetRelativeDir();
	
	return genDestFileName(srcFileName, m_pref, tempExt, relativeDir, destIndex, srcDir);
}

bool CMuxer::SetPref(CXMLPref* pref)
{
	if(!pref || !pref->goRoot()) {
		logger_err(LOGM_TS_MUX, "Muxer pref is invalid.\n");
		return false;
	}
	m_pref = pref;

	const char* audioIndexStr = pref->getAttribute("aid");
	if(audioIndexStr && *audioIndexStr) {
		if(!strchr(audioIndexStr, ',')) {
			m_audioIndexs.push_back(atoi(audioIndexStr));
		} else {
			StrPro::StrHelper::parseStringToNumArray(m_audioIndexs, audioIndexStr);
		}
	}

	const char* videoIndexStr = pref->getAttribute("vid");
	if(videoIndexStr && *videoIndexStr) {
		if(!strchr(videoIndexStr, ',')) {
			m_videoIndexs.push_back(atoi(videoIndexStr));
		} else {
			StrPro::StrHelper::parseStringToNumArray(m_videoIndexs, videoIndexStr);
		}
	}

	// Index passed in are based on 1 
	for (size_t i=0; i<m_audioIndexs.size(); ++i) {
		if(m_audioIndexs[i] == 0) continue;
		m_audioIndexs[i] -= 1;
	}
	for (size_t i=0; i<m_videoIndexs.size(); ++i) {
		if(m_videoIndexs[i] == 0) continue;
		m_videoIndexs[i] -= 1;
	}

	return true;
}

void CMuxer::AddAudioStream(int audioIdx)
{
	if(audioIdx > 0) m_audioIndexs.push_back(audioIdx);
}

bool CMuxer::ContainStream(int streamType, size_t index)
{
	switch(streamType) {
		case ST_AUDIO: {
			for (size_t i=0; i<m_audioIndexs.size(); ++i) {
				if(m_audioIndexs[i] == index) {
					return true;
				}
			}
			break;
		}
		case ST_VIDEO: {
			for (size_t i=0; i<m_videoIndexs.size(); ++i) {
				if(m_videoIndexs[i] == index) {
					return true;
				}
			}
			break;
		}
	}
	return false;
}

bool CMuxer::checkAvailableSpace(const char* filePath)
{
#ifdef WIN32
	int64_t availableSpace = GetFreeSpaceKb(filePath);
	if(availableSpace != -1) {
		if(availableSpace < 1024) {		// Smaller than 1M
			logger_err(LOGM_TS_MUX, "Available disk space is:%d KB, too small.\n", availableSpace);
			return false;
		}
	}
#endif
	return true;
}

//THREAD_RET_T WINAPI CMuxer::muxThread(void *muxer)
//{
//	CMuxer* pMuxer = static_cast<CMuxer*>(muxer);
//	if(pMuxer == NULL) return THREAD_FAIL_CODE;
//	if(pMuxer->muxFunction()) return 0;
//	return THREAD_FAIL_CODE;
//}

//mux_error_t CMuxer::Mux()
//{
	//int ret = CBIThread::Create(m_hThread, CMuxer::muxThread, this);
	//if(ret != 0) {
	//	logger_err(LOGM_TS_MUX, "Create muxing thread failed.\n");
	//	return false;
	//}
	//CBIThread::SetPriority(m_hThread, THREAD_PRIORITY_BELOW_NORMAL);
	//// Wait for complete
	//THREAD_RET_T* threadRetCode = new THREAD_RET_T();
	//CBIThread::Join(m_hThread, threadRetCode);
	//if(*threadRetCode == 0) {
	//	delete threadRetCode;
	//	return true;
	//}

	//delete threadRetCode;
	//return false;
//}

// Validate mp4 file(Check audio/video data is corrupt)
bool validateMp4File(const char* mp4file)
{
	CProcessWrapper proc;
	std::string cmdString = FFMPEG" -i \"";
	cmdString += mp4file;
	cmdString += "\" -t 400 -c:v copy -c:a copy -v error";
	cmdString += " -f null -y "FFMPEG_NUL;
		
	proc.flags = SF_REDIRECT_OUTPUT;
    proc.Spawn(cmdString.c_str());
	int procWaitRet = proc.Wait(500);
    if(procWaitRet == -1) {
		return true;
	}

    //bool parseEnd = false;
	const int bufSize = 1024;
	char buf[bufSize+1] = {0};
	int readTryCount = 0;
	do {
		int bytesRead = proc.Read(buf, bufSize);
		if (bytesRead < 0) {
			int waitCode = proc.Wait(100);
			if(waitCode == 0) {		// Timeout
				readTryCount++;
				if(readTryCount <= 5) continue;
			} else if(waitCode == 1) {	// Proc terminate
				readTryCount += 4;
				if(readTryCount <= 5) continue;
			}
		}
			
		if(strstr(buf, "no frame!") || strstr(buf, "Error number -1 occurred") || 
			strstr(buf, "channel element 0.0 is not allocated") ||
			strstr(buf, "error while decoding") || strstr(buf, "decode_slice_header error") ||
			strstr(buf, "Missing reference picture")) {
			logger_err(LOGM_TS_MUX, "Output mp4 audio/video data is corrupted.\n");
			proc.Cleanup();
			return false;
		}
		memset(buf, 0, bufSize);
	} while (proc.IsProcessRunning());
	proc.Cleanup();
	return true;
}

class CMp4box : public CMuxer
{
public:
	CMp4box() {m_muxInfoSizeRatio = 0.012f;}
	int Mux()
	{
		if(!m_pFileQueue) {
			logger_err(LOGM_TS_MUX, "FileQueue hasn't been set.\n");
			return MUX_ERR_INVALID_FILE_QUEUE;
		}

		// Use DMG muxer
		/*if(m_bUsingDMGMp4Mux) {
			const char* dmgMuxer = m_pref->GetString("muxer.mp4box.dmgPath");
			std::string dmgCmd("\"");
			dmgCmd += dmgMuxer;
			dmgCmd.push_back('"');
			CFileQueue::queue_item_t* item = NULL;
			for (item = m_pFileQueue->GetFirst(ST_VIDEO); item; item = m_pFileQueue->GetNext(ST_VIDEO)) {
				if(!item->fileName.empty()) {
					dmgCmd += " -i \"";
					dmgCmd += item->fileName;
					dmgCmd.push_back('"');
				}
			}
			for (item = m_pFileQueue->GetFirst(ST_AUDIO); item; item = m_pFileQueue->GetNext(ST_AUDIO)) {
				if(!item->fileName.empty()) {
					dmgCmd += " -i \"";
					dmgCmd += item->fileName;
					dmgCmd.push_back('"');
				}
			}

			const char* destFile = m_pFileQueue->GetCurDestFile();
            if (destFile == NULL || (*destFile) == NULL) {
               logger_err(LOGM_TS_MUX, "Invaild target File name.\n");
			   return MUX_ERR_MUXER_FAIL;
            } 
			dmgCmd += " -o \"";
			dmgCmd += destFile;
			dmgCmd.push_back('"');
            RemoveFile(destFile);
			#ifdef DEBUG_EXTERNAL_CMD
			logger_info(LOGM_TS_MUX, "Cmd:%s\n", dmgCmd.c_str());
			#endif
			// Run command
			if(CProcessWrapper::Run(dmgCmd.c_str()) != 0) {
				logger_err(LOGM_TS_MUX, "dmg-umux mux file failed.\n");
				return MUX_ERR_MUXER_FAIL;
			}

			// Add dolby DMG tag
			std::string mp4BoxTagCmd = MP4BOX" -itags tool=\"Dolby Media Generator 3.5.0\" \"";
			mp4BoxTagCmd += destFile;
			mp4BoxTagCmd.push_back('"');
			CProcessWrapper::Run(mp4BoxTagCmd.c_str());
			return MUX_ERR_SUCCESS;
		}*/

		std::ostringstream sstr;
		std::string tmpdir = m_pFileQueue->GetTempDir();
        if(*tmpdir.rbegin() == '\\' || *tmpdir.rbegin() == '/') {
            tmpdir.erase(tmpdir.end()-1);
		}
		
		sstr << MP4BOX << " -quiet -tmp \"" << tmpdir << "\"";
		int fragMs = m_pref->GetInt("muxer.mp4box.frag");
		if(fragMs > 0) {
			sstr << " -frag " << fragMs;
		} else {
			int interMs = m_pref->GetInt("muxer.mp4box.inter");
			if(interMs > 0) {
				sstr << " -inter " << interMs;
			}
		}

		if (m_pref->GetBoolean("muxer.mp4box.packed")) sstr << " -packed";
		if (m_pref->GetBoolean("muxer.mp4box.keepSystemInfo")) sstr << " -keepsys";
		if (m_pref->GetBoolean("muxer.mp4box.isma")) sstr << " -isma";
		if (m_pref->GetBoolean("muxer.mp4box.sampleDesc")) sstr << " -mpeg4";
		if (m_pref->GetBoolean("muxer.mp4box.nodrop")) sstr << " -nodrop";
		if(m_pref->GetBoolean("muxer.mp4box.hint")) {
			sstr << " -hint";
		}
		
		const char* majorBrand = m_pref->GetString("muxer.mp4box.brand");
		if(majorBrand && *majorBrand) {
			sstr << " -brand " << majorBrand;
			const char* brandVer = m_pref->GetString("muxer.mp4box.version");
			if(brandVer && *brandVer) {
				sstr << ":" << brandVer;
			}
		}

		bool is3GP = false;
		switch (m_pref->GetInt("overall.container.format")) {
		case CF_3GP:
			is3GP = true;
		case CF_3GP2:
			is3GP = true;
			break;
		}

		//const char* outputUrl = m_pref->GetString("overall.muxer.url");
		
		if (m_pref->GetBoolean("muxer.mp4box.rewrite3gp") || is3GP) {
			sstr << " -3gp";
			is3GP = true;
		}

		std::stringstream videoTrackStr, audioTrackStr;
		size_t streamIdx = 0;
		
		CFileQueue::queue_item_t* item = NULL;
		for (item = m_pFileQueue->GetFirst(ST_AUDIO); item; item = m_pFileQueue->GetNext(ST_AUDIO)) {
			if(!item->ainfo) continue;
			if (item->ainfo->format == AC_AAC_HE) {
				int aacSbr = m_pref->GetInt("muxer.mp4box.sbr");
				if(aacSbr == 1) {
					audioTrackStr << " -sbr";
				} else if(aacSbr == 2) {
					audioTrackStr << " -sbrx";
				}
			} else if (item->ainfo->format == AC_EAC3) {
				sstr << " -itags tool=\"Dolby Media Generator 3.5.0\"";
			}
			
			if(ContainStream(ST_AUDIO, streamIdx)) {
				audioTrackStr << " -add \"" << item->fileName << "\"#audio:name=Audio"
					<< streamIdx+1;
				if(*(item->ainfo->lang)) {
					audioTrackStr << "-" << item->ainfo->lang << ":lang=" << item->ainfo->lang;
				}
// 				if(item->ainfo && item->ainfo->delay != 0) {
// 					audioTrackStr << ":delay=" << item->ainfo->delay;
// 				}
			}
			streamIdx++;
			//if (tracks[i].delay) audioTrackStr << ":delay=%d", tracks[i].delay);
		}

		streamIdx = 0;
		for (CFileQueue::queue_item_t* item = m_pFileQueue->GetFirst(ST_VIDEO); item; item = m_pFileQueue->GetNext(ST_VIDEO)) {
			if(ContainStream(ST_VIDEO, streamIdx)) {
				if(item->vinfo && item->vinfo->fps_out.den > 0 && m_pref->GetBoolean("muxer.mp4box.fps")) {
					videoTrackStr << " -fps " << (float)item->vinfo->fps_out.num / item->vinfo->fps_out.den;
				}
				videoTrackStr << " -add \"" << item->fileName << "\"#video:name=VideoByEZMediaEditor";
				if(m_pref->GetBoolean("muxer.mp4box.par") && item->vinfo && item->vinfo->dest_par.den > 0) {
					videoTrackStr << ":par=" << item->vinfo->dest_par.num << ':' << item->vinfo->dest_par.den;
				}
				if(item->vinfo->format == VC_HEVC) {
					videoTrackStr << ":FMT=HEVC";
				}
			}
			streamIdx++;
		}

		if (m_pref->GetInt("overall.container.trackOrder") == 0) {
			//video first
			sstr << videoTrackStr.str() << audioTrackStr.str();
		}
		else {
			//audio first
			sstr << audioTrackStr.str() << videoTrackStr.str();
		}

		// subtitle
		for (CFileQueue::queue_item_t* item = m_pFileQueue->GetFirst(ST_SUBTITLE); item; item = m_pFileQueue->GetNext(ST_SUBTITLE)) {
			sstr << " -add \"" << item->fileName << "\"";
		}

		if (m_pref->GetBoolean("overall.tagging.tag")) {
			//Tagging(muxfile, is3GP);
			const char* strValue = m_pref->GetString("overall.tagging.copyright");
			if(strValue && *strValue) {
                sstr << " -cprt \"" << strValue << "\"";
			}
		}

		std::string muxfile = m_pFileQueue->GetStreamFileName(ST_MUXER, MUX_MP4, 0, GetOutExt(false));
		sstr << " -new \"" << muxfile << "\"";

		const char* extraOption = m_pref->GetString("muxer.mp4box.options");
		if(extraOption && *extraOption) {
			sstr << " " << extraOption;
		}

#ifdef DEBUG_EXTERNAL_CMD
		logger_info(LOGM_TS_MUX, "%s\n", sstr.str().c_str());
#endif
        #define FAIL_INFO(err) logger_err(LOGM_TS_MUX, err); muxRet=MUX_ERR_MUXER_FAIL; break;
		int muxRet = 0;
        do {
            muxRet = CProcessWrapper::Run(sstr.str().c_str());
			// If failed to mux, then try once
			if (muxRet != 0) {
				logger_err(LOGM_TS_MUX, "First mux with MP4Box failed, return code:%d.\nRetry once.", muxRet);
				RemoveFile(muxfile.c_str());
				muxRet = CProcessWrapper::Run(sstr.str().c_str());
			}

            if (muxRet != 0) {
                logger_err(LOGM_TS_MUX, "Failed to mux with MP4Box, return code:%d.\n", muxRet);
				muxRet = MUX_ERR_MUXER_FAIL;
				break;
            }

            const char* destFile = m_pFileQueue->GetCurDestFile();
            if (destFile == NULL || (*destFile) == NULL) {
               FAIL_INFO("Invaild target File name.\n");
            } 

            RemoveFile(destFile);

			// Remux
			Sleep(WAIT_INTERVAL_MS);
			if(m_pref->GetInt("overall.container.format") == CF_FLV) {	// Flv mux
				muxRet = muxFlv(muxfile.c_str(), destFile);
				RemoveFile(muxfile.c_str());
			} else if(m_pref->GetInt("overall.container.format") == CF_HLS) {	// HLS mux
				muxRet = muxHls(muxfile.c_str(), destFile);
				RemoveFile(muxfile.c_str());
			} else {
				if (!TsMoveFile(muxfile.c_str(), destFile)) {
					logger_err(LOGM_TS_MUX, MOVE_ERROR_FORMAT, muxfile.c_str(), destFile);
                    if (!TsCopyFile(muxfile.c_str(), destFile)) {
						void* lpMsgBuf = GetLastErrorMsg();
						logger_err(LOGM_TS_MUX, "Failed to copy, error:%s.\n", (char*)lpMsgBuf);
						FreeErrorMsgBuf(lpMsgBuf);
						muxRet = MUX_ERR_MUXER_FAIL;
						break;
					}
				}

				// Validate mp4 file
				if(!validateMp4File(destFile)) {
					logger_err(LOGM_TS_MUX, "Validate mp4 file failed.\n");
					muxRet = MUX_ERR_INVALID_FILE;
				}
			}
	    } while (false);	
	   
		return muxRet;
	}

	const char* GetOutExt(bool fAudioOnly)
	{
		switch (m_pref->GetInt("overall.container.format")) {
		case CF_FLV:
			return "flv";
		case CF_3GP:
			return "3gp";
		case CF_3GP2:
			return "3g2";
		case CF_F4V:
			return "f4v";
		case CF_HLS:
			return "mp4";
		default:
			return fAudioOnly ? "m4a" : "mp4";
		}
	}

private:
	int Tagging(const std::string &muxfile, bool is3GP)
	{
		std::ostringstream sstr;
		const char *strValue;
		int intValue;

		if (is3GP) {
			strValue = m_pref->GetString("overall.tagging.title");
			if ( strValue && *strValue) {
				sstr << " --3gp-title \"" << strValue << "\"";
			}

			strValue = m_pref->GetString("overall.tagging.artist");
			if (strValue && *strValue) {
				sstr << " --3gp-author \"" << strValue << "\"";
			}

			strValue = m_pref->GetString("overall.tagging.copyright");
			if (strValue && *strValue) {
				sstr << " --3gp-copyright \"" << strValue << "\"";
			}

			strValue = m_pref->GetString("overall.tagging.comment");
			if (strValue && *strValue) {
				sstr << " --3gp-description \"" << strValue << "\"";
			}

			intValue = m_pref->GetInt("overall.tagging.rating");
			if (intValue != PREF_ERR_NO_INTNODE) {
				if (intValue < 0) intValue = 0;
				else if (intValue > 5) intValue = 5;

				sstr << " --3gp-rating " << intValue;
			}
		} else {
			strValue = m_pref->GetString("overall.tagging.title");
			if ( strValue && *strValue) {
				sstr << " --title \"" << strValue << "\"";
			}

			strValue = m_pref->GetString("overall.tagging.artist");
			if (strValue && *strValue) {
				sstr << " --artist \"" << strValue << "\"";
			}

			strValue = m_pref->GetString("overall.tagging.copyright");
			if (strValue && *strValue) {
				sstr << " --copyright \"" << strValue << "\"";
			}

			strValue = m_pref->GetString("overall.tagging.description");
			if (strValue && *strValue) {
				sstr << " --description \"" << strValue << "\"";
			}
			//No rating tag for mp4 file?
		}

		if (sstr.str().empty()) {
			logger_warn(LOGM_TS_MUX, "No tags to be writed\n");
			return 0;
		}
		else {
			sstr << " --overWrite";
			std::string cmd = ATOMICPARSLEY;
			cmd += " \"";
			cmd += muxfile;
			cmd += "\"";
			cmd += sstr.str();

#ifdef DEBUG_EXTERNAL_CMD
			logger_info(LOGM_TS_MUX, "%s\n", cmd.c_str());
#endif

			return CProcessWrapper::Run(cmd.c_str());
		}
	}

	int muxHls(const char* inMp4File, const char* outMp4) 
	{
		if(!inMp4File || !*inMp4File || !outMp4 || !*outMp4) {
			logger_err(LOGM_TS_MUX, "Invalid path for muxing flv.\n");
			return MUX_ERR_INVALID_FILE_PATH;
		}
		int muxRet = MUX_ERR_MUXER_FAIL;
	
		const char* segDur = m_pref->GetString("muxer.hls.dur");
		const char* listSize = m_pref->GetString("muxer.hls.listsize");
		const char* postfix = m_pref->GetString("muxer.hls.postfix");
		const char* startIdx = m_pref->GetString("muxer.hls.startIndex");
		// Get dest file name and list file
		std::string destFile = outMp4;
		std::string finalM3u8File;
		size_t extPos = destFile.rfind(".mp4");
		if(extPos != std::string::npos) {
			destFile = destFile.substr(0, extPos);
			if(postfix && *postfix) {
				destFile += postfix;
			}
			// To insert '.' before %d of segment number, dest file for ffmpeg should add one more '.'
			finalM3u8File = destFile + ".m3u8";
			destFile += "..m3u8";
		}
		
		// Generate hls command line
		std::string hlsCmd = FFMPEG" -i \"";
		hlsCmd += inMp4File;
		hlsCmd += "\" -v error -map 0 -vcodec copy -acodec copy -vbsf h264_mp4toannexb -f hls ";
		if(segDur && *segDur) {
			hlsCmd += " -hls_time ";
			hlsCmd += segDur;
		}
		if(listSize && *listSize) {
			hlsCmd += " -hls_list_size ";
			hlsCmd += listSize;
		}
		if(startIdx && *startIdx) {
			hlsCmd += " -start_number ";
			hlsCmd += startIdx;
		}
		
		hlsCmd += " -y \"";
		hlsCmd += destFile + "\"";
#ifdef DEBUG_EXTERNAL_CMD
		logger_info(LOGM_TS_MUX, "%s\n", hlsCmd.c_str());
#endif
		muxRet = CProcessWrapper::Run(hlsCmd.c_str());
		if(muxRet != 0) {
			logger_err(LOGM_TS_MUX, "Failed to use ffmpeg to generate hls file.\n");
			return MUX_ERR_MUXER_FAIL;
		}

		//Correct m3u8 file, remove redundant '.' before ".m3u8"
		TsMoveFile(destFile.c_str(), finalM3u8File.c_str());
		return muxRet;
	}

	int muxFlv(const char* inMp4File, const char* outFlv) 
	{
		if(!inMp4File || !*inMp4File || !outFlv || !*outFlv) {
			logger_err(LOGM_TS_MUX, "Invalid path for muxing flv.\n");
			return MUX_ERR_INVALID_FILE_PATH;
		}
		int muxRet = MUX_ERR_MUXER_FAIL;
        std::string ffmpegCmd = FFMPEG;
		ffmpegCmd += " -i \"";
		ffmpegCmd += inMp4File;
		ffmpegCmd += "\" -vcodec copy -acodec copy \"";
		ffmpegCmd += outFlv;
		ffmpegCmd += "\"";
		muxRet = CProcessWrapper::Run(ffmpegCmd.c_str());
		if(muxRet != 0) {
			RemoveFile(outFlv);
			logger_err(LOGM_TS_MUX, "Failed to use ffmpeg to remux flv file.\n");
			return MUX_ERR_MUXER_FAIL;
		}

#ifdef _WIN32	// Flvmdi
		bool bEnableMdi = m_pref->GetBoolean("muxer.flvmdi.enabled");
		if(!bEnableMdi) return MUX_ERR_SUCCESS;
		std::string mdiCmd = FLVMID;
		mdiCmd += " \"";
		mdiCmd += outFlv;
		mdiCmd += "\"";
		bool bXml = m_pref->GetBoolean("muxer.flvmdi.xml");
		bool bLastKeyframe = m_pref->GetBoolean("muxer.flvmdi.lastkeyframe");
		bool bLastSec = m_pref->GetBoolean("muxer.flvmdi.lastsec");
		if(bXml) mdiCmd += " /x";
		if(bLastKeyframe) mdiCmd += " /k";
		if(bLastSec) mdiCmd += " /l";
		muxRet = CProcessWrapper::Run(mdiCmd.c_str());
		if(muxRet != 0) {
			logger_err(LOGM_TS_MUX, "Failed to use flvmdi to index flv file.\n");
			return MUX_ERR_MUXER_FAIL;
		}
#else
		bool bEnableMdi = m_pref->GetBoolean("muxer.yamdi.enabled");
		if(!bEnableMdi) return MUX_ERR_SUCCESS;
		std::string mdiCmd = YAMDI;
		mdiCmd += " -i \"";
		mdiCmd += outFlv;
		mdiCmd += "\" -o \"";
		mdiCmd += inMp4File;
		mdiCmd += ".flv\"";
		bool bXml = m_pref->GetBoolean("muxer.yamdi.xml");
		bool bLastKeyframe = m_pref->GetBoolean("muxer.yamdi.lastkeyframe");
		bool bLastSec = m_pref->GetBoolean("muxer.yamdi.lastsec");
		const char* creatorInfo = m_pref->GetString("muxer.yamdi.creator");
		bool omitKeyframeInfo = m_pref->GetBoolean("muxer.yamdi.omitkeframe");
		if(bXml) mdiCmd += " -x";
		if(bLastKeyframe) mdiCmd += " -k";
		if(bLastSec) mdiCmd += " -s";
		if(omitKeyframeInfo) mdiCmd += " -X";
		if(creatorInfo && *creatorInfo) {
			mdiCmd += " -c ";
			mdiCmd += creatorInfo;
		}
#endif

#ifdef DEBUG_EXTERNAL_CMD
		logger_info(LOGM_TS_MUX, "%s\n", mdiCmd.c_str());
#endif
		muxRet = CProcessWrapper::Run(mdiCmd.c_str());
		if(muxRet != 0) {
			logger_err(LOGM_TS_MUX, "Failed to use yamdi to index flv file.\n");
			return MUX_ERR_MUXER_FAIL;
		}

#ifndef _WIN32
		std::string tempFlv = inMp4File;
		tempFlv += ".flv";
		if (!TsMoveFile(tempFlv.c_str(), outFlv)) {
                	logger_err(LOGM_TS_MUX, MOVE_ERROR_FORMAT, tempFlv.c_str(), outFlv);
			return MUX_ERR_MUXER_FAIL;
		}
#endif
		return muxRet;
	}
};


static int SaveTextFile(const char* filename, const char* text)
{
	FILE *fp = ts_fopen(filename, "w");
	if (fp == NULL) return -1;

	fprintf(fp, "%s", text);
	fclose(fp);
	return 0;
}

class CTSMuxer: public CMuxer
{
public:
	CTSMuxer() {m_muxInfoSizeRatio = 0.025f;}

	const char* GetOutExt(bool fAudioOnly)
	{
		switch (m_pref->GetInt("muxer.tsmuxer.output")) {
		case 1:
			return "m2ts";
		default:
			return "ts";
		}
	}

	int Mux()
	{
		if(!m_pFileQueue) {
			logger_err(LOGM_TS_MUX, "FileQueue hasn't been set.\n");
			return MUX_ERR_INVALID_FILE_QUEUE;
		}
		std::ostringstream meta;
		meta << "MUXOPT --no-pcr-on-video-pid --new-audio-pes "; // 
		switch (m_pref->GetInt("muxer.tsmuxer.mode")) {
		case 0:
			meta << "--vbr";
			break;
		case 1: {
			int vbrMin = m_pref->GetInt("muxer.tsmuxer.vbrmin");
			int vbrMax = m_pref->GetInt("muxer.tsmuxer.vbrmax");
			meta << "--vbr";
			if(vbrMin > 0) meta << " --minbitrate=" << vbrMin;
			if(vbrMax > 0) meta << " --maxbitrate=" << vbrMax;
			} break;
		case 2:
			meta << "--cbr --bitrate=";
			if (m_pref->GetInt("muxer.tsmuxer.cbrrate") > 0) {
				meta << m_pref->GetInt("muxer.tsmuxer.cbrrate");
			} else {
				meta << m_pref->GetInt("overall.video.bitrate");
			}
			if(m_pref->GetInt("muxer.tsmuxer.vbvbuf") > 0) {
				meta << " --vbv-len=" << m_pref->GetInt("muxer.tsmuxer.vbvbuf");
			}
			
			break;
		default: //default vbr
			meta << "--vbr";
			break;			
		}
		meta << std::endl;

		size_t streamIdx = 0;
		for (CFileQueue::queue_item_t* item = m_pFileQueue->GetFirst(ST_VIDEO); item; item = m_pFileQueue->GetNext(ST_VIDEO)) {
			if(ContainStream(ST_VIDEO, streamIdx)) {
				video_format_t vFormat = VC_H264;
				if (item->vinfo) {
					vFormat = item->vinfo->format;
				} else {
					std::string videoFileExt;
					StrPro::StrHelper::getFileExt(item->fileName.c_str(), videoFileExt);
					if(_stricmp(videoFileExt.c_str(), ".264") == 0) vFormat = VC_H264;
					else if(_stricmp(videoFileExt.c_str(), ".m1v") == 0) vFormat = VC_MPEG1;
					else if(_stricmp(videoFileExt.c_str(), ".m2v") == 0) vFormat = VC_MPEG2;
				}
				switch (vFormat) {
				case VC_H264:
					meta << "V_MPEG4/ISO/AVC, \"" << item->fileName << "\"";
					if (m_pref->GetBoolean("muxer.tsmuxer.pictiming")) meta << ",insertSEI";
					if (m_pref->GetBoolean("muxer.tsmuxer.sps")) meta << ",contSPS";
					break;
				case VC_MPEG1:
					meta << "V_MPEG-1, \"" << item->fileName << "\"";
					break;
				case VC_MPEG2:
					meta << "V_MPEG-2, \"" << item->fileName << "\"";
					break;
				default:
					logger_warn(LOGM_TS_MUX, "Unsupported video type%d.\n", vFormat);
					break;
				}
				if(item->vinfo && item->vinfo->fps_out.num > 0) {
					meta << ",fps=" << (float)item->vinfo->fps_out.num/item->vinfo->fps_out.den;
				}
				meta << std::endl;
			}
			streamIdx++;
		}

		streamIdx = 0;
		for (CFileQueue::queue_item_t* item = m_pFileQueue->GetFirst(ST_AUDIO); item; item = m_pFileQueue->GetNext(ST_AUDIO)) {
			if(ContainStream(ST_AUDIO, streamIdx)) {
				audio_format_t aFormat = AC_AC3;
				if (item->ainfo) {
					aFormat = item->ainfo->format;
				} else {
					std::string audioFileExt;
					StrPro::StrHelper::getFileExt(item->fileName.c_str(), audioFileExt);
					if(_stricmp(audioFileExt.c_str(), ".ac3") == 0) aFormat = AC_AC3;
					else if(_stricmp(audioFileExt.c_str(), ".aac") == 0) aFormat = AC_AAC_LC;
					else if(_stricmp(audioFileExt.c_str(), ".m4a") == 0) aFormat = AC_AAC_HE;
					else if(_stricmp(audioFileExt.c_str(), ".m2a") == 0 ||
						_stricmp(audioFileExt.c_str(), ".mp3") == 0 ||
						_stricmp(audioFileExt.c_str(), ".mp2") == 0) {
						aFormat = AC_MP3;
					}
				}
				switch (aFormat) {
				case AC_AC3:
					meta << "A_AC3, \"" << item->fileName << "\"" << std::endl;
					break;
				case AC_AAC_LC:
					meta << "A_AAC, \"" << item->fileName << "\"" << std::endl;
					break;
				case AC_AAC_HE:
				case AC_AAC_HEV2:
					meta << "A_AAC, \"" << item->fileName << "\", track=1" << std::endl;
					break;
				case AC_MP2:
				case AC_MP3:
					meta << "A_MP3, \"" << item->fileName << "\"" << std::endl;
					break;
				default:
					logger_warn(LOGM_TS_MUX, "Unsupported audio type %d.\n", aFormat);
					break;
				}
			}
			streamIdx++;
		}

		std::string metafile = m_pFileQueue->GetStreamFileName(ST_MUXER, 0, 0, "meta");

		if (!SaveTextFile(metafile.c_str(), meta.str().c_str())) {
			std::ostringstream s;
			std::string muxfile = m_pFileQueue->GetStreamFileName(ST_MUXER, MUX_MP4, 0, GetOutExt(false));
			s << "\"" << TSMUXER << "\" \"" << metafile << "\" \"" << muxfile << "\"";

#ifdef DEBUG_EXTERNAL_CMD
			logger_info(LOGM_TS_MUX, "%s\n", s.str().c_str());
#endif
 			int muxRet = CProcessWrapper::Run(s.str().c_str());
			if(!m_pFileQueue->GetKeepTemp()) RemoveFile(metafile.c_str());
			if(muxRet != MUX_ERR_SUCCESS) {
				logger_err(LOGM_TS_MUX, "Failed to mux with tsMuxer, return code:%d.\n", muxRet);
				return MUX_ERR_MUXER_FAIL;
			}
			
			std::string outputUrl = m_pFileQueue->GetCurDestFile();
			RemoveFile(outputUrl.c_str());
			Sleep(WAIT_INTERVAL_MS);
			if (!TsMoveFile(muxfile.c_str(), outputUrl.c_str())) {
				logger_err(LOGM_TS_MUX, MOVE_ERROR_FORMAT, muxfile.c_str(), outputUrl.c_str());
                if (!TsCopyFile(muxfile.c_str(), outputUrl.c_str())) {
					logger_err(LOGM_TS_MUX, "Failed to copy.\n");
					return MUX_ERR_MUXER_FAIL;
				}
            }
			return muxRet;
		} 
		return false;
	}
};

class CMatroska  : public CMuxer
{
public:
	CMatroska() {m_muxInfoSizeRatio = 0.012f;}

	const char* GetOutExt(bool fAudioOnly) {
		if(fAudioOnly) return "mka";
		return "mkv";
	}

	int Mux()
	{
		if(!m_pFileQueue) {
			logger_err(LOGM_TS_MUX, "FileQueue hasn't been set.\n");
			return MUX_ERR_INVALID_FILE_QUEUE;
		}
		std::ostringstream sstr;
		size_t acount = 0, vcount = 0;
		sstr << "\""MKVMERGE"\"";
		for (CFileQueue::queue_item_t* item = m_pFileQueue->GetFirst(ST_VIDEO); item; item = m_pFileQueue->GetNext(ST_VIDEO)) {
			if(ContainStream(ST_VIDEO, vcount)) {
				if(item->vinfo) {
					sstr << " --aspect-ratio 0:" << item->vinfo->dest_dar.num << '/' << item->vinfo->dest_dar.den;
					sstr << " --default-duration 0:" << ((double)item->vinfo->fps_out.num / item->vinfo->fps_out.den) << "fps";
				} 
				sstr << " -A -S \"" << item->fileName << "\"";
			}
			vcount++;
		}
		for (CFileQueue::queue_item_t* item = m_pFileQueue->GetFirst(ST_AUDIO); item; item = m_pFileQueue->GetNext(ST_AUDIO)) {
			if(ContainStream(ST_AUDIO, acount)) {
				if(item->ainfo) {
					switch (item->ainfo->format) {
					case AC_AAC_LC:
					case AC_AAC_HE:
					case AC_AAC_HEV2:
						sstr << " --aac-is-sbr " << "0:0";
						break;
					}
				} else {
					std::string audioFileExt;
					StrPro::StrHelper::getFileExt(item->fileName.c_str(), audioFileExt);
					if(_stricmp(audioFileExt.c_str(), ".aac") == 0 || _stricmp(audioFileExt.c_str(), ".m4a") == 0){
						sstr << " --aac-is-sbr " << "0:0";
					}
				}
				
				sstr << " -D -S \"" << item->fileName << "\"";
			}
			acount++;
		}
		std::string muxfile = m_pFileQueue->GetStreamFileName(ST_MUXER, MUX_MKV, 0, GetOutExt(false));
		sstr << " -o \"" << muxfile << "\"";

#ifdef DEBUG_EXTERNAL_CMD
		logger_info(LOGM_TS_MUX, "MKV Cmd: %s\n", sstr.str().c_str());
#endif

		if(CProcessWrapper::Run(sstr.str().c_str()) == 2 ) {  // MKV returns 2 if fail
			logger_err(LOGM_TS_MUX, "Failed to mux with MKV.");
			return MUX_ERR_MUXER_FAIL;
		}

		std::string outputUrl = m_pFileQueue->GetCurDestFile();
		RemoveFile(outputUrl.c_str());
		Sleep(WAIT_INTERVAL_MS);
		if (!TsMoveFile(muxfile.c_str(), outputUrl.c_str())) {
			logger_err(LOGM_TS_MUX, MOVE_ERROR_FORMAT, muxfile.c_str(), outputUrl.c_str());
            if (!TsCopyFile(muxfile.c_str(), outputUrl.c_str())) {
				logger_err(LOGM_TS_MUX, "Failed to copy.\n");
				return MUX_ERR_MUXER_FAIL;
			}
        }

		return MUX_ERR_SUCCESS;
	}
};

class CFFmpegMuxer : public CMuxer
{
public:
	CFFmpegMuxer() {m_muxInfoSizeRatio = 0.02f;}

	const char* GetOutExt(bool fAudioOnly) 
	{
		return GetFFMpegOutFileExt(m_pref->GetInt("overall.container.format"), fAudioOnly);
	}
	
	bool copyAVStream() 
	{
		std::string outputUrl = m_pFileQueue->GetCurDestFile();
		std::string outDir, fileTitle, tmpExt;
		bool ret = StrPro::StrHelper::splitFileName(outputUrl.c_str(), outDir, fileTitle, tmpExt);
		if(!ret) {
			logger_err(LOGM_TS_MUX, "Invalid output path.\n");
			return ret;
		}

		// Copy audio stream
		int acount = 0;
		for (CFileQueue::queue_item_t* item = m_pFileQueue->GetFirst(ST_AUDIO); item; item = m_pFileQueue->GetNext(ST_AUDIO)) {
			if(ContainStream(ST_AUDIO, acount)) {
				std::string audioTmpFile = item->fileName;
				std::string audioExt;
				if(StrPro::StrHelper::getFileExt(audioTmpFile.c_str(), audioExt)) {
					std::string audioFile = outDir + fileTitle + audioExt;
					RemoveFile(audioFile.c_str());
					if (!TsMoveFile(audioTmpFile.c_str(), audioFile.c_str())) {
						logger_err(LOGM_TS_MUX, MOVE_ERROR_FORMAT, audioTmpFile.c_str(), audioFile.c_str());
                        if (!TsCopyFile(audioTmpFile.c_str(), audioFile.c_str())) {
							logger_err(LOGM_TS_MUX, "Failed to copy.\n");
							ret = false;
							break;
						}
					}
				}
			}
			acount++;
		}

		if(!ret) return false;

		// Copy video stream
		int vcount = 0;
		for (CFileQueue::queue_item_t* item = m_pFileQueue->GetFirst(ST_VIDEO); item; item = m_pFileQueue->GetNext(ST_VIDEO)) {
			std::string videoTmpFile = item->fileName;
			std::string videoExt;
			if(StrPro::StrHelper::getFileExt(videoTmpFile.c_str(), videoExt)) {
				std::string videoFile = outDir + fileTitle + videoExt;
				RemoveFile(videoFile.c_str());
				if (!TsMoveFile(videoTmpFile.c_str(), videoFile.c_str())) {
					logger_err(LOGM_TS_MUX, MOVE_ERROR_FORMAT, videoTmpFile.c_str(), videoFile.c_str());
                    if (!TsCopyFile(videoTmpFile.c_str(), videoFile.c_str())) {
						logger_err(LOGM_TS_MUX, "Failed to copy.\n");
						ret = false;
						break;
					}
				}
			}
			vcount++;
		}

		return ret;
	}

	int Mux()
	{
		if(!m_pFileQueue) {
			logger_err(LOGM_TS_MUX, "FileQueue hasn't been set.\n");
			return MUX_ERR_INVALID_FILE_QUEUE;
		}

		int containerFormat = m_pref->GetInt("overall.container.format");
		// Detach audio/video stream separately
		if(containerFormat == CF_RAW) {
			return copyAVStream() ? MUX_ERR_SUCCESS : MUX_ERR_MUXER_FAIL;
		}

		std::ostringstream sstr;
		std::stringstream videoTrackStr, audioTrackStr;
		size_t acount = 0, vcount = 0;
		//std::string videoSetTimeStr;
		bool useAACBsf = false;
		for (CFileQueue::queue_item_t* item = m_pFileQueue->GetFirst(ST_AUDIO); item; item = m_pFileQueue->GetNext(ST_AUDIO)) {
			if(ContainStream(ST_AUDIO, acount)) {
				if(item->ainfo->encoder_type == AE_FAAC) {
					useAACBsf = true;
				}
				audioTrackStr << " -i \"" << item->fileName << "\"";
			}
			acount++;
		}
		for (CFileQueue::queue_item_t* item = m_pFileQueue->GetFirst(ST_VIDEO); item; item = m_pFileQueue->GetNext(ST_VIDEO)) {
			if(ContainStream(ST_VIDEO, vcount)) {
				//if(item->vinfo->format == VC_XVID && containerFormat == CF_3GP) {	// Set timebase
				//	videoSetTimeStr = " -time_base:0 ";
				//	char timebaseStr[32] = {0};
				//	int fpsDen = item->vinfo->fps_out.den, fpsNum = item->vinfo->fps_out.num;
				//	if(fpsDen < 100) {
				//		fpsDen *= 1000;
				//		fpsNum *= 1000;
				//	}
				//	sprintf(timebaseStr, "%d/%d", fpsDen, fpsNum);
				//	videoSetTimeStr += timebaseStr;
				//}
				videoTrackStr << " -i \"" << item->fileName << "\"";
			}
			vcount++;
		}

		if (audioTrackStr.str().empty() && videoTrackStr.str().empty()) {
			logger_err(LOGM_TS_MUX, "NO video or audio stream is seleted in task configure");
			return MUX_ERR_MUXER_FAIL;
		}

        sstr << FFMPEG;
		int order = m_pref->GetInt("overall.container.trackOrder");
		if ( order == 0) {	//video first
			sstr << videoTrackStr.str() << audioTrackStr.str();
		}
		else {				//audio first
			sstr << audioTrackStr.str() << videoTrackStr.str();
		}
		/*if(!videoSetTimeStr.empty()) {
			sstr << videoSetTimeStr;
		}*/
		if(vcount+acount > 2) {
			for(size_t i=0; i<vcount+acount; ++i) {
				sstr << " -map " << i << ":0";
			}
		}
		sstr << " -c copy";
		if(useAACBsf) sstr << " -bsf:a aac_adtstoasc";
		const char *s = GetFFMpegFormatName(containerFormat);
		if (s) {
			sstr << " -f " << s;
		}
		if ( (s = m_pref->GetString("overall.video.fourcc")) && *s) {
			sstr << " -vtag " << s;
		}
		std::string muxfile = m_pFileQueue->GetStreamFileName(ST_MUXER, MUX_FFMPEG, 0, GetOutExt(false));
		sstr << " -y \"" << muxfile << "\"";
		

#ifdef DEBUG_EXTERNAL_CMD
		logger_info(LOGM_TS_MUX, "%s\n", sstr.str().c_str());
#endif
		int muxRet = CProcessWrapper::Run(sstr.str().c_str()); // ffmpeg returns 0 on success
		if(muxRet != MUX_ERR_SUCCESS) {
			logger_err(LOGM_TS_MUX, "Failed to mux with FFMpeg, return code:%d.\n", muxRet);
			return MUX_ERR_MUXER_FAIL;
		}
		//---------- Post processing -----------------
		if (containerFormat == CF_ASF) {
			if (!reindexWmv(muxfile)) {
				logger_warn(LOGM_TS_MUX, "reindex wmv file failed.\n");
			}
		}

		std::string outputUrl = m_pFileQueue->GetCurDestFile();
		RemoveFile(outputUrl.c_str());
		Sleep(WAIT_INTERVAL_MS);
		if (!TsMoveFile(muxfile.c_str(), outputUrl.c_str())) {
			logger_err(LOGM_TS_MUX, MOVE_ERROR_FORMAT, muxfile.c_str(), outputUrl.c_str());
            if (!TsCopyFile(muxfile.c_str(), outputUrl.c_str())) {
				logger_err(LOGM_TS_MUX, "Failed to copy.\n");
				return MUX_ERR_MUXER_FAIL;
			}
        }
		return MUX_ERR_SUCCESS;
	}

private:
	bool reindexWmv(const std::string muxfile)
	{
		if (muxfile.empty()) return false;
	
		std::ostringstream sstr;

		std::string tempOutput(muxfile);
		std::string cmd;
		
		tempOutput.append(".tmp");

		sstr << ASFBIN;
		sstr << " -i \"" << muxfile << "\""; //input
		sstr << " -o \"" << tempOutput << "\""; //output
		sstr << " -forceindex"; //force reindex switch

#ifdef DEBUG_EXTERNAL_CMD
		logger_warn(LOGM_TS_MUX, "%s\n", sstr.str().c_str());
#endif

		int ret = CProcessWrapper::Run(sstr.str().c_str());
		if (ret == 0) { // OK
			RemoveFile(muxfile.c_str());
			return TsMoveFile(tempOutput.c_str(), muxfile.c_str());
		}

		return false;
	}
};

class CDummyMuxer: public CMuxer
{
	int m_muxCount;
public:
	CDummyMuxer():m_muxCount(0) {m_muxInfoSizeRatio = 0.f;}

	int Mux()
	{
		if(!m_pFileQueue) {
			logger_err(LOGM_TS_MUX, "FileQueue hasn't been set.\n");
			return MUX_ERR_INVALID_FILE_QUEUE;
		}
		std::string destFile = m_pFileQueue->GetCurDestFile();
		if(destFile.empty()) return false;
		CFileQueue::queue_item_t* item = NULL;
		
		if(m_muxCount == 0) {	// Mux first audio
			item = m_pFileQueue->GetFirst(ST_AUDIO);
		} else {
			item = m_pFileQueue->GetNext(ST_AUDIO);
		}
		if(!item) return MUX_ERR_MUXER_FAIL;

		RemoveFile(destFile.c_str());
		if (!TsMoveFile(item->fileName.c_str(), destFile.c_str())) {
            if(!TsCopyFile(item->fileName.c_str(), destFile.c_str())) {
				logger_err(LOGM_TS_MUX, "Generate final music file failed.\n");
				return MUX_ERR_MUXER_FAIL;
			}
		}

		m_muxCount++;
		if(m_muxCount == m_pFileQueue->GetAudioItemCount()) {	
			// If all audio tracks in audio items are muxed, reset counter
			m_muxCount = 0;
		}

		return MUX_ERR_SUCCESS;
	}

	const char* GetOutExt(bool fAudioOnly) 
	{ 
		audio_format_t aFormat = (audio_format_t)m_pref->GetInt("overall.audio.format");
		CFileQueue::queue_item_t* item = m_pFileQueue->GetFirst(ST_AUDIO);
		if(item) aFormat = item->ainfo->format;
		
		switch(aFormat) {
			case AC_MP3:
				return "mp3";
			case AC_VORBIS:
				return "ogg";
			case AC_AAC_LC:
				return "aac";
			case AC_AAC_HE:
			case AC_AAC_HEV2:
				return "m4a";
			case AC_WMA7:
			case AC_WMA8:
			case AC_WMA9:
				return "wma";
			case AC_AC3:
				return "ac3";
			case AC_EAC3:
				return "ec3";
			case AC_MP2:
				return "mp2";
			case AC_MPC:
				break;
			case AC_SPEEX:
				break;
			case AC_AMR:
				return "amr";
			case AC_PCM:
				return "wav";
			case AC_FLAC:
				return "flac";
		}
		return "";
	}
};

#ifdef HAVE_MII_ENCODER
class CMgpMuxer : public CMuxer
{
public:
	CMgpMuxer() {m_muxInfoSizeRatio = 0.02f;}

	const char* GetOutExt(bool fAudioOnly) 
	{
		return "mgp";
	}
	
	int Mux()
	{
		if(!m_pFileQueue) {
			logger_err(LOGM_TS_MUX, "FileQueue hasn't been set.\n");
			return MUX_ERR_INVALID_FILE_QUEUE;
		}

		CFileQueue::queue_item_t* audioItem = m_pFileQueue->GetFirst(ST_AUDIO);
		CFileQueue::queue_item_t* videoItem = m_pFileQueue->GetFirst(ST_VIDEO); 			

		if(audioItem && videoItem) {
			mpgstream_desc_t streams[2] = {0};
			stream_urls_t urls = {0};

			streams[0].type = MBMMGP_Stream_type_video;
			streams[0].keyflag = 1;
			streams[0].streamId = 1;
			streams[0].info.v.coding = MBMMGP_Coding_mii;
			streams[0].info.v.desc.bitrate = videoItem->vinfo->bitrate;
			int fpsNum = videoItem->vinfo->fps_out.num;
			int fpsDen = videoItem->vinfo->fps_out.den;
			streams[0].info.v.desc.frameintv = 1000*fpsDen/fpsNum;
			streams[0].info.v.desc.width = (short)(videoItem->vinfo->res_out.width);
			streams[0].info.v.desc.height = (short)(videoItem->vinfo->res_out.height);

			switch(audioItem->ainfo->format) {
			case AC_AAC_LC:
			case AC_AAC_HE:
			case AC_AAC_HEV2:
				streams[1].info.a.coding = MBMMGP_Coding_aacplus;
				break;
			case AC_AMR:
				streams[1].info.a.coding = MBMMGP_Coding_amr;
				break;
			}
			streams[1].type = MBMMGP_Stream_type_audio;
			streams[1].keyflag = 0;
			streams[1].streamId = 2;
			streams[1].info.a.desc.channels = audioItem->ainfo->out_channels;
			streams[1].info.a.desc.encodemode = 1;
			streams[1].info.a.desc.samplerate = audioItem->ainfo->out_srate;
			streams[1].info.a.desc.samplebits = 16;

			strncpy(urls.audEsUrl, audioItem->fileName.c_str(), 255);
			strncpy(urls.vdoEsUrl, videoItem->fileName.c_str(), 255);
			strncpy(urls.mgpFileUrl, m_pFileQueue->GetCurDestFile(), 255);

			HANDLE hdl = MbmmgpMachineOpen(streams, 2, &urls);
			if(!hdl) {
				logger_err(LOGM_TS_MUX, "Open Mgp muxer failed!");
				return MUX_ERR_MUXER_FAIL;
			}
			int ret = MbmmgpMachineRun(hdl);
			if(ret != 0) {
				logger_err(LOGM_TS_MUX, "Mgp muxing failed, return code:%d.", ret);
				MbmmgpMachineClose(hdl);
				return MUX_ERR_MUXER_FAIL;
			}
			MbmmgpMachineClose(hdl);
		}
		
		return MUX_ERR_SUCCESS;
	}
};
#endif

class CFlvMuxer : public CMuxer
{
public:
	CFlvMuxer() {m_muxInfoSizeRatio = 0.02f;}

	const char* GetOutExt(bool fAudioOnly) 
	{
		return "flv";
	}
	
	int Mux()
	{
		if(!m_pFileQueue) {
			logger_err(LOGM_TS_MUX, "FileQueue hasn't been set.\n");
			return MUX_ERR_INVALID_FILE_QUEUE;
		}

		int ret = MUX_ERR_INVALID_FILE;
		do {
			CFileQueue::queue_item_t* audioItem = m_pFileQueue->GetFirst(ST_AUDIO);
			CFileQueue::queue_item_t* videoItem = m_pFileQueue->GetFirst(ST_VIDEO); 			

			if(audioItem && videoItem) {
				FileMixer mixer;
				if(!mixer.Parse264File(videoItem->fileName.c_str())) {
					logger_err(LOGM_TS_MUX, "Parse 264 file failed.\n");
					break;
				}

#ifdef _WIN32
				if(audioItem->ainfo->format == AC_AAC_HE || 
					audioItem->ainfo->format == AC_AAC_HEV2) {
					if(!mixer.ParseAACFile(audioItem->fileName.c_str())) {
						logger_err(LOGM_TS_MUX, "Parse aac file failed.\n");
						break;
					}
				} else
#endif
				{
					if(!mixer.ParseADTS(audioItem->fileName.c_str())) {
						logger_err(LOGM_TS_MUX, "Parse aac file failed.\n");
						break;
					}
				}

				const char* destFile = m_pFileQueue->GetCurDestFile();
				if(!mixer.WriteOutPutFile(destFile)) {
					logger_err(LOGM_TS_MUX, "Write flv file failed.\n");
					break;
				}

				ret = MUX_ERR_SUCCESS;
			}
		} while(false);

		return ret;
	}
};

CMuxer* CMuxerFactory::CreateInstance(int type)
{
	CMuxer* muxer;

	switch (type) {
#ifdef PRODUCT_MEDIACODERDEVICE		// Device version only support mp4 format
	case MUX_MP4:		muxer = new CMp4box(); break;
	case MUX_MKV:		muxer = new CMatroska(); break;
#else
	case MUX_MP4:		muxer = new CMp4box(); break;
	case MUX_TSMUXER:	muxer = new CTSMuxer();	break;
	case MUX_FFMPEG:	muxer = new CFFmpegMuxer();	break;
	case MUX_DUMMY:     muxer = new CDummyMuxer(); break;
	case MUX_MKV:		muxer = new CMatroska(); break;
	case MUX_FLV:		muxer = new CFlvMuxer(); break;
#ifdef HAVE_WMENCODER
	case MUX_WM:		muxer = new CWMMuxer(); break;
#endif
#ifdef HAVE_MII_ENCODER
	case MUX_MGP:		muxer = new CMgpMuxer(); break;
#endif
	/*
	case MUX_MP4CREATOR:muxer = new CMp4Creator();		break;
	case MUX_MENCODER:	muxer = new CMencoderMuxer();	break;
	*/
#endif
	
	default:
		muxer = 0;
	}
	if (muxer) muxer->SetMuxerType(type);
	return muxer;
}
