#ifndef _IMAGE_SOURCE_H_
#define _IMAGE_SOURCE_H_

#include <string>
#include <vector>
#include <stdint.h>
#include "watermakermacro.h"

class WATERMARK_EXT CImageSrc
{
public:
	CImageSrc(void);
	~CImageSrc(void);
	// Set static image and how many times to show
	void SetStaticImage(const char* imgPath);
	void SetStaticImageDuration(int dur) {m_duration = dur;}

	// Set dynamic images folder, show one image each time
	void SetImageFolder(const char* imgFolder);
	
	// Set dest yuv propertyies
	void SetDestYuvSize(int width, int height);
	void SetDestDar(int darNum, int darDen);
	void SetDestFps(int fpsNum, int fpsDen);
	
	// Crop or add black band to adapt to dest video frame (0:crop, 1:padding)
	void SetCropMode(int cropMode = 0);

	// After set basic params, initialize
	bool Initialize();

	// For 2-pass encoding, Reset after first pass
	void Reset();

	// Get each yuv frame
	uint8_t* GetNextFrame();

private:
	void getCropParam(int& x0, int& y0, int& x1, int& y1, int imgW, int imgH);
	bool loadAndConvertImage(const char* imagePath);

	uint8_t*    m_yuvFrame;
	int			m_cropMode;
	int			m_yuvW;
	int			m_yuvH;
	int         m_yuvDarNum;
	int			m_yuvDarDen;
	int			m_fpsNum;
	int			m_fpsDen;
	int			m_duration;
	int			m_showCounts;
	std::string m_staticImage;
	std::vector<std::string> m_dynamicImages;
	int			m_dynamicIdx;
};

#endif