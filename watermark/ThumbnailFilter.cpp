#include "ThumbnailFilter.h"
#include "logger.h"
#include "util.h"

#define cimg_use_jpeg
#define cimg_use_png
#define cimg_display 0
#include "cimg/CImg.h"

#ifdef WIN32
#include <direct.h>
#endif

#include "bit_osdep.h"

#ifdef min
#undef min
#undef max
#endif

using namespace cimg_library;

CThumbnailFilter::CThumbnailFilter(void): m_thumbType(THUMB_BMP),
m_yuvW(0), m_yuvH(0), m_thumbW(0), m_thumbH(0), m_thumbCount(1), m_thumbIndex(1),
m_thumbInterval(0), m_startTime(5), m_endTime(0), m_frameCount(0), m_fps(0.f),
m_duration(0), m_bStitching(false), m_stitchAlign(0), m_gridRow(0), m_gridCol(0),
m_enablePackImage(false), m_bOptimizeImage(false), m_imageQuality(100), m_cropMode(0), 
m_bEnableMultiSize(false), m_dar(1.f), m_sizePostfix("_%dx%d")
{
}

CThumbnailFilter::~CThumbnailFilter(void)
{
	
}

void CThumbnailFilter::SetFolder(const char* folderPath)
{
	if(folderPath && strlen(folderPath) > 0) {
		m_folder = folderPath;
		if(m_folder.empty()) return;
		if(*(m_folder.end()-1) == '\\' || *(m_folder.end()-1) == '/') {
			m_folder.erase(m_folder.end()-1);
		}
		_mkdir(m_folder.c_str());	// Ensure the folder exists
	}
}

bool CThumbnailFilter::CalculateCapturePoint()
{
	if(m_thumbCount < 1) return false;

	// Check thumbnail size
	if(m_thumbW > 0 && m_thumbH <= 0) {
		if(m_dar > 0) {
			m_thumbH = (int)(m_thumbW/m_dar);
			EnsureMultipleOfDivisor(m_thumbH, 2);
		}
	} else if(m_thumbH > 0 && m_thumbW <= 0) {
		if(m_dar > 0) {
			m_thumbW = (int)(m_thumbH*m_dar);
			EnsureMultipleOfDivisor(m_thumbW, 2);
		}
	}
	if(m_yuvW <= 0 || m_yuvH <= 0 || m_thumbW <= 0 || m_thumbH <= 0) {
		logger_err(LOGM_TS_VE, "YUV frame size or thumbnail size not set in CThumbnailFilter.\n");
		return false;
	}

	if(m_thumbType == THUMB_DEFAULT) {
		m_thumbType = THUMB_BMP;
	}

	if(!m_bStitching) m_enablePackImage = false;
	calculateTable();

	if(m_endTime == 0) {
		m_endTime = m_duration;
	}
	// Calculate capture point time(seconds)
	if(m_thumbInterval > 0) {	// Set every thumb interval snapshot a image
		m_thumbCount = (m_endTime-m_startTime)/m_thumbInterval;
		if(m_startTime <= 0) m_startTime = m_thumbInterval;
		for(int i=0; i< m_thumbCount; ++i) {
			m_capturePoint.push_back(m_startTime+i*m_thumbInterval);
		}
	} else {		// Set start/end time and thumbnail count
		m_capturePoint.push_back(m_startTime);
		if(m_thumbCount > 1) {	// Multiple thumbnail
			int intervalSecs = (m_endTime-m_startTime)/m_thumbCount;
			for (int i=1; i<m_thumbCount; ++i) {
				m_capturePoint.push_back(m_startTime + intervalSecs*i);
			}
		}
	}
	
	if(m_prefixName.empty()) {
		m_prefixName = "Thumbnail";
	}

	if(m_folder.empty()) {
		logger_err(LOGM_TS_VE, "folder path not set in CThumbnailFilter.\n");
		return false;
	}
	return true;
}

