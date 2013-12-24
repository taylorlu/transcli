#ifndef _WATER_MARK_H_
#define _WATER_MARK_H_

#include <stdint.h>

#include <vector>
#include "watermakermacro.h"
#include "math/Vector3.h"

#pragma warning(disable: 4251)

namespace cimg_library {	// Declaration for CImg
	template<typename T>
	struct CImg;
}

enum water_mark_pos_t {
	WATERMARK_CEN,
	WATERMARK_LU,
	WATERMARK_RU,
	WATERMARK_RD,
	WATERMARK_LD
};

enum image_format_t {
	FORMAT_PNG,
	FORMAT_JPG,
	FORMAT_BMP,
	FORMAT_ALL_SUPPORTED
};

enum logo_move_route_t {
	LOGO_MOVE_ALONG_X,
	LOGO_MOVE_ALONG_Y,
	LOGO_MOVE_ALONG_CROSS
};

class WATERMARK_EXT CWaterMarkFilter
{
public:
	CWaterMarkFilter(void);
	struct pos_time_t {
		int startMs;
		int endMs;
		int left;
		int top;
		int w;
		int h;
	};

	struct relative_pos_time_t {
		int startMs;
		int endMs;
		water_mark_pos_t pos;
		int offsetX;
		int offsetY;
	};

	struct move_logo_t {
		std::vector<int>  moveRoute;	// moving route
		int				  moveSpeed;	// pixels per second
		int				  pauseTime;	// pause x seconds each time the logo reach the end of route
		float			  sina;
		float			  cosa;	
		int				  offsetX;
		int				  offsetY;
		
		int				  curPos;
		size_t			  curRouteIdx;
		int				  isPaused;
		float			  curPosX;
		float			  curPosY;
		int				  frameCounter;
		float			  weightX;		// delta value weight of x when logo is moving(+1/-1/cosa)
		float			  weightY;		// delta value weight of y when logo is moving(+1/-1/sina)
	};

	//-----------Set source for water mark, select one of following two functions--------------
	// Source is image
	bool SetWaterMarkImages(std::vector<const char*>& imgFiles, bool ajustToFitSize=false);
	// Source is text
	bool SetWaterMarkText(const char* strText, int fontSize,
		const Vector3ub& foreColor, const Vector3ub& backColor);
	
	// Time string logo
	bool SetTimeStringFormat(const char* timeFormat, int fontSize, 
		const Vector3ub& foreColor, const Vector3ub& backColor);

	//-----------------------Set params for water mark-----------------------------------------
	// Set interval if the logo is dynamic (multiple images)
	void SetInterval(int intervalMs) {m_intervalMs = intervalMs;}
	// Set watermark show times and positions(width or height is 0, means not to scale image)
	void AddShowTimeAndPosition(int startMs, int endMs, int left, int top, int w, int h);
	void AddShowTimeAndPosition(int startMs = 0, int endMs = -1, water_mark_pos_t pos = WATERMARK_RD,
		int offsetX = 0, int offsetY = 0, int w = 0, int h = 0, 
		int speed = 0, const char* route = NULL, int waitTime = -1);

	// Set alpha(0~1), blend water mark and original yuv frame. [ WK*alpha + YUV*(1-alpha) ]
	void SetAlpha(float alphaValue) {m_alpha = alphaValue;}
	// Set transparent color
	void SetTransparentColor(const Vector3ub& transparentColor) 
	{
		m_transparentColor = transparentColor;
		m_enableTransColor = true;
	}
	
	// Whether overlay u/v plan
	void EnableColor(bool ifEnable) {m_enableColor = ifEnable;}

	//-----------------------Set params for original yuv frame---------------------------------
	void SetYUVFrameSize(int yuvWidth, int yuvHeight);
	void SetYUVFps(float fps) {m_yuvFps = fps;}

	//-----------------------Add water mark to YUV frame---------------------------------------
	bool RenderWaterMark(uint8_t* pYuvBuf);

