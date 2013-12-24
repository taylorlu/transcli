#include <sstream>
#include <time.h>
#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif
#include "bit_osdep.h"
#include "FileQueue.h"
#include "muxers.h"
#include "logger.h"
#include "StrHelper.h"
#include "util.h"

using namespace StrPro;
//#define NO_ES_DELETION

CFileQueue::CFileQueue(void):m_workerId(0), m_srcFileIndex(0)
{
	m_audioIter = m_audioItems.begin();
	m_videoIter = m_videoItems.begin();
	m_subIter = m_subItems.begin();
	m_destFileIter = m_destFiles.begin();
	m_curProcessId = _getpid();
	SetTempDir(""); //set default temp dir
	m_keepTemp = false;
}

CFileQueue::~CFileQueue(void)
{
	Clear();
}

bool CFileQueue::SetTempDir(const char* dir)
{
	if(!dir || !(*dir)) {
		m_tempDir.clear();
	} else {
		m_tempDir = dir;
	}

	if(m_tempDir.empty()) {
		char* sysTmp = getenv("TEMP");
		if(sysTmp) {
			m_tempDir = sysTmp;
		}
#ifndef WIN32
		else {
			m_tempDir = "/tmp";
		}
#endif
	}

	if(!m_tempDir.empty()) {
		if(!FolderExist(m_tempDir.c_str())) {
			logger_warn(LOGM_TS_FQ, "Invalid temp path: %s\n, try to make it.", m_tempDir.c_str());
			if(MakeDirRecursively(m_tempDir.c_str()) != 0) {
				logger_err(LOGM_TS_FQ, "Try to make temp folder failed.");
				return false;
			}
		}
		size_t strLen = m_tempDir.length();
		if(m_tempDir.at(strLen-1) != PATH_DELIMITER) {
			m_tempDir += PATH_DELIMITER;
		}
		return true;
	}
	return false;
}

