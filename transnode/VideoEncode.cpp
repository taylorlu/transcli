#include "VideoEncode.h"
#include "StreamOutput.h"
#include "vfhead.h"
#include "MEvaluater.h"
#include "WaterMarkFilter.h"
#include "ThumbnailFilter.h"
#include "processwrapper.h"
#include "util.h"
#include "videoEnhancer.h"
#include "bitconfig.h"

CVideoEncoder::CVideoEncoder(void):m_pOutStream(NULL),
								m_fdRead(-1), m_fdWrite(-1), m_tid(0),
								m_frameCount(0), m_yuvFrameSize(0), m_frameSizeAfterVf(0),
								m_pVideoFilter(NULL), m_logModuleType(LOGM_TS_VE), m_pXmlPrefs(NULL),
								m_pWaterMarkMan(NULL), m_pThumbnail(NULL), m_bClosed(1),
								m_encodePass(1),m_bMultiPass(false)
{
#ifdef RAW_VIDEO_DUMP
	m_rawDumpStream = NULL;
#endif
	memset(&m_vInfo, 0, sizeof(m_vInfo));
	m_pPassLogFile = NULL;
#ifdef HAVE_VIDEO_ENHANCE
	m_pVideoEnhancer = NULL;
#endif
}

CVideoEncoder::CVideoEncoder(const char* outFileName):m_pOutStream(NULL),
								m_fdRead(-1), m_fdWrite(-1), m_tid(0),
								m_frameCount(0), m_yuvFrameSize(0), m_frameSizeAfterVf(0),
								m_pVideoFilter(NULL), m_logModuleType(LOGM_TS_VE), m_pXmlPrefs(NULL),
								m_pWaterMarkMan(NULL), m_pThumbnail(NULL),
								m_encodePass(1),m_bMultiPass(false)
{
	m_strOutFile = outFileName;
#ifdef RAW_VIDEO_DUMP
	m_rawDumpStream = NULL;
#endif
	memset(&m_vInfo, 0, sizeof(m_vInfo));
	m_pPassLogFile = NULL;
#ifdef HAVE_VIDEO_ENHANCE
    m_pVideoEnhancer = NULL;
#endif
}

CVideoEncoder::~CVideoEncoder(void)
{
	// Write handle is closed in trans worker
	CloseReadHandle();
	CloseWriteHandle();

	if(m_pOutStream) {
		destroyStreamOutput(m_pOutStream);
		m_pOutStream = NULL;
	}

	if (m_pWaterMarkMan) {
		delete m_pWaterMarkMan;
	}
	if(m_pThumbnail) {
		delete m_pThumbnail;
	}

	if(m_pPassLogFile) {
		free(m_pPassLogFile);
	}

#ifdef RAW_VIDEO_DUMP
	if(m_rawDumpStream) {
		destroyStreamOutput(m_rawDumpStream);
	}
#endif
}

void CVideoEncoder::SetPassLogFile(const std::string& logFile)
{
	if(m_pPassLogFile) {
		free(m_pPassLogFile);
	}
	m_pPassLogFile = _strdup(logFile.c_str());
}

bool CVideoEncoder::InitWaterMark()
{
#ifdef PRODUCT_MEDIACODERDEVICE		// Disable watermark under the product
	return true;
#endif

	if(!m_pXmlPrefs) {
		logger_err(LOGM_TS_VE, "Video pref not set in video encoder.\n");
		return false;
	}
	m_bClosed = 0;
	// Initialize frame size
	m_frameSizeAfterVf = m_vInfo.res_out.width*m_vInfo.res_out.height*m_vInfo.bits/8;
	if(m_pVideoFilter) {	// Need scale by built-in video filter
		m_yuvFrameSize = m_vInfo.res_in.width*m_vInfo.res_in.height*m_vInfo.bits/8;
	} else {
		m_yuvFrameSize = m_frameSizeAfterVf;
	}

#ifdef HAVE_VIDEO_ENHANCE
	// Initialize video enhancer
	if(m_pXmlPrefs->GetInt("videofilter.eq.mode") == 1) { //Intelligence enhance
	#ifdef HAVE_ALVA_VIDEO_OPT
		vidopt2_init();
		FILE* fp = fopen("alva.txt", "rt");
		if(fp) {
			fscanf(fp, "%d %d", &alvaIHD, &alvaICS);
			fclose(fp);
		}
		if(alvaIHD == -1) alvaIHD = 4;
		if(alvaICS == -1) alvaICS = 4;
	#else
		float colorLevel = m_pXmlPrefs->GetFloat("videofilter.eq.colorlevel");
		float sharpness = m_pXmlPrefs->GetFloat("videofilter.eq.sharpness");
		int contrastThreshold = m_pXmlPrefs->GetInt("videofilter.eq.contrastThreshold");
		float contrastLevel = m_pXmlPrefs->GetFloat("videofilter.eq.contrastLevel");
		m_pVideoEnhancer = new CVideoEnhancer;
		if(m_pVideoEnhancer) {
			m_pVideoEnhancer->init(m_vInfo.res_out.width, m_vInfo.res_out.height, m_vInfo.res_out.width, 0);
			m_pVideoEnhancer->setColorLevel(colorLevel);
			m_pVideoEnhancer->setSharpenLevel(sharpness);
			m_pVideoEnhancer->setContrastThreshlod(contrastThreshold);
			m_pVideoEnhancer->setContrastLevel(contrastLevel);
		}
	#endif
	}
#endif

	if(m_encodePass == 2 && m_bMultiPass) return true;
	
	bool ret = parseWaterMarkInfo(m_pWaterMarkMan, m_pXmlPrefs, &m_vInfo);
	ret = parseThumbnailInfo(m_pThumbnail,m_pXmlPrefs,&m_vInfo,m_destFileName);
	return ret;
}