	//--------------------------------Delogo---------------------------------------------------
	

	//-----------------------Helper function---------------------------------------------------
	// Dump image data to text file
	bool DumpImageToText(const char* txtFileName);
	// Load logo from memory
	bool LoadLogo(bool ajustToFitSize=false);

	~CWaterMarkFilter(void);

private:
	int GetTimeIntervalIndex();
	void adjustComponentWeight(int routeType, int pos);
	void adjustLogoPosition(int logoW, int logoH);
	std::vector<std::string> m_imgFiles;
	std::vector<cimg_library::CImg<uint8_t>*> m_vctImgs;
	int m_intervalMs;

	std::vector<pos_time_t> m_posTimes;
	std::vector<relative_pos_time_t> m_relativePosTimes;
	

	Vector3ub m_transparentColor;
	Vector3ub m_textForeColor;
	Vector3ub m_textBackColor;
	int		  m_textFontSize;

	float m_alpha;
	int	  m_yuvWidth;
	int   m_yuvHeight;
	float m_yuvFps;
	uint32_t m_frames;
	int   m_curImgIndex;
	int   m_imgWidth;
	int   m_imgHeight;
	bool  m_enableColor;
	bool  m_ajustToFitSize;
	bool  m_enableTransColor;

	// Variables relate to moving logo
	move_logo_t m_moveLogo;		// moving logo
	std::string m_timeFormat;	// Time string format
	char m_curTimeString[256];
};

class WATERMARK_EXT CWaterMarkManager
{
public:
	CWaterMarkManager();
	CWaterMarkFilter* AddWaterMark(std::vector<const char*>& imgFiles, bool ajustToFitSize);

	CWaterMarkFilter* AddWaterMark(const char* imgDir, 
		image_format_t imgFormat = FORMAT_ALL_SUPPORTED, bool ajustToFitSize=false);

	CWaterMarkFilter* AddTextMark(const char* strText, int fontSize = 24,
		const Vector3ub& foreColor=Vector3ub(255,255,255),
		const Vector3ub& backColor=Vector3ub(0,0,0));

	CWaterMarkFilter* AddWaterMark(bool ajustToFitSize = false);

	/*----------- time format as following ----------------------------------------------
		%a	Abbreviated weekday name *	Thu
		%A	Full weekday name * 	Thursday
		%b	Abbreviated month name *	Aug
		%B	Full month name *	August
		%c	Date and time representation *	Thu Aug 23 14:55:02 2001
		%d	Day of the month (01-31)	23
		%H	Hour in 24h format (00-23)	14
		%I	Hour in 12h format (01-12)	02
		%j	Day of the year (001-366)	235
		%m	Month as a decimal number (01-12)	08
		%M	Minute (00-59)	55
		%p	AM or PM designation	PM
		%S	Second (00-61)	02
		%U	Week number with the first Sunday as the first day of week one (00-53)	33
		%w	Weekday as a decimal number with Sunday as 0 (0-6)	4
		%W	Week number with the first Monday as the first day of week one (00-53)	34
		%x	Date representation *	08/23/01
		%X	Time representation *	14:55:02
		%y	Year, last two digits (00-99)	01
		%Y	Year	2001
		%Z	Timezone name or abbreviation	CDT
		%%	A % sign	%    */
	CWaterMarkFilter* AddTimeStringMark(const char* timeFormat="%X", int fontSize = 24, 
		const Vector3ub& foreColor=Vector3ub(255,255,255),
		const Vector3ub& backColor=Vector3ub(0,0,0));

	CWaterMarkFilter* GetFirstWaterMark();

	void SetYUVInfo(int yuvWidth, int yuvHeight, float fps);
	void RenderWaterMark(uint8_t* pYuvBuf);
	~CWaterMarkManager();

private:
	std::vector<CWaterMarkFilter*> m_vctWaterMarks;
};

#endif