void CFileQueue::CleanTempDir()
{
	SYSTEMTIME curSysTime;
	GetSystemTime(&curSysTime);
	// Every 2 hours execute once
	if(curSysTime.wMinute < 30) return;

	const char* cleanTypes = ".264,.aac,.m4a,.mp4,.avs,.flv,.wmv,.m2v,.mp3,.m4v,.ac3,.wma,.f4v,.ffindex,.mpg,.ts";
	const char* fileContain = "x264stat_";
	WIN32_FIND_DATAA FindFileData;
	HANDLE hFind = NULL;
	bool ret = true;

	string findPath(m_tempDir);
	findPath.push_back('*');

	hFind = FindFirstFileA(findPath.c_str(), &FindFileData);   

	if (hFind == INVALID_HANDLE_VALUE) {
		logger_warn(LOGM_TS_FQ, "Invalid find path: %s\n", findPath.c_str());
		return ;
	}

	std::vector<std::string> allCleanFiles;
	do {
		if((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
			string fullPath(m_tempDir);
			fullPath.append(FindFileData.cFileName);
			if (!MatchFilterCase(FindFileData.cFileName, cleanTypes) &&
				!strstr(FindFileData.cFileName, fileContain)) continue;
	
			SYSTEMTIME fileSysTime;
			FileTimeToSystemTime(&(FindFileData.ftLastWriteTime), &fileSysTime);
			int outdateInterval = 4;	// 4 hours
			if(fullPath.compare(".ffindex") == 0) {		// FFMS index file need more time to outdate
				outdateInterval = 24;	// 24 hours
			}
			if(curSysTime.wHour - fileSysTime.wHour >= outdateInterval || (
			   curSysTime.wDay > fileSysTime.wDay && curSysTime.wHour+24-fileSysTime.wHour >= outdateInterval)) {
				allCleanFiles.push_back(fullPath);
			}
		} 
	} while(FindNextFileA(hFind, &FindFileData));
	FindClose(hFind);

	// Delete outdate temp file
	for (size_t i=0; i<allCleanFiles.size(); ++i) {
		RemoveFile(allCleanFiles[i].c_str());
		Sleep(10);
	}
	return ;
}

std::string CFileQueue::GetEncoderStatFile(int videoStreamId)
{
	__int64 curTime = GetTickCount();
	std::stringstream tmpStr;
	tmpStr<<m_tempDir<<"x264stat_"<<m_curProcessId<<"_"<<curTime<<"_"<<m_workerId<<"_"<<videoStreamId<<".log";
	std::string retStr = tmpStr.str();
	m_encoderStatFiles.push_back(retStr);
	m_encoderStatFiles.push_back(retStr+".mbtree");
	return retStr;
}

// Rename firstPass x264 stat file(when transcode multi file, x264 rename will fail)
void CFileQueue::RenameX264StatFile()
{
	for (size_t i=0; i<m_encoderStatFiles.size(); ++i) {
		std::string& curfile = m_encoderStatFiles[i];
		if(!FileExist(curfile.c_str())) {
			std::string tmpFile = curfile + ".temp";
			if(FileExist(tmpFile.c_str())) {
				if(TsMoveFile(tmpFile.c_str(), curfile.c_str())) {
					logger_info(LOGM_TS_FQ, "Rename x264 stat file success!\n");
				} else {
					if(!CopyFile(tmpFile.c_str(), curfile.c_str(), TRUE)) {
						logger_err(LOGM_TS_FQ, "Rename or copy x264 stat file failed!\n");
					}
				}
			}
		}
	}
}

std::string CFileQueue::GetStreamFileName(stream_type_t streamType, int format, int streamId, const char* ext)
{
	__int64 curTime = GetTickCount();
	std::stringstream tmpStr;

	if(ext) {
		std::string temExt(ext);
		if(temExt.at(0) != '.') {
			temExt.insert(temExt.begin(), '.');
		}
		tmpStr << m_tempDir << m_curProcessId << "_" << curTime << "_" << m_workerId << "_" <<streamId<< temExt;
		return tmpStr.str();
	}

	std::string strExt;
	switch (streamType) {
	case ST_VIDEO:
		switch(format) {
		case VC_H264:
			strExt = ".264";
			break;
		case VC_HEVC:
			strExt = ".265";
			break;
		case VC_XVID:
			strExt = ".m4v";
			break;
		case VC_FLV:
			strExt = ".flv";
			break;
		case VC_H263:
			strExt = ".263";
			break;
		case VC_H263P:
			strExt = ".263p";
			break;
		case VC_WMV7:
		case VC_WMV8:
		case VC_WMV9:
			strExt = ".wmv";
			break;
		case VC_VC1:
			strExt = ".vc1";
			break;
		case VC_MPEG1:
			strExt = ".m1v";
			break;
		case VC_MPEG2:
			strExt = ".m2v";
			break;
		case VC_MPEG4:
			strExt = ".m4v";
			break;
		case VC_MII:
			strExt = ".mii";
			break;
		case VC_CAVS:
			strExt = ".cavs";
			break;
		}
		break;
		
	case ST_AUDIO:
		switch(format) {
		case AC_AAC_LC:
			strExt = ".aac";
			break;
		case AC_AAC_HE:
			strExt = ".m4a";
			break;
		case AC_AAC_HEV2:
			strExt = ".m4a";
			break;
		case AC_MP3:
			strExt = ".mp3";
			break;
		case AC_AC3:
			strExt = ".ac3";
			break;
		case AC_MP2:
			strExt = ".m2a";
			break;
		case AC_WMA7:
		case AC_WMA8:
		case AC_WMA9:
			strExt = ".wma";
			break;
		case AC_FLAC:
			strExt = ".flac";
			break;
		case AC_APE:
			strExt = ".ape";
			break; 
		case AC_PCM:
			strExt = ".wav";
			break;
		case AC_AMR:
			strExt = ".amr";
			break;
		case AC_EAC3:
			strExt = ".ec3";
			break;
		case AC_DTS:
			strExt = ".dts";
			break;
		}
		break;

	case ST_MUXER:
		switch(format) {
		case MUX_MP4 :
			strExt = ".mp4";
			break;
		case MUX_MKV:
			strExt = ".mkv";
			break;
		case MUX_TSMUXER:
			strExt = ".ts";
			break;
		case MUX_PMP:
			strExt = ".pmp";
			break;
		case MUX_MP4CREATOR:
			strExt = ".mp4";
			break;
		case MUX_VCD:
			break;
		}
		break;
	}

	if(!strExt.empty()) {
		tmpStr << m_tempDir << m_curProcessId << "_" << curTime << "_" << m_workerId << "_" <<streamId<< strExt;
	} else {
		logger_err(LOGM_TS_FQ, "Stream file's extension is empty().\n");
	}

	if(ST_MUXER == streamType) {		// Save temp dest files(delete after task done)
		m_tempDstFiles.push_back(tmpStr.str());
	}
	
	return tmpStr.str();
}

void CFileQueue::Enqueue(const CFileQueue::queue_item_t& fileItem)
{
	switch(fileItem.type) {
		case ST_AUDIO:
			m_audioItems.push_back(fileItem);
			break;
		case ST_VIDEO:
			m_videoItems.push_back(fileItem);
			break;
		case ST_SUBTITLE:
			m_subItems.push_back(fileItem);
			break;
		default:
			logger_err(LOGM_TS_FQ, "CFileQueue::Enqueue failed, unrecognise stream type.\n");
			break;
	}
}

bool CFileQueue::ModifyDestFileNames(const char* postfix, int index, const char *prefix)
{
	for (size_t i=0; i<m_destFiles.size(); ++i) {
		std::string dirPath, fileTitle, fileExt;
		if(StrHelper::splitFileName(m_destFiles[i].c_str(),dirPath, fileTitle, fileExt)){
			if (prefix != NULL && *prefix != '\0') {
				fileTitle = prefix;
			} else {
				if(!m_vctDestFileTitle[i].empty()) {
					fileTitle = m_vctDestFileTitle[i];
				} else {
					m_vctDestFileTitle[i] = fileTitle;
				}
			}
			
			if(postfix && *postfix) {
				if(!_stricmp(postfix, "index")) {
					char str_Idx[12] = {0};
					sprintf(str_Idx, "-%d", index);
					fileTitle += str_Idx;
				} else if (!_stricmp(postfix, "time")) {
					time_t t = time(0);
					struct tm *btm = localtime(&t);
					char curTime[24] = {0};
					sprintf(curTime, "-%d%d%d-%d%d%d", btm->tm_year, btm->tm_mon, btm->tm_mday,
						btm->tm_hour, btm->tm_min, btm->tm_sec);
					fileTitle += curTime;
				} else if (strstr(postfix, "%d")) {
					char str_Idx[128] = {0};
					sprintf(str_Idx, postfix, index);
					fileTitle += str_Idx;
				}
			}
			
			m_destFiles[i] = dirPath + fileTitle + fileExt;
		}
	}
	m_destFileIter = m_destFiles.begin();
	return true;
}

bool CFileQueue::ModifyAudioStreamFile(int curIdx)
{
	if(!m_audioItems.empty() && curIdx >= 0 && curIdx < m_vctAudioSegFiles.size()) {
		m_audioItems[0].fileName = m_vctAudioSegFiles[curIdx];
		return true;
	}
	return false;
}

bool CFileQueue::ModifyVideoStreamFile(int curIdx)
{
	if(!m_videoItems.empty() && curIdx >= 0 && curIdx < m_vctVideoSegFiles.size()) {
		m_videoItems[0].fileName = m_vctVideoSegFiles[curIdx];
		return true;
	}
	return false;
}

void CFileQueue::ClearAVSegmentFiles()
{
	if(!m_keepTemp) {
		for(size_t i=0; i<m_vctAudioSegFiles.size(); ++i) {
			const char* tmpFile = m_vctAudioSegFiles[i].c_str();
			if(!RemoveFile(tmpFile)) {
				logger_warn(LOGM_TS_FQ, "Delte temp audio file failed: %s \n", tmpFile);
			}
		}
		for(size_t i=0; i<m_vctVideoSegFiles.size(); ++i) {
			const char* tmpFile = m_vctVideoSegFiles[i].c_str();
			if(!RemoveFile(tmpFile)) {
				logger_warn(LOGM_TS_FQ, "Delte temp audio file failed: %s \n", tmpFile);
			}
		}
	}
	m_vctAudioSegFiles.clear();
	m_vctVideoSegFiles.clear();
}

void CFileQueue::AddSrcFile(const std::string& fileName)
{
	m_srcFiles.push_back(fileName);
}

const char* CFileQueue::GetFirstSrcFile()
{	
	if(!m_srcFiles.empty()) {
		return  m_srcFiles[0].c_str();;
	}
	return NULL;
}

const char* CFileQueue::GetCurSrcFile()
{
	if(m_srcFileIter != m_srcFiles.end()) {
		const char* curSrcFile = (*m_srcFileIter++).c_str();
		StrHelper::getFileExt(curSrcFile, m_curSrcFileExt);
		return curSrcFile;
	}
	return NULL;
}

std::string CFileQueue::GetTempSrcFile(const char* srcFile)
{
	if(!srcFile) return "";
	// No need to get source file to temp folder
#if defined(ENABLE_SINGLE_MODE) || !defined(COPY_TO_LOCAL)
	return srcFile;
#endif

#ifdef COPY_TO_LOCAL
	// If source file is local file
 	if( *(srcFile+1) == ':' || (*srcFile == '/' && *(srcFile+1) != '/')) {	// Local file on Windows or Linux
 		return srcFile;
 	}

	if(m_tempSrcFiles.count(std::string(srcFile)) > 0) {
		return m_tempSrcFiles[std::string(srcFile)];
	}

	m_srcFileIndex++;
	int64_t curTime = GetTickCount();
	std::stringstream tmpStr;
	std::string ext;
	if(StrPro::StrHelper::getFileExt(srcFile, ext)) {
		tmpStr << m_tempDir << curTime << "_src_" << m_srcFileIndex << ext;
		m_tempSrcFiles[std::string(srcFile)] = tmpStr.str();
		CTransmitData transmiter;
		transmiter.Transmit(srcFile, tmpStr.str().c_str(), true);
		transmiter.WaitForCompletion();
		if(!tmpStr.str().empty()) {
			// Copy subtitle files
			std::string srcPath, srcName, ext;
			StrPro::StrHelper::splitFileName(srcFile, srcPath, srcName, ext);
			std::string subFile = srcPath + srcName + ".srt";
			std::string subExt = ".srt";
			if(!FileExist(subFile.c_str())) {
				subFile = srcPath + srcName + ".sub";
				subExt = ".sub";
				if(!FileExist(subFile.c_str())) {
					subFile.clear();
				}
			} 
			if(!subFile.empty()) {
				std::stringstream tmpSubFile;
				tmpSubFile << m_tempDir << curTime << "_sub_" << m_srcFileIndex << subExt;
				m_tempSubFiles.push_back(tmpSubFile.str());
				CopyFile(subFile.c_str(), tmpSubFile.str().c_str(), FALSE);
			}

			return tmpStr.str();
		}
	}
#endif

	return "";
}

void CFileQueue::AddDestFile(const std::string& fileName)
{
	if(fileName.empty()) return;
	std::string curAddFile(fileName);

	bool hasSame = false;
	for (size_t i=0; i<m_destFiles.size(); ++i) {
		if(fileName == m_destFiles.at(i)) {
			hasSame = true;
			break;
		}
	}

	if(hasSame) {	// Rename fileName (multi output file, same file format)
		std::string title, dir, ext;
		bool ret = StrPro::StrHelper::splitFileName(curAddFile.c_str(), dir, title, ext);
		if(ret) {
			curAddFile = dir + title + "_1" + ext;
		}
	}
	m_destFiles.push_back(curAddFile);
	m_vctDestFileTitle.push_back("");
}

const char* CFileQueue::GetCurDestFile()
{
	if(m_destFileIter != m_destFiles.end()) {
		return (*m_destFileIter++).c_str();
	}
	return NULL;
}

CFileQueue::queue_item_t* CFileQueue::GetAudioFileItem(size_t id)
{
	if(id>=0 && id<m_audioItems.size()) {
		return &m_audioItems.at(id);
	}
	return NULL;
}

CFileQueue::queue_item_t* CFileQueue::GetVideoFileItem(size_t id)
{
	if(id>=0 && id<m_videoItems.size()) {
		return &m_videoItems.at(id);
	}
	return NULL;
}

CFileQueue::queue_item_t* CFileQueue::GetFirst(stream_type_t type)
{
	//logger_status(LOGM_TS_FQ, "GetFirst() Video:%d, Audio:%d\n", m_audioItems.size(), m_videoItems.size());
	switch(type) {
		case ST_AUDIO:
			m_audioIter = m_audioItems.begin();
			if(m_audioIter != m_audioItems.end()) {
				return &(*m_audioIter);
			} 
			break;
		case ST_VIDEO:
			m_videoIter = m_videoItems.begin();
			if(m_videoIter != m_videoItems.end()) {
				return &(*m_videoIter);
			} 
			break;
		case ST_SUBTITLE:
			m_subIter = m_subItems.begin();
			if(m_subIter != m_subItems.end()) {
				return &(*m_subIter);
			} 
			break;
		default:
			logger_err(LOGM_TS_FQ, "CFileQueue::GetFirst() failed, unrecognise stream type.\n");
			break;
	}

	//logger_status(LOGM_TS_FQ, "GetFirst() failed, type=%d\n", type);
	return NULL;
}

CFileQueue::queue_item_t* CFileQueue::GetNext(stream_type_t type)
{
	switch(type) {
		case ST_AUDIO:
			m_audioIter++;
			if(m_audioIter != m_audioItems.end()) {
				return &(*m_audioIter);
			} 
			break;
		case ST_VIDEO:
			m_videoIter++;
			if(m_videoIter != m_videoItems.end()) {
				return &(*m_videoIter);
			} 
			break;
		case ST_SUBTITLE:
			m_subIter++;
			if(m_subIter != m_subItems.end()) {
				return &(*m_subIter);
			} 
			break;
		default:
			logger_err(LOGM_TS_FQ, "CFileQueue::GetNext() failed, unrecognise stream type.\n");
			break;
	}
	return NULL;
}

void CFileQueue::ClearAudioFiles()
{
	if(!m_keepTemp) {
		for (size_t i=0; i<m_audioItems.size(); ++i) {
			const char* tmpFile = m_audioItems[i].fileName.c_str();
			if(!RemoveFile(tmpFile)) {
				logger_warn(LOGM_TS_FQ, "Delte temp audio file failed: %s \n", tmpFile);
			}
		}
	}
	m_audioItems.clear();
}

void CFileQueue::ClearVideoFiles()
{
	if(!m_keepTemp) {
		for (size_t i=0; i<m_videoItems.size(); ++i) {
			const char* tmpFile = m_videoItems[i].fileName.c_str();
			if(!RemoveFile(tmpFile)) {
				logger_warn(LOGM_TS_FQ, "Delte temp video file failed: %s \n", tmpFile);
			}
		}
	}

	m_videoItems.clear();
}

void CFileQueue::ClearTempSrcFiles()
{
	//logger_status(LOGM_TS_FQ, "Calling CFileQueue::ClearTempSrcFiles()\n");

	map<string, string>::iterator beginIt = m_tempSrcFiles.begin(), 
		endIt = m_tempSrcFiles.end();

	for(map<string, string>::iterator it = beginIt; it != endIt; ++it) {
		std::string tmpFile = it->second;
		RemoveFile(tmpFile.c_str());
	}
	
	m_tempSrcFiles.clear();
}

void CFileQueue::ClearTempDstFiles()
{
	for (size_t i=0; i<m_tempDstFiles.size(); ++i) {
		const char* tmpFile = m_tempDstFiles[i].c_str();
		RemoveFile(tmpFile);
	}
	m_tempDstFiles.clear();
}

void CFileQueue::ClearEncoderStatFiles()
{
	for (size_t i=0; i<m_encoderStatFiles.size(); ++i) {
		const char* tmpFile = m_encoderStatFiles[i].c_str();
		RemoveFile(tmpFile);
	}
	m_encoderStatFiles.clear();
}

void CFileQueue::Clear()
{
	ClearAudioFiles();
	ClearVideoFiles();
	ClearAVSegmentFiles();
	
	m_subItems.clear();
	ClearDestFiles();
	ClearSrcFiles();
	ClearTempSrcFiles();
	ClearTempDstFiles();
	ClearEncoderStatFiles();

	m_subTitleFile.clear();
	m_vctDestFileTitle.clear();
	m_workerId = 0;
	m_srcFileIndex = 0;
}
