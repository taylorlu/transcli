#include "ImageSrc.h"
#define cimg_use_jpeg
#define cimg_use_png
#include "cimg/CImg.h"
#include "util.h"
#include "logger.h"

CImageSrc::CImageSrc(void):m_yuvFrame(NULL), m_cropMode(0), m_yuvW(0),
	m_yuvH(0), m_yuvDarNum(0), m_yuvDarDen(0), m_fpsNum(0), m_fpsDen(0),
	m_duration(0), m_showCounts(0), m_dynamicIdx(0)
{
}


CImageSrc::~CImageSrc(void)
{
	if(m_yuvFrame) free(m_yuvFrame);
	m_yuvFrame = NULL;
	m_dynamicImages.clear();
}

void CImageSrc::SetStaticImage(const char* imgPath)
{
	//m_showCounts = showCounts;
	m_staticImage = imgPath;
}

void CImageSrc::SetImageFolder(const char* imgFolder)
{
	GetFileListInFolder(imgFolder, m_dynamicImages, ".bmp,.jpg,.png", false);
	//m_showCounts = m_dynamicImages.size();
}

void CImageSrc::SetCropMode(int cropMode)
{
	m_cropMode = cropMode;
}

void CImageSrc::SetDestYuvSize(int width, int height)
{
	m_yuvW = width;
	m_yuvH = height;
}

void CImageSrc::SetDestDar(int darNum, int darDen)
{
	m_yuvDarNum = darNum;
	m_yuvDarDen = darDen;
}

void CImageSrc::SetDestFps(int fpsNum, int fpsDen)
{
	m_fpsNum = fpsNum;
	m_fpsDen = fpsDen;
}

void CImageSrc::getCropParam(int& x0, int& y0, int& x1, int& y1, int imgW, int imgH)
{
	float srcDar = (float)imgW/imgH;
	float dstDar = (float)m_yuvDarNum/m_yuvDarDen;
	bool bXChange = true;			// Crop or expand along x axis (false:along y axis)
	if(srcDar - dstDar > 0.001) {	// src dar > dst dar
		if(m_cropMode == 0) {	// Crop
			bXChange = true;
		} else {				// Expand
			bXChange = false;
		}
	} else if (srcDar - dstDar < -0.001) {	// src dar < dst dar
		if(m_cropMode == 0) {	// Crop
			bXChange = false;
		} else {				// Expand
			bXChange = true;
		}
	}

	// In CImg, if x0 or y0 < 0, it's expanding, if x0 > 0, it's cropping
	if(bXChange) {
		int calcW = (int)(imgH*dstDar);
		x0 = (imgW - calcW)/2;
		x1 = imgW - 1 - x0;
		y0 = 0;
		y1 = imgH - 1;
	} else {
		int calcH = (int)(imgW/dstDar);
		x0 = 0;
		x1 = imgW - 1;
		y0 = (imgH - calcH)/2;
		y1 = imgH - 1 - y0;
	}
}

static void RGB2YV12(uint8_t* pYv12, cimg_library::CImg<uint8_t>& img)
{
	int w = img.width();
	int h = img.height();
	uint8_t* pUplan = pYv12 + w*h;
	uint8_t* pVplan = pUplan + w*h/4;
	const uint8_t* pr = img.data(0, 0, 0, 0);
	const uint8_t* pg = img.data(0, 0, 0, 1);
	const uint8_t* pb = img.data(0, 0, 0, 2);
	for(int y = 0; y<h; ++y) {
		for(int x = 0; x<w; ++x) {
			int r = *pr++;
			int g = *pg++;
			int b = *pb++;
			int idensity = ((66*r + 129*g + 25*b + 128) >> 8) + 16;
			*pYv12++ = idensity;
			if(y%2 == 0) {		// U plan
				if(x%2 == 0) {
					uint8_t u = (uint8_t)(((112*b - 74*g - 38*r + 128) >> 8) + 128);
					int uIndex = y/2*w/2 + x/2;
					pUplan[uIndex] = u;
				}
			} else {
				if(x%2 != 0) {
					uint8_t v = (uint8_t)(((112*r - 94*g - 18*b + 128) >> 8) + 128);
					int vIndex = y/2*w/2 + x/2;
					pVplan[vIndex] = v;
				}
			}
		}
	}
}

bool CImageSrc::loadAndConvertImage(const char* imagePath)
{
	try {
		cimg_library::CImg<uint8_t> img(imagePath);
		if(img.spectrum() < 3) {
			logger_err(LOGM_TS_VE, "Image color channels less than 3 in CImageSrc::Initialize().\n");
			return false;
		}

		int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
		if(m_yuvDarDen > 0 && m_yuvDarNum > 0) {
			getCropParam(x0, y0, x1, y1, img.width(), img.height());
		}
		if(x0 != 0 || y0 != 0) {	// Crop or Expand
			img.crop(x0, y0, x1, y1, true);
		}
		img.resize(m_yuvW, m_yuvH, -100, -100, 5);
		RGB2YV12(m_yuvFrame, img);
	} catch (cimg_library::CImgIOException& e) {
		logger_err(LOGM_TS_VE, "%s!\n", e.what());
		return false;
	}
	return true;
}

bool CImageSrc::Initialize()
{
	if(m_yuvW <= 0 || m_yuvH <= 0) return false;
	m_yuvFrame = (uint8_t*) malloc(m_yuvW*m_yuvH*3/2);
	if(!m_yuvFrame) return false;

	if(!m_staticImage.empty()) {	// Static image mode
		if(m_fpsDen > 0) {
			m_showCounts = m_duration*m_fpsNum/m_fpsDen;
		}
		return loadAndConvertImage(m_staticImage.c_str());
	} 

	return true;
}

void CImageSrc::Reset()
{
	if(!m_staticImage.empty()) {
		if(m_fpsDen > 0) {
			m_showCounts = m_duration*m_fpsNum/m_fpsDen;
		}
	} else {
		m_dynamicIdx = 0;
	}
}

uint8_t* CImageSrc::GetNextFrame()
{
	if(!m_staticImage.empty()) {	// Static image mode
		if(m_showCounts > 0) {
			m_showCounts--;
			return m_yuvFrame;
		} 
	} else {
		if(m_dynamicIdx < m_dynamicImages.size()) {
			if(loadAndConvertImage(m_dynamicImages[m_dynamicIdx].c_str())) {
				return m_yuvFrame;
			}
			m_dynamicIdx++;
		}
	}
	return NULL;
}