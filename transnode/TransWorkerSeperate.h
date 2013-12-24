#ifndef _TRANS_WORKER_SEPERATE_H_
#define _TRANS_WORKER_SEPERATE_H_

#include "TransWorker.h"

class CAudioEncoder;
class CVideoEncoder;
class CMuxer;
class CMediaSplitter;
class CImageSrc;

class CTransWorkerSeperate : public CTransWorker
{
	friend class CMediaSplitter;
public:
	CTransWorkerSeperate();
	~CTransWorkerSeperate(void);
	
	void CleanUp();
	//void ResetStateAndParams();

	//parse XML_RPC request
	bool ParseSetting();
	int  StartWork();
	bool Stop();
	//video_encoder_t GetVideoEncoder();
private:
	bool parseWatchfoderConfig(CXMLPref* prefs);
	bool parsePlaylistConfig(CXMLPref* prefs);
	bool parseClipConfig(CXMLPref* prefs);
	bool parseSegmentConfig(CXMLPref* prefs);
	bool parseImageTailConfig(CXMLPref* prefs);

	// Set audio/video/muxer preset
	bool setAudioPref(CXMLPref* prefs);
	bool setVideoPref(CXMLPref* prefs);
	bool setMuxerPref(CXMLPref* prefs);

	void initAudioFractionOfSecond();
	bool initialize();
	bool initResourceInOnePassOrLastPass(const char* srcFile);
	// Set dest file name for video encoder, used in thumbnail generation.
	void setDestNameForVideo(CVideoEncoder* pVideoEnc, size_t encIdx);

	// Decide to use decoder or libresample to do audio filter
	void decideAfType() ;
	// Set audio/video info of source file(get from mediainfo)
	bool setSourceAVInfo(StrPro::CXML2* mediaInfo);
	
	bool initAudioEncoders();
	bool initVideoEncoders();
	bool initAVSrcAttrib(StrPro::CXML2* mediaInfo);
	bool initSrcAudioAttrib(StrPro::CXML2* mediaInfo);
	bool initSrcVideoAttrib(StrPro::CXML2* mediaInfo);
	bool initSrcGeneralAttrib(StrPro::CXML2* mediaInfo);
	bool initSrcSubtitleAttrib(StrPro::CXML2* mediaInfo, CXMLPref* pVideoPref);
	bool setDecoderParam(video_info_t* pVInfo, CXMLPref* pVideoPref, 
		audio_info_t* pAInfo, CXMLPref* pAduioPref);
	bool startDecoder(const char* srcFileName);
	bool startAuxDecoders(const char* srcFileName);
	bool decodeNext();

	CAudioEncoder* createAudioEncoder(audio_encoder_t encType, audio_format_t aencFormat);
	CVideoEncoder* createVideoEncoder(video_encoder_t encType, video_format_t vencFormat,
		vf_type_t vfType);

	CDecoder* createAudioDecoder(audio_decoder_t decType);
	CDecoder* createVideoDecoder(video_decoder_t decType);

	bool createVideoFilter(CVideoEncoder* pVideoEncode);
	bool initVideoFilter(CVideoEncoder* pVideoEncode);
	void uninitVideoFilter(CVideoEncoder* pVideoEncode);
	void deleteDecoders();

	static THREAD_RET_T WINAPI mainThread(void* transWorker);
	THREAD_RET_T mainFunc();

	THREAD_RET_T transcodeAudio();
	// Various type of audio transcode combination
	THREAD_RET_T transcodeMbrAudio();
	THREAD_RET_T transcodeSingleAudio();
	THREAD_RET_T transcodeSingleAudioComplex();

	// pass 1 video encoding(2-pass mode) and audio replay gain analyse
	bool firstPassAnalyse();
	bool initPass2();
	THREAD_RET_T analyseMainAudioTrack();
	bool createAuxAudioAnalyseThread(std::vector<CBIThread::Handle>& threadHandles);
	// Entry for replay analyse of aux audio
	static THREAD_RET_T WINAPI auxAudioAnalyseEntry(void* decoderParam);

	THREAD_RET_T transcodeVideo();
	// Various type of video transcode combination 
	THREAD_RET_T transcodeMbrVideo();
	THREAD_RET_T transcodeSingleVideo();
	THREAD_RET_T transcodeSingleVideoComplex(); // Including split and join

	// Thread for encoding one audio stream when MBR
	static THREAD_RET_T WINAPI encodeAudioThread(void* encodeParam);	
	THREAD_RET_T encodeAudio(size_t audioIdx);

	// Thread for encoding one video stream when MBR
	static THREAD_RET_T WINAPI encodeVideoThread(void* encodeParam);	
	THREAD_RET_T encodeVideo(size_t videoIdx);

	void closeDecoders();
	
	void waitForCopyStreams();
	void cleanAudioEncoders();
	void cleanVideoEncoders();
	void cleanMuxers();

	// If there is no video in source file, only generate audio file
	bool resetMuxersForMusic();		// If there is no video in source file

	bool doMux();			// Do muxing, generate target files

	void autoSelectBestDecoder(const char* srcFile, CXMLPref* pVideoPref, CXMLPref* pAudioPref);
	// New decoder selection strategy(Mainly use ffmpeg 2012-09-18)
	void autoSelectBestDecoder2(const char* srcFile, CXMLPref* pVideoPref, CXMLPref* pAudioPref);
	void readExtraClipConfig(const char* clipFile, std::vector<int>& clipStarts, 
		std::vector<int>& clipEnds);
	
	bool processStreamCopy();
	bool copyStream(const char* srcFile, bool isAudio);

	// Add image tail to video stream(Static:single image repeat multi times, Dynmaic:image sequence)
	bool addImageTailToVideoStream(CVideoEncoder* pEncoder);

	// Add blank audio sample to the end, to avoid big difference of a/v duration
	bool appendBlankAudio(CAudioEncoder* pEncoder);

	// Add black video frames to the end, to avoid big difference of a/v duration
	bool appendBlankVideo(CVideoEncoder* pEncoder);

	//Benchmark related functions
	void benchmark();
	
private:
	struct multi_thread_param_t {
		CTransWorkerSeperate* worker;
		size_t		  curStreamId;
	};

	std::vector<CAudioEncoder*>     m_audioEncs;			// audio encoders
	std::vector<CVideoEncoder*>		m_videoEncs;			// video encoders
	std::vector<CMuxer*>			m_muxers;

	CDecoder*			   m_pAudioDec;
	CDecoder*			   m_pVideoDec;
	video_decoder_t		   m_videoDecType;
	audio_decoder_t        m_audioDecType;
	audio_info_t*		   m_pCopyAudioInfo;
	video_info_t*          m_pCopyVideoInfo;

	std::vector<CDecoder*> m_auxDecoders;		// Auxiliary decoders (decode other audio tracks)
	std::vector<CXMLPref*> m_auxAudioPrefs;		// Auxiliary audio prefs

	CImageSrc*			m_pImgTail;		// Only support one video stream

	CMediaSplitter*		m_pSplitter;
	int					m_muxingCount;		// The count of muxing (When split media)

	volatile int	    m_lockReadAudio;	// Check whether lock reading from decoder
	volatile int	    m_lockReadVideo;	// Check whether lock reading from decoder
	bool				m_bDecodeNext;		// 是否顺序解码几个视频文件

	int  m_encoderPass;
	bool m_bCopyAudio;
	bool m_bCopyVideo;
	bool m_bMultiAudioTrack;

	std::string m_playlistKey;
};

#endif