void CThumbnailFilter::fixImageSizeAccordingCropMode(cimg_library::CImg<uint8_t>* img, int w, int h)
{
	if(!img) return;
	if(w <= 0 || h <= 0) return;
	float thumbNailDar = (float)w/(float)h;
	if(!FloatEqual(thumbNailDar, m_dar, 0.05f)) {
		if(m_cropMode == 1) {			// crop
			if(m_dar > thumbNailDar) {	// crop along width
				int calcW = (int)(img->height()*thumbNailDar);
				EnsureMultipleOfDivisor(calcW, 2);
				int deltaX = (img->width() - calcW)/2;
				if(deltaX > 0) {
					EnsureMultipleOfDivisor(deltaX, 2);
					img->crop(deltaX, img->width()-deltaX-1);
				}
			} else {
				int calcH = (int)(img->width()/thumbNailDar);
				EnsureMultipleOfDivisor(calcH, 2);
				int deltaY = (img->height() - calcH)/2;
				if(deltaY > 0) {
					EnsureMultipleOfDivisor(deltaY, 2);
					img->crop(0, deltaY, img->width()-1, img->height()-deltaY-1);
				}
			}
		} else if (m_cropMode == 2) {		// expand
			if(m_dar > thumbNailDar) {		// Expand along height
				int calcH = (int)(img->width()/thumbNailDar);
				EnsureMultipleOfDivisor(calcH, 2); 
				img->resize(-100, calcH, -100, -100, 0, 0, 0.f, 0.5f);	// no interpolate
			} else {						// Expand along width
				int calcW = (int)(img->height()*thumbNailDar);
				EnsureMultipleOfDivisor(calcW, 2);
				img->resize(calcW, -100, -100, -100, 0, 0, 0.5f);		// no interpolate
			}
		}
	}
	img->resize(w, h, -100, -100, 3, 2);	// Linear resize(shrink)
}

bool CThumbnailFilter::GenerateThumbnail(uint8_t *pYuvBuf)
{
	m_frameCount++;
	if(m_thumbIndex > m_thumbCount) return true;
	// Convert capture point time to frames
	if(m_frameCount != (int)(m_capturePoint.at(m_thumbIndex-1)*m_fps+0.5f)) return true;
	
	// Generate thumb image
	CImg<uint8_t>* img = new CImg<uint8_t>(m_yuvW, m_yuvH, 1, 3);
	if(!img) return false;

	convertYUV2RGB(pYuvBuf, img->data());
	// Use video dar to fix image distortion
	if(!FloatEqual(m_dar, (float)m_yuvW/(float)m_yuvH, 0.06f)) {	//if dar is not equal to w/h
		int tmpW = (int)(m_yuvH*m_dar);
		EnsureMultipleOfDivisor(tmpW, 2);
		img->resize(tmpW, -100, -100, -100, 6, 2);	// Lancros resize(may be enlarge here)
	}

	if(m_bStitching && m_thumbCount > 1) {
		fixImageSizeAccordingCropMode(img, m_thumbW, m_thumbH);
		m_imgList.push_back(img);
	} else {
		if(m_bEnableMultiSize) {
			for(size_t i=0; i<m_multiSizes.size(); ++i) {
				genSingleImage(img, m_multiSizes[i].x, m_multiSizes[i].y);
			}
		} else {
			genSingleImage(img, m_thumbW, m_thumbH);
		}
		
		delete img;
	}
	
	// If all thumbnails are done or reach to row*col, then generate a stitched image
	if(m_bStitching && (m_thumbIndex == m_thumbCount || 
		(m_gridRow>0 && m_thumbIndex%(m_gridRow*m_gridCol)==0)) ) {
		genStitchingImage(m_capturePoint.at(m_thumbIndex-1));
	}

	m_thumbIndex++;
	return true;
}

