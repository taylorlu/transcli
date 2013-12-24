#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "VideoPreview.h"
#include "logger.h"
#include "util.h"
#include "bit_osdep.h"

#define cimg_use_jpeg
#define cimg_use_png
#include "cimg/CImg.h"

#ifdef min
#undef min
#undef max
#endif
using namespace cimg_library;

CVideoPreview::CVideoPreview(void)
{
}

CVideoPreview::CVideoPreview(int yuvW, int yuvH):m_previewRect(NULL), m_imgFrame(NULL),
m_hThread(NULL), m_yuvW(yuvW), m_yuvH(yuvH)
{
}

bool CVideoPreview::Initialize()
{
	if(!m_yuvW || m_yuvH) {
		logger_err(LOGM_TS_VE, "Yuv frame size not set in CVideoPreview.\n");
		return false;
	}
	m_imgFrame = new CImg<uint8_t>(m_yuvW, m_yuvH);
	m_previewRect = new CImgDisplay(m_yuvW, m_yuvH, "Preview");
	if(!m_imgFrame || !m_previewRect) {
		logger_err(LOGM_TS_VE, "Frame image or CImgDisplay created failed.\n");
		return false;
	}
	return true;
}

bool CVideoPreview::GeneratePipeHandle()
{
	int bufSize = 0;
	if(m_yuvW > 720) {
		bufSize = 3 << 20;
	} else {
		bufSize = m_yuvW*m_yuvH*3;
	}

	int pipeRdWr[2] = {-1};
	int ret = 0;
#ifdef WIN32
	ret = _pipe(pipeRdWr, bufSize, O_BINARY);		
#else
	ret = pipe(pipeRdWr);
#endif

	if(ret == 0) {
		m_fdRead = pipeRdWr[0];
		m_fdWrite = pipeRdWr[1];
		return true;
	} 

	logger_err(LOGM_TS_VE, "CVideoPreview::generatePipeHandle failed.");
	return false;
}

CVideoPreview::~CVideoPreview(void)
{
	delete m_imgFrame;
	delete m_previewRect;
}
