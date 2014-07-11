#ifndef _VIDEO_ENCODE_H_
#define _VIDEO_ENCODE_H_

#include "encoderCommon.h"
#include "bitconfig.h"
//#define RAW_VIDEO_DUMP
class CVideoEnhancer;

class CVideoEncoder
{
public:
	enum EncodingResult {
		VIDEO_ENC_END = -2,
		VIDEO_ENC_ERROR = -1,
		VIDEO_ENC_CONTINUE = 0
	};

	CVideoEncoder(const char* outFileName);
	virtual ~CVideoEncoder(void);
	virtual bool GeneratePipeHandle();
	void		 SetVideoInfo(const video_info_t& vInfo, CXMLPref* pXmlPrefs);
	virtual	bool Initialize() = 0;
	virtual	int  EncodeFrame(uint8_t *pFrameBuf, int bufSize, bool bLast=false) = 0;	// If success return frameCount, else return -1;
	virtual bool Stop();
	virtual bool IsRunning() {return true;}
	virtual int	 GetDelayedFrames() {return 0;}

	void SetOutFileName(const char* outFile) {if(outFile) m_strOutFile = outFile;}
	std::string GetOutFileName() {return m_strOutFile;}
	int		GetBitrate() {return m_vInfo.bitrate;}		// Unit: kbit

	// Watermark and Thumbnail
	void SetDestFileName(const char* videoDestFile) {if(videoDestFile) m_destFileName=videoDestFile;}  // Used for thumbnail filename
	bool InitWaterMark();
	bool StopThumbnail();

	video_encoder_t GetEncoderType() {return m_vInfo.encoder_type;}
	video_info_t*   GetVideoInfo() {return &m_vInfo;}
	CXMLPref*		GetVideoPref() {return m_pXmlPrefs;}
	int				GetFrameSize() {return m_yuvFrameSize;}

	int  GetWirteHandle() {return m_fdWrite;}
	int  GetReadHandle() {return m_fdRead;}
	bool CloseWriteHandle();
	bool CloseReadHandle();

	CStreamOutput* GetOutputStream() {return m_pOutStream;}
	int  GetLogModuleType() {return m_logModuleType;}

	void SetVideoFilterType(vf_type_t vfType) {m_vInfo.vfType = vfType;}
	vf_type_t GetVideoFilterType() const { return m_vInfo.vfType;}
	void SetVideoFilter(CVideoFilter* pFilter) {m_pVideoFilter = pFilter;}	
	CVideoFilter* GetVideoFilter() {return m_pVideoFilter;}
	void SetWorkerId(int id) {m_tid = id;}
	
	uint8_t *FilterFrame(uint8_t* pOrigBuf, int origBufSize);

	bool IsStopped() {return m_bClosed == 1;}
	//if the encoder is not a CLI app, return -1
	virtual int GetCLIPid() {return -1;}
	void SetEncodePass(int passNum){m_encodePass = passNum; m_bMultiPass = passNum > 1;}
	void SetPassLogFile(const std::string& logFile);
	bool GetIsMultiPass() {return m_bMultiPass;}

protected:
	CVideoEncoder(void);

#ifdef RAW_VIDEO_DUMP
	CStreamOutput* m_rawDumpStream;
#endif

	CStreamOutput*	m_pOutStream;
	std::string		m_strOutFile;
	std::string		m_destFileName;
	int				m_fdRead;
	int				m_fdWrite;
	int				m_tid;
	int				m_frameCount;
	int				m_yuvFrameSize;
	int				m_frameSizeAfterVf;
	
	CVideoFilter*	m_pVideoFilter;
	
	//int			m_state;
	int				m_logModuleType;
	video_info_t    m_vInfo;
	CXMLPref*		m_pXmlPrefs;

	// Water mark and snapshot
	CWaterMarkManager* m_pWaterMarkMan;
	CThumbnailFilter* m_pThumbnail;
	CThumbnailFilter* m_pThumbnail1;
	int				  m_bClosed;
	
	int				  m_encodePass;
	bool			  m_bMultiPass;
	char*             m_pPassLogFile;

#ifdef HAVE_VIDEO_ENHANCE
	CVideoEnhancer*   m_pVideoEnhancer;
#endif
};

#endif