bool CThumbnailFilter::StopThumbnail()
{
	if(!m_imgList.empty()) {
		genStitchingImage(m_capturePoint.at(m_thumbIndex-1));
	}

	// Generate ipk file(pack all images into a ipk file)
	if(m_enablePackImage) {
		if(m_packImageFiles.size() == 0) {
			logger_err(LOGM_TS_VE, "No thumbnail images to pack.\n");
			return false;
		}
		std::string ipkFile = m_folder + PATH_DELIMITER + m_prefixName + ".ipk";
		FILE* fpIpk = fopen(ipkFile.c_str(), "wb");
		if(!fpIpk) {
			logger_err(LOGM_TS_VE, "Create ipk file failed.\n");
			return false;
		}
		// Write ipk file header
		size_t imgHeaderLen = 0;
		size_t u16Len = sizeof(uint16_t);
		fwrite("PIPK", 1, 4, fpIpk);					// "PIPK" indicate file type
		imgHeaderLen += 4;

		uint16_t imageType = (uint16_t)m_thumbType;		// Thumbnail image type
		fwrite(&imageType, u16Len, 1, fpIpk);
		imgHeaderLen += u16Len;

		uint16_t tileRow = (uint16_t)m_gridRow;			// Tile row count
		fwrite(&tileRow, u16Len, 1, fpIpk);
		imgHeaderLen += u16Len;

		uint16_t tileCol = (uint16_t)m_gridCol;			// Tile column count
		fwrite(&tileCol, u16Len, 1, fpIpk);
		imgHeaderLen += u16Len;

		uint16_t cellImgW = (uint16_t)m_thumbW;			// Cell image width
		fwrite(&cellImgW, u16Len, 1, fpIpk);
		imgHeaderLen += u16Len;

		uint16_t cellImgH = (uint16_t)m_thumbH;			// Cell image height
		fwrite(&cellImgH, u16Len, 1, fpIpk);
		imgHeaderLen += u16Len;

		uint16_t bigImgCount = (uint16_t)(m_packImageFiles.size());
		fwrite(&bigImgCount, u16Len, 1, fpIpk);			// Image count
		imgHeaderLen += u16Len;

		uint16_t thumbInterval = (uint16_t)(m_thumbInterval);
		fwrite(&thumbInterval, u16Len, 1, fpIpk);		// Cell image interval
		imgHeaderLen += u16Len;

		// Write image offset table
		uint32_t imagesLen = 0;
		for(size_t i=0; i< m_packImageFiles.size(); ++i) {
			FILE* fp = fopen(m_packImageFiles[i].c_str(), "rb");
			if(!fp) {
				logger_err(LOGM_TS_VE, "Open file failed:%s.\n", m_packImageFiles[i].c_str());
				if(fpIpk) fclose(fpIpk);
				return false;
			}
			m_imgOffsets[i].offset = imgHeaderLen + bigImgCount*sizeof(image_offset_t)+imagesLen;
			fseek(fp, 0L, SEEK_END);
			imagesLen += (uint32_t)ftell(fp);
			fclose(fp);

			fwrite(&m_imgOffsets[i].offset, sizeof(uint32_t), 1, fpIpk);
			fwrite(&m_imgOffsets[i].startTime, sizeof(uint32_t), 1, fpIpk);
		}

		// Write every image to ipk file
		for(size_t i=0; i< m_packImageFiles.size(); ++i) {
			FILE* fp = fopen(m_packImageFiles[i].c_str(), "rb");
			if(fp) {
				while(!feof(fp)) {
					const int bufLen = 2048;
					uint8_t tmpBuf[bufLen] = {0};
					size_t readLen = fread(tmpBuf, 1, bufLen, fp);
					if(readLen > 0) {
						fwrite(tmpBuf, 1, readLen, fpIpk);
					}
				}
				fclose(fp);
			}
		}
		fclose(fpIpk);

		// Remove intermedia big images
		for(size_t i=0; i< m_packImageFiles.size(); ++i) {
			RemoveFile(m_packImageFiles[i].c_str());
		}
	}
	return true;
}

