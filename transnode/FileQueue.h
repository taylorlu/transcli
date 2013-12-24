#ifndef _FILE_QUEUE_H_
#define _FILE_QUEUE_H_
#include <string>
#include <vector>
#include <map>

#include "bit_osdep.h"
#include "tsdef.h"

using namespace std;

class CFileQueue
{
public:
	CFileQueue(void);
	~CFileQueue(void);

	typedef struct {
		std::string fileName;
		stream_type_t type;
		video_info_t* vinfo;
		audio_info_t* ainfo;
	} queue_item_t;

	void SetWorkerId(int id) {m_workerId = id;}
	bool SetTempDir(const char* dir) ;
	const char* GetTempDir() {return m_tempDir.c_str();}
	void CleanTempDir();
	void AddSrcFile(const std::string& fileName);
	const char* GetFirstSrcFile();
	const char* GetCurSrcFile();
	std::string GetCurSrcFileExt(){return m_curSrcFileExt;}
	std::string GetTempSrcFile(const char* srcFile);

	size_t GetSrcFileCount() {return m_srcFiles.size();}
	bool IsAllSrcFilesConsumed() {return m_srcFileIter == m_srcFiles.end();}
	void RewindSrcFiles() {m_srcFileIter = m_srcFiles.begin();}

	void AddDestFile(const std::string& fileName);
	const char* GetCurDestFile();
	void RewindDestFiles() {m_destFileIter = m_destFiles.begin();};
	bool ModifyDestFileNames(const char* postfix, int index = 0, const char *prefix = NULL);

	std::vector<std::string> GetDestFiles() {return m_destFiles;}
	std::vector<std::string> GetTempDestFiles() {return m_tempDstFiles;}
	void SetSubTitleFile(const char* subFile) {m_subTitleFile = subFile;}
	const char* GetSubTitleFile() {return m_subTitleFile.c_str();}
	
	// Get temp stream file name
	std::string   GetStreamFileName(stream_type_t streamType, int format=-1, int streamId=0, const char* ext=NULL);
	size_t GetAudioItemCount() {return m_audioItems.size();}
	size_t GetVideoItemCount() {return m_videoItems.size();}
	queue_item_t* GetAudioFileItem(size_t id);
	queue_item_t* GetVideoFileItem(size_t id);
	std::string GetEncoderStatFile(int videoStreamId);
	void RenameX264StatFile();
	void Enqueue(const queue_item_t& fileItem);
	queue_item_t* GetFirst(stream_type_t type);
	queue_item_t* GetNext(stream_type_t type);

	void SetRelativeDir(const char* relativeDir) {if(relativeDir) m_relativeDir = relativeDir;}
	const char* GetRelativeDir() {return m_relativeDir.c_str();}

	void AddAudioSegFile(std::string audioSegFile) {m_vctAudioSegFiles.push_back(audioSegFile);}
	void AddVideoSegFile(std::string videoSegFile) {m_vctVideoSegFiles.push_back(videoSegFile);}
	bool ModifyAudioStreamFile(int curIdx);
	bool ModifyVideoStreamFile(int curIdx);

	void RemoveCurSplitTempFile();
	void SetKeepTemp(bool bKeep) {m_keepTemp = bKeep;}
	bool GetKeepTemp() {return m_keepTemp;}

	void ClearAudioFiles();
	void ClearVideoFiles();
	void ClearDestFiles() {m_destFiles.clear();}
	void ClearSrcFiles() {m_srcFiles.clear();}
	void ClearTempSrcFiles();
	void ClearTempDstFiles();
	void ClearEncoderStatFiles();
	void ClearAVSegmentFiles();
	void Clear();

private:

	std::vector<queue_item_t> m_audioItems;
	std::vector<queue_item_t> m_videoItems;
	std::vector<queue_item_t> m_subItems;
	std::vector<queue_item_t>::iterator m_audioIter;
	std::vector<queue_item_t>::iterator m_videoIter;
	std::vector<queue_item_t>::iterator m_subIter;

	std::vector<std::string> m_destFiles;
	std::vector<std::string>::iterator m_destFileIter;

	std::vector<std::string> m_srcFiles;
	std::vector<std::string>::iterator m_srcFileIter;
	std::string m_curSrcFileExt;

	std::map<std::string, std::string> m_tempSrcFiles;	// Temp source file on local node(in temp dir)
	std::vector<std::string> m_tempDstFiles;	// Temp dest file on local node(in temp dir)
	std::vector<std::string> m_tempSubFiles;
	std::vector<std::string> m_encoderStatFiles;

	std::string m_tempDir;
	std::string m_subTitleFile;
	
	std::string m_relativeDir;		// When keep directory structure of source watch folder, use this

	// Used for modifying segment files
	std::vector<std::string> m_vctDestFileTitle;	

	// Segment audio/video stream files
	std::vector<std::string> m_vctAudioSegFiles;
	std::vector<std::string> m_vctVideoSegFiles;

	int         m_workerId;
	int			m_srcFileIndex;
	int			m_curProcessId;
	bool        m_keepTemp;
};

#endif
