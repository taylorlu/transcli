#ifndef _THUMB_NAIL_H_
#define _THUMB_NAIL_H_

#include <string>
#include <vector>
#include <stdint.h>
#include "math/Vector2.h"
#include "watermakermacro.h"

namespace cimg_library {	// Declaration for CImg
	template<typename T>
	struct CImg;
}


class WATERMARK_EXT CThumbnailFilter
{
public:
	enum ThumbnailType {
		THUMB_DEFAULT,	// Default is BMP
		THUMB_PNG,
		THUMB_JPG,
		THUMB_BMP,
		THUMB_GIF
	};

	enum StitchAlign {
		STITCH_TILE,
		STITCH_HORIZONTAL,
		STITCH_VERTICAL,
		STITCH_USER_GRID
	};

	struct image_pack_header_t {
		char fileType[4];		// "PIPK" indicate file type
		uint16_t imageType;		// Thumbnail image type
		uint16_t tileRow;		// Tile row count
		uint16_t tileCol;		// Tile column count
		uint16_t cellImgW;		// Cell image width
		uint16_t cellImgH;		// Cell image height
		uint16_t bigImgCount;	// Big image count(after stitching)
		uint16_t thumbInterval; // Thumb image interval(seconds)
	};

	struct image_offset_t {
		uint32_t offset;		// Bytes offset from file start
		uint32_t startTime;		// Start seconds of this big image(stitched)
	};

	CThumbnailFilter(void);
	~CThumbnailFilter(void);
	//------------------------- Thumbnail Param settings ----------------------------------
	void SetFolder(const char* folderPath);
	void SetPrefixName(const char* prefixName) {if(prefixName) m_prefixName = prefixName;}
	void SetPostfixName(const char* postfixName) {if(postfixName) m_postfixName = postfixName;}
	void SetImageFormat(int thumbType) {m_thumbType = (ThumbnailType)thumbType;}
	void SetImageQuality(unsigned int quality) {m_imageQuality = quality;}
	void SetCropMode(int cropMode) {m_cropMode = cropMode;}
	void SetThumbnailSize(int w, int h) {m_thumbW = w; m_thumbH = h;}
	void SetStartTime(int startTime) {m_startTime = startTime;}						// Thumbnail start time
	void SetEndTime(int endTime) {m_endTime = endTime;}								// Thumbnail end time
	void SetThumbnailCount(int thumbCount) {m_thumbCount = thumbCount;}				// Thumbnails count
	void SetThumbnailInterval(int secs) {m_thumbInterval = secs;}
	void SetEanbleStitching(bool enableStitch) {m_bStitching = enableStitch;}		// Enable stitching thumbnails to large image
	void SetStitchAlign(int alignment) {m_stitchAlign = alignment;}					// Stitching alignment
	void SetStitchGrid(int row, int col){m_gridRow=row; m_gridCol=col;}				// Row/col count of user grid
	void SetEnablePackImage(bool enablePack) {m_enablePackImage = enablePack;}		// Pack thumb images into .ipk file(image package)
	void SetEnableOptimize(bool bOptimizeImage) {m_bOptimizeImage = bOptimizeImage;}

	//------------------------- Multi target size support ------------------------------
	//------- Every thumbnail output multiple sizes, except stitching mode ------------
	void SetEnableMultiSize(bool enableMultiSize) {m_bEnableMultiSize = enableMultiSize;}
	void AddThumbSize(Vector2i thumbSize) {m_multiSizes.push_back(thumbSize);}
	void SetSizePostfixFormat(const char* sizePostfix) {if(sizePostfix) m_sizePostfix = sizePostfix;}

	//------------------------- Movie Param settings ----------------------------------
	void SetFps(float fps) {m_fps = fps;}
	void SetDuration(int duration) {m_duration = duration;}		// seconds
	void SetYUVFrameSize(int w, int h) {m_yuvW = w; m_yuvH = h;}
	void SetVideoDar(float dar) {m_dar = dar;}

	//------------------------- Generate Thumbnail ----------------------------------
	bool CalculateCapturePoint();				// Calculate the frame where to generate thumbnail (multiple thumbnails)
	bool GenerateThumbnail(uint8_t* pYuvBuf);	// Generate thumbnail
	bool StopThumbnail();

private:
	void calculateTable();
	void convertYUV2RGB(uint8_t* pYuvBuf, uint8_t* dstRGB);
	void genStitchingImage(int startSec);	// startSec:Time of first image of stitched file
	void genSingleImage(cimg_library::CImg<uint8_t>* img, int w, int h);
	void fixImageSizeAccordingCropMode(cimg_library::CImg<uint8_t>* img, int w, int h);
	std::string getThumbFileName(int w = 0, int h = 0);

	ThumbnailType m_thumbType;
	int m_yuvW;
	int m_yuvH;
	int m_thumbW;
	int m_thumbH;
	int m_thumbCount;
	int m_thumbIndex;
	int m_thumbInterval;

	int m_startTime;
	int m_endTime;
	uint32_t m_frameCount;
	float m_fps;
	int   m_duration;

	bool  m_bStitching;
	int   m_stitchAlign;
	int   m_gridRow;
	int   m_gridCol;
	bool  m_enablePackImage;	// Enable image packing
	bool  m_bOptimizeImage;		// Enable image optimize(Jpegtran)

	unsigned int   m_imageQuality;
	int	  m_cropMode;
	float m_dar;

	// Multi target size support 
	bool m_bEnableMultiSize;
	std::vector<Vector2i> m_multiSizes;

	std::vector<int> m_capturePoint;
	std::string m_folder;
	std::string m_prefixName;
	std::string m_postfixName;
	std::string m_sizePostfix;
	std::vector<cimg_library::CImg<uint8_t>* > m_imgList;

	std::vector<image_offset_t> m_imgOffsets;	// Record image offset for pack into ipk file
	std::vector<std::string> m_packImageFiles;	// Image file path for packing
};

#endif