void CThumbnailFilter::genSingleImage(CImg<uint8_t>* img, int w, int h)
{
	if(!img || !w || !h) return;
	std::string thumbFile = getThumbFileName(w, h);
	
	try {
		CImg<uint8_t> tmpImg(*img);
		fixImageSizeAccordingCropMode(&tmpImg, w, h);
		tmpImg.save(thumbFile.c_str(), m_imageQuality);
		if(m_thumbType == THUMB_JPG && m_bOptimizeImage) {
			OptimizeJpeg(thumbFile.c_str());
		}
	} catch (CImgIOException& e) {
		logger_err(LOGM_TS_VE, "%s!\n", e.what());
	}
}

void CThumbnailFilter::genStitchingImage(int startSec)
{
	do {
		size_t imgNum = m_imgList.size();
		if(imgNum == 0) return;
		std::string thumbFile = getThumbFileName();
		
		// if only one image left
		/*if(imgNum == 1) {
			try {
				m_imgList.at(0)->save(thumbFile.c_str(), m_imageQuality);
			} catch (CImgIOException& e) {
				logger_err(LOGM_TS_VE, "%s!\n", e.what());
			}
			break;
		}*/

		// Calculate stitching row/col
		int imgRow = m_gridRow;
		int imgCol = m_gridCol;
		switch(m_stitchAlign) {
		case STITCH_TILE: 
			m_gridRow = (size_t)(::sqrt((float)imgNum));
			imgCol = imgNum/imgRow + imgNum%imgRow;	  
			break;
		case STITCH_HORIZONTAL:
			imgCol = imgNum;
			break;
		case STITCH_VERTICAL:
			imgRow = imgNum;
			break;
		}

		int destWidth = m_thumbW*imgCol, destHeight = m_thumbH*imgRow;
		CImg<uint8_t> destImg(destWidth, destHeight, 1, 3, 0);

		bool shouldBreak = false;
		for (size_t i=0; i<imgRow; ++i) {
			for (size_t j=0; j<imgCol; ++j) {
				size_t imgIdx = imgCol*i + j;
				if(imgIdx >= imgNum) {
					shouldBreak = true;
					break;
				}
				CImg<uint8_t>* curImg = m_imgList[imgIdx];

				// Loop each image buffer
				for(int h=0; h<m_thumbH; ++h) {		// Each loop get a line of data
					// R color plane
					uint8_t* p = destImg.data(j*m_thumbW, i*m_thumbH+h, 0, 0);
					memcpy(p, curImg->data(0, h, 0, 0), m_thumbW);
					// G color plane
					p = destImg.data(j*m_thumbW, i*m_thumbH+h, 0, 1);
					memcpy(p, curImg->data(0, h, 0, 1), m_thumbW);
					// B color plane
					p = destImg.data(j*m_thumbW, i*m_thumbH+h, 0, 2);
					memcpy(p, curImg->data(0, h, 0, 2), m_thumbW);
				}
			}
			if(shouldBreak) {
				break;
			}
		}
		try {
			destImg.save(thumbFile.c_str(), m_imageQuality);
		} catch (CImgIOException& e) {
			logger_err(LOGM_TS_VE, "%s!\n", e.what());
		}

		if(m_thumbType == THUMB_JPG && m_bOptimizeImage) {
			OptimizeJpeg(thumbFile.c_str());
		}

		//If pack images into ipk file, then record file name
		if(m_enablePackImage) {
			m_packImageFiles.push_back(thumbFile);
			image_offset_t curStetchedImageOffset = {0, startSec};
			m_imgOffsets.push_back(curStetchedImageOffset);
		}
	} while (false);
	
	// Release memory
	for (size_t i=0; i<m_imgList.size(); ++i) {
		delete m_imgList.at(i);
	}
	m_imgList.clear();
}

static unsigned char clp[1024] = {0};

