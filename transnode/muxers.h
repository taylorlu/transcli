#ifndef _MUXER_H_
#define _MUXER_H_

#include <string>
#include <vector>
#include "FileQueue.h"
#include "BiThread.h"

class CXMLPref;

enum mux_error_t {
	MUX_ERR_SUCCESS,
	MUX_ERR_INVALID_FILE_QUEUE,
	MUX_ERR_INVALID_FILE_PATH,
	MUX_ERR_NOT_ENOUGH_SPACE,
	MUX_ERR_INVALID_FILE,
	MUX_ERR_MUXER_FAIL
};

class CMuxer
{
public:
	virtual ~CMuxer() {}
	bool SetPref(CXMLPref* pref);
	bool SetFileQueue(CFileQueue* pFileQueue) {m_pFileQueue = pFileQueue; return true;}

	virtual int Mux() {return MUX_ERR_SUCCESS;}
	virtual bool Remux() { return true; }
	virtual const char* GetOutExt(bool fAudioOnly = false) { return "out"; }
	
	CXMLPref* GetPref() {return m_pref;}
	std::string GenDestFileName(const char* srcFileName, size_t destIndex=0, const char* srcDir=NULL);	// Generate dest file name according to src file and naming rules 

	void AddAudioStream(int audioIdx);		// Add new audio stream index (multi audio track, create new audio)
	bool ContainStream(int streamType, size_t index);	// Test whether the stream is in muxer
	float GetMuxInfoSizeRatio() {return m_muxInfoSizeRatio;}

	int GetMuxerType() {return m_muxerType;}
	void SetMuxerType(int type) { m_muxerType = type;}

	//void SetUseDMGMuxMp4(bool bUseDMG) {m_bUsingDMGMp4Mux = bUseDMG;}
protected:
	CMuxer() {
		m_muxInfoSizeRatio = 0.f; 
		m_pFileQueue = NULL;
		// Defualt not use DMG, only when audio format is eac3, use DMG mux mp4
		//m_bUsingDMGMp4Mux = false;
	}

	bool checkAvailableSpace(const char* filePath);
	//static THREAD_RET_T WINAPI muxThread(void *muxer);

	CXMLPref* m_pref;
	CFileQueue* m_pFileQueue;
	std::vector<int> m_audioIndexs;		// audio stream indexs that the muxer contain
	std::vector<int> m_videoIndexs;		// video stream indexs that the muxer contain
	float m_muxInfoSizeRatio;
	int m_muxerType;
	bool m_bUsingDMGMp4Mux;
	//CBIThread::Handle m_hThread;
};

class CMuxerFactory
{
public:
	static CMuxer* CreateInstance(int type);
};

bool validateMp4File(const char* mp4file);
#endif
