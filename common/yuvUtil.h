#ifndef _YUV_UTIL_H_
#define _YUV_UTIL_H_
#include "stdint.h"

class CYuvUtil {
public:
	struct FrameRect {
		int x, y;
		int w, h;
	};
	static bool EqualFloat(float x, float y, float epsilon = 1e-4);
	static FrameRect CalcAutoCrop(float srcDar, float dstDar, int srcW, int srcH);
	static FrameRect CalcAutoExpand(float srcDar, float dstDar, int srcW, int srcH);
	static FrameRect AutoDetectCrop(const char* fileName, int duration = 0, int detectThreshold = 24);
	// Destinate buffer is allocated inside, should call FreeBuffer to release after use.
	static bool Crop(const FrameRect& cropParam, int srcW, int srcH, uint8_t* srcYuv, uint8_t** dstYuv);

	// Destinate buffer is allocated inside, should call FreeBuffer to release after use.
	static bool Expand(const FrameRect& cropParam, int srcW, int srcH, uint8_t* srcYuv, uint8_t** dstYuv);

	// Free buffer that is allocated by this class
	static void FreeBuffer(uint8_t** pBuffer);
};

void GetFraction(double value, int* num, int* den);

template<typename T>
T GetGcd(T a, T b) {
	while( true ) {
		T c = a % b;
		if( !c )
			return b;
		a = b;
		b = c;
	}
}

class CAspectRatio
{
public:
	CAspectRatio():dar(1) {}
	CAspectRatio(int width, int height)
	{
		SetRes(width, height);
		dar = (double)width / height;
	}
	void SetRes(int width, int height)
	{
		this->width = width;
		this->height = height;
	}
	void SetDAR(double dar)
	{
		if (dar > 0) {
			this->dar = dar;
		} else {
			dar = (double)width / height;
		}
	}
	void SetDAR(int dar_num, int dar_den)
	{
		if (dar_num > 0 && dar_den > 0) {
			dar = (double)dar_num / dar_den;
		} else {
			dar = (double)width / height;
		}
	}
	void SetPAR(double par)
	{
		if (par > 0) {
			dar = par * width / height;
		} else {
			dar = (double)width / height;
		}
	}
	void SetPAR(int par_num, int par_den)
	{
		if (par_num > 0 && par_den > 0) {
			dar = (double)par_num / par_den * width / height;
		} else {
			dar = (double)width / height;
		}
	}
	double GetPAR(int* par_num = 0, int* par_den = 0);
	double GetDAR(int* dar_num = 0, int* dar_den = 0);
private:
	int width;
	int height;
	double dar;
};
#endif