/* Data for ConvertYUVtoRGB*/
static int crv_tab[256];
static int cbu_tab[256];
static int cgu_tab[256];
static int cgv_tab[256];
static int tab_76309[256];
void CThumbnailFilter::calculateTable()
{
	long int crv,cbu,cgu,cgv;
	int i,ind;   

	crv = 104597; cbu = 132201;  /* fra matrise i global.h */
	cgu = 25675;  cgv = 53279;

	for (i = 0; i < 256; i++) {
		crv_tab[i] = (i-128) * crv;
		cbu_tab[i] = (i-128) * cbu;
		cgu_tab[i] = (i-128) * cgu;
		cgv_tab[i] = (i-128) * cgv;
		tab_76309[i] = 76309*(i-16);
	}

	for (i=0; i<384; i++)
		clp[i] =0;
	ind=384;
	for (i=0;i<256; i++)
		clp[ind++]=i;
	ind=640;
	for (i=0;i<384;i++)
		clp[ind++]=255;
}

void CThumbnailFilter::convertYUV2RGB(uint8_t* pYuvBuf, uint8_t* dstRGB)
{
	int y11,y21;
	int y12,y22;
	int y13,y23;
	int y14,y24;
	int u,v; 
	int i,j;
	int c11, c21, c31, c41;
	int c12, c22, c32, c42;
	
	unsigned char *py1,*py2,*pu,*pv;
	
	int planeSize = m_yuvW*m_yuvH;
	uint8_t* dstR = dstRGB;
	uint8_t* dstG = dstR+planeSize;
	uint8_t* dstB = dstG+planeSize;

	py1 = pYuvBuf; 
	pv  = pYuvBuf+planeSize; 
	pu  = pv + planeSize/4;

	py2 = py1 + m_yuvW;

	for (j = 0; j < m_yuvH; j += 2) { 
		int counterInRow = j*m_yuvW; 
		int counterInNextRow = counterInRow + m_yuvW;
		for (i = 0; i < m_yuvW; i += 4) {
			u = *pu++;
			v = *pv++;
			c11 = crv_tab[v];
			c21 = cgu_tab[u];
			c31 = cgv_tab[v];
			c41 = cbu_tab[u];
			u = *pu++;
			v = *pv++;
			c12 = crv_tab[v];
			c22 = cgu_tab[u];
			c32 = cgv_tab[v];
			c42 = cbu_tab[u];

			y11 = tab_76309[*py1++]; // (255/219)*65536 
			y12 = tab_76309[*py1++];
			y13 = tab_76309[*py1++]; // (255/219)*65536 
			y14 = tab_76309[*py1++];

			y21 = tab_76309[*py2++];
			y22 = tab_76309[*py2++];
			y23 = tab_76309[*py2++];
			y24 = tab_76309[*py2++];

			// Column 1
			*(dstR+counterInRow)		= clp[384+((y11 + c41)>>16)];	
			*(dstR+counterInNextRow)	= clp[384+((y21 + c41)>>16)];
			
			*(dstG+counterInRow)		= clp[384+((y11 - c21 - c31)>>16)];
			*(dstG+counterInNextRow)	= clp[384+((y21 - c21 - c31)>>16)];
			
			*(dstB+counterInRow)		= clp[384+((y11 + c11)>>16)];
			*(dstB+counterInNextRow)	= clp[384+((y21 + c11)>>16)];

			counterInRow++;
			counterInNextRow++;
			if(counterInRow == j*m_yuvW+m_yuvW) continue;

			// Column 2
			*(dstR+counterInRow)		= clp[384+((y12 + c41)>>16)];
			*(dstR+counterInNextRow)	= clp[384+((y22 + c41)>>16)];

			*(dstG+counterInRow)		= clp[384+((y12 - c21 - c31)>>16)];
			*(dstG+counterInNextRow)	= clp[384+((y22 - c21 - c31)>>16)];

			*(dstB+counterInRow)		= clp[384+((y12 + c11)>>16)];
			*(dstB+counterInNextRow)	= clp[384+((y22 + c11)>>16)];
			
			counterInRow++;
			counterInNextRow++;
			if(counterInRow == j*m_yuvW+m_yuvW) continue;

			// Column 3
			*(dstR+counterInRow)		= clp[384+((y13 + c42)>>16)];
			*(dstR+counterInNextRow)	= clp[384+((y23 + c42)>>16)];

			*(dstG+counterInRow)		= clp[384+((y13 - c22 - c32)>>16)];
			*(dstG+counterInNextRow)	= clp[384+((y23 - c22 - c32)>>16)];

			*(dstB+counterInRow)		= clp[384+((y13 + c12)>>16)];
			*(dstB+counterInNextRow)	= clp[384+((y23 + c12)>>16)];
			
			counterInRow++;
			counterInNextRow++;
			if(counterInRow == j*m_yuvW+m_yuvW) continue;

			// Column 4
			*(dstR+counterInRow)		= clp[384+((y14 + c42)>>16)];
			*(dstR+counterInNextRow)	= clp[384+((y24 + c42)>>16)];

			*(dstG+counterInRow)		= clp[384+((y14 - c22 - c32)>>16)];
			*(dstG+counterInNextRow)	= clp[384+((y24 - c22 - c32)>>16)];

			*(dstB+counterInRow)		= clp[384+((y14 + c12)>>16)];
			*(dstB+counterInNextRow)	= clp[384+((y24 + c12)>>16)];

			counterInRow++;
			counterInNextRow++;
			//if(counterInRow == j*m_yuvW+m_yuvW) continue;
		}
		
		py1 += m_yuvW;
		py2 += m_yuvW;
	}
} 

