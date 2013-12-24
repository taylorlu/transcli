#ifndef _VIDEO_PREVIEW_H_
#define _VIDEO_PREVIEW_H_
#include "watermakermacro.h"
#include "stdint.h"
#include <string>
#include <vector>
#include "BiThread.h"

namespace cimg_library {	// Declaration for CImg
	template<typename T>
	struct CImg;
	struct CImgDisplay;
}

class WATERMARK_EXT CVideoPreview
{
public:
	CVideoPreview(int yuvW, int yuvH);
	bool Initialize();
	bool GeneratePipeHandle();
	bool StartPreview();
	~CVideoPreview(void);

private:
	CVideoPreview(void);
	static unsigned long previewThread(void* videoPreview);

	cimg_library::CImgDisplay* m_previewRect;
	cimg_library::CImg<uint8_t>* m_imgFrame;
	CBIThread::Handle m_hThread;
	int m_yuvW;
	int m_yuvH;
	int m_fdRead;
	int m_fdWrite;
};

#endif