bool CVideoEncoder::GeneratePipeHandle()
{
	int bufSize = m_vInfo.res_out.width*m_vInfo.res_out.height*6;
	/*if(m_vInfo.res_out.width > 720) {
		bufSize = 4 << 20;
	} else {
		bufSize 
	}*/

	if (CProcessWrapper::MakePipe(m_fdRead, m_fdWrite, bufSize, false, false)) {
		return true;
	}
	logger_err(LOGM_TS_VE, "CVideoEncoder::generatePipeHandle failed.");
	return false;
}

void CVideoEncoder::SetVideoInfo(const video_info_t& vInfo, CXMLPref* pXmlPrefs)
{
	if(m_pXmlPrefs == NULL) {
		m_vInfo = vInfo;
		m_pXmlPrefs = pXmlPrefs;
	}
}

uint8_t* CVideoEncoder::FilterFrame(uint8_t* pOrigBuf, int origBufSize)
{
	uint8_t *processedData = NULL;

	if(pOrigBuf == NULL) return NULL;

	if(m_pVideoFilter && (m_vInfo.res_out.width != m_vInfo.res_in.width ||
		m_vInfo.res_out.height != m_vInfo.res_in.height)) {
		// Do video filter
		m_pVideoFilter->Fill(0, 0, pOrigBuf);
		m_pVideoFilter->Process(0, 0, 0);
		processedData = (uint8_t*)m_pVideoFilter->GetOutputBufferQuick();
		if (processedData == NULL) {
			//skiled or failed
        	return NULL;
    	}
	} else {
		processedData = pOrigBuf;
	}

#ifdef HAVE_VIDEO_ENHANCE
    #ifdef HAVE_ALVA_VIDEO_OPT
	if(vidopt2_isInit()) {
		if(!pInBuf) {
			pInBuf = (uint8_t*)malloc(origBufSize);
		}
		memcpy(pInBuf, processedData, origBufSize);
		int planarSize = m_vInfo.res_out.width*m_vInfo.res_out.height;
		VIDOPT_PLANARS inPlanar, outPlanar;
		inPlanar.planars[0].data = pInBuf;
		outPlanar.planars[0].data = processedData;
		outPlanar.planars[0].width = inPlanar.planars[0].width = m_vInfo.res_out.width;
		outPlanar.planars[0].height = inPlanar.planars[0].height = m_vInfo.res_out.height;
		outPlanar.planars[0].stride = inPlanar.planars[0].stride = m_vInfo.res_out.width;
		outPlanar.planars[1].data = processedData+planarSize;
		inPlanar.planars[1].data = pInBuf+planarSize;
		outPlanar.planars[1].width = inPlanar.planars[1].width = m_vInfo.res_out.width/2;
		outPlanar.planars[1].height = inPlanar.planars[1].height = m_vInfo.res_out.height/2;
		outPlanar.planars[1].stride = inPlanar.planars[1].stride = m_vInfo.res_out.width/2;
		outPlanar.planars[2].data = processedData+planarSize*5/4;
		inPlanar.planars[2].data = pInBuf+planarSize*5/4;
		outPlanar.planars[2].width = inPlanar.planars[2].width = m_vInfo.res_out.width/2;
		outPlanar.planars[2].height = inPlanar.planars[2].height = m_vInfo.res_out.height/2;
		outPlanar.planars[2].stride = inPlanar.planars[2].stride = m_vInfo.res_out.width/2;
		outPlanar.planars[3].data = inPlanar.planars[3].data = NULL;
		outPlanar.planars[3].width = inPlanar.planars[3].width = 0;
		outPlanar.planars[3].height = inPlanar.planars[3].height = 0;
		outPlanar.planars[3].stride = inPlanar.planars[3].stride = 0;
		vidopt2_yv12(&inPlanar, &outPlanar, alvaIHD, alvaICS);
	}
    #else
	if(m_pVideoEnhancer) {
		m_pVideoEnhancer->process(processedData);
	}
    #endif
#endif

	if(m_pWaterMarkMan && m_encodePass == 1) {
		m_pWaterMarkMan->RenderWaterMark(processedData);
	}

	if(m_pThumbnail && m_encodePass == 1) {
		m_pThumbnail->GenerateThumbnail(processedData);
	}

	return processedData;
}

bool CVideoEncoder::CloseWriteHandle()
{
	if(m_fdWrite >= 0){
		//_commit(m_fdWrite);
		_close(m_fdWrite);
		m_fdWrite=-1;
		return true;
	}
	return false;
}

bool CVideoEncoder::CloseReadHandle()
{
	if(m_fdRead >= 0){
		_close(m_fdRead);
		m_fdRead=-1;
		return true;
	}
	return false;
}

bool CVideoEncoder::StopThumbnail()
{
	if(m_pThumbnail && m_encodePass == 1) {
		return m_pThumbnail->StopThumbnail();
	}
	return true;
}

bool CVideoEncoder::Stop()
{
	if(m_pOutStream) {
		destroyStreamOutput(m_pOutStream);
		m_pOutStream = NULL;
	}
#ifdef HAVE_VIDEO_ENHANCE
	if(m_pVideoEnhancer) {
		m_pVideoEnhancer->fini();
		delete m_pVideoEnhancer;
		m_pVideoEnhancer = NULL;
	}
    #ifdef HAVE_ALVA_VIDEO_OPT
	if(vidopt2_isInit()) {
		vidopt2_uint();
	}
	if(pInBuf) {
		free(pInBuf);
		pInBuf = NULL;
	}
    #endif
#endif

	m_bClosed = 1;
	if(m_encodePass > 1) m_encodePass--;
	return true;
}