std::string CThumbnailFilter::getThumbFileName(int w, int h)
{
	std::string thumbFile = m_folder + PATH_DELIMITER + m_prefixName;
	// If enable pack image into ipk file, ensure every output image's name is different
	if((m_thumbCount > 1 && !m_bStitching) || m_enablePackImage) {
		thumbFile += '_';
		char indexStr[12] = {0};
		_itoa(m_thumbIndex, indexStr, 10);
		thumbFile += indexStr;
	}

	if(m_bEnableMultiSize && !m_bStitching) {
		char sizeStr[64] = {0};
		sprintf(sizeStr, m_sizePostfix.c_str(), w, h);
		thumbFile += sizeStr;
	}

	switch(m_thumbType) {
		case THUMB_PNG:
			thumbFile += ".png";
			break;
		case THUMB_JPG:
			thumbFile += ".jpg";
			break;
		case THUMB_GIF:
			thumbFile += ".gif";
			break;
		default:
			thumbFile += ".bmp";
			break;
	}
	return thumbFile;
}

/*
//  Convert from YUV420 to RGB24. Sequence of dstRGB: RRRRRRRRRRR...GGGGGGGGG...BBBBBBBB
//
void CThumbnailFilter::convertYUV2RGB(uint8_t* pYuvBuf, uint8_t* dstRGB)
{
	const int planeSize = m_yuvH*m_yuvW;
	uint8_t* pUplan = pYuvBuf + planeSize;
	uint8_t* pVplan = pUplan + planeSize/4;

	int rgbIndex = 0;
	for (int j=0; j<m_yuvH; ++j) {
		for (int i=0; i<m_yuvW; ++i) {
			int yIndex = j*m_yuvW + i;
			uint8_t y = pYuvBuf[yIndex];
			int uvIndex = j*m_yuvW/4 + i/2;
			uint8_t u = pUplan[uvIndex];
			uint8_t v = pVplan[uvIndex];

			int tmpU = u - 128;
			int tmpV = v - 128;

			int rdif = tmpV + ((tmpV * 103) >> 8);
			int invgdif = ((tmpU * 88) >> 8) +((tmpV * 183) >> 8);
			int bdif = tmpU +( (tmpU*198) >> 8);

			int r = y + rdif;
			int g = y - invgdif;
			int b = y + bdif;
			if(r > 255) r = 255;
			if(g > 255) g = 255;
			if(b > 255) b = 255;

			if(r < 0) r = 0;
			if(g < 0) g = 0;
			if(b < 0) b = 0;

			dstRGB[rgbIndex] = r;
			dstRGB[rgbIndex+planeSize] = g;
			dstRGB[rgbIndex+planeSize*2] = b;
			rgbIndex ++;
		}
	}
}
*/
