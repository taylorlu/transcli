#include "yuvUtil.h"
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include "processwrapper.h"
#include "util.h"
#include "logger.h"
#include "bitconfig.h"
#include "bit_osdep.h"

#ifdef WIN32
#include <windows.h>
#else
#define _abs64 llabs
#define max(x,y) ((x)>(y)?(x):(y))
#define min(x,y) ((x)<(y)?(x):(y))
#endif

bool CYuvUtil::EqualFloat(float x, float y, float epsilon)
{ 
	if(x-y < epsilon && x-y > -epsilon) {
		return true;
	} 
	return false;
}

CYuvUtil::FrameRect CYuvUtil::CalcAutoCrop(float srcDar, float dstDar, int srcW, int srcH)
{
	CYuvUtil::FrameRect rect = {0, 0, 0, 0};
	if(EqualFloat(dstDar, srcDar)) return rect;
	if(srcW <= 0 || srcH <= 0) return rect;
	int calcW = srcW;
	int calcH = srcH;
	float par = srcDar * srcH / srcW ;
	
	if(dstDar > srcDar) {
		calcH = (int)(calcW/srcDar+0.5f);
		int deltaH = calcH - (int)(calcW/dstDar+0.5f);
		deltaH = (int)(deltaH*par+0.5f);
		rect.y = deltaH / 2;
		rect.y -= rect.y%2;
		rect.h = srcH - rect.y*2;
		rect.w = calcW;
	} else {
		calcW = (int)(calcH*srcDar+0.5f);
		int deltaW = calcW - (int)(calcH*dstDar+0.5f);
		deltaW = (int)(deltaW*par+0.5f);
		rect.x = deltaW / 2;
		rect.x -= rect.x%2;
		rect.w = srcW - rect.x*2;
		rect.h = calcH;
	}
	return rect;
}

CYuvUtil::FrameRect CYuvUtil::CalcAutoExpand(float srcDar, float dstDar, int srcW, int srcH)
{
	CYuvUtil::FrameRect rect = {0, 0, 0, 0};
	if(EqualFloat(dstDar, srcDar)) return rect;

	int calcW = srcW;
	int calcH = srcH;
	float par = srcDar / srcW * srcH;
	if(!EqualFloat(par, 1.f)) {	// pixel is not square (par != 1)
		if(par > 1) {
			calcW = (int)(srcW*par);
		} else {
			calcH = (int)(srcH/par);
		}
	}

	if(dstDar > srcDar) {
		int deltaW = (int)(calcH*dstDar) - calcW;
		deltaW = (int)(deltaW*par+0.2);
		rect.x = deltaW / 2;
		rect.x -= rect.x%2;
		rect.w = srcW + rect.x*2;
	} else {
		int deltaH = (int)(calcW/dstDar) - calcH;
		deltaH = (int)(deltaH*par+0.2);
		rect.y = deltaH / 2;
		rect.y -= rect.y%2;
		rect.h = srcH + rect.y*2;
	}

	return rect;
}

CYuvUtil::FrameRect CYuvUtil::AutoDetectCrop(const char* fileName, int duration, int detectThreshold)
{
	CYuvUtil::FrameRect rect = {0, 0, 0, 0};
	do {
		if(!fileName || !FileExist(fileName)) {
			logger_err(LOGM_UTIL_YUVUTIL, "File not found when auto detect black band.\n");
			break;
		}

		if(detectThreshold <= 0) {
			detectThreshold = 26;
		}
		char vfStr[128] = {0};
		int selectInterval = 5;
		if(duration < 35000) {
			selectInterval = 1;
		}
		// Select one frame every 5 seconds
		sprintf(vfStr, " -vf \"select=isnan(prev_selected_t)+gte(t-prev_selected_t\\,%d),cropdetect=%d:4\"", selectInterval, detectThreshold);
		
		CProcessWrapper proc;
		std::string cmdString = FFMPEG;
		if(duration > 250000) {		// If duration is bigger than 250s
			cmdString += " -ss 150";
		} 
		cmdString += " -i \"";
		cmdString += fileName;
		cmdString += "\" -an -t 200 -c:v rawvideo";
		cmdString += vfStr;
		cmdString += " -f null -y "FFMPEG_NUL;
		
		// Count frames, when frames > 1000 then end detect.
		int frameDetectCount = 0;
		// If mplayer won't exit normally, then force it to end when frames > 1000
		const int maxFramesDetect = 1000;	
		
		proc.flags = SF_REDIRECT_OUTPUT;
		#ifdef DEBUG_EXTERNAL_CMD
		logger_info(LOGM_UTIL_YUVUTIL, "Auto detect cmd: %s.\n", cmdString.c_str());
		#endif
		bool success = proc.Spawn(cmdString.c_str());
		int procWaitRet = proc.Wait(500);
        if(procWaitRet == -1 ) {
			success = false;
		}

		if(!success) {
			proc.Cleanup();
			logger_err(LOGM_UTIL_YUVUTIL, "FFMpeg open file failed when auto detect black band.\n");
			break;
		}
	
		bool parseEnd = false;
		const int bufSize = 1024;
		char buf[bufSize+1] = {0};
		int bytesRead = 0;
		char finalCropStr[64] = {0};
		int readTryCount = 0;
		do {
			int oldBytes = bytesRead;
			bytesRead = proc.Read(buf + bytesRead, bufSize - bytesRead);
			if (bytesRead < 0) {
				if(proc.Wait(50) == 0) {
					bytesRead = oldBytes;
					readTryCount++;
					if(readTryCount <= 4) continue;
					logger_err(LOGM_UTIL_YUVUTIL, "Read buffer failed when detect black band and retry.\n");
					break;
				} else {
					proc.Cleanup();
					logger_err(LOGM_UTIL_YUVUTIL, "Read buffer failed when detect black band and exit.\n");
					break;
				}
			}
			
			char *lineStr = NULL;
			char *p = buf;
			for (;;) {
				// Get one line of string
				lineStr = p;
				// Find line ending
				for (; *p && *p != '\r' && *p != '\n'; p++);
				if (!*p) break;
				*p = 0;
				// Consume line ending char
				while (*(++p) == '\n' || *p == '\r');
				
				// Save current crop string.
				char* cropStr = NULL;
				if(cropStr = strstr(lineStr, "crop=")) {
					// Depend frame count to end detect when running without seek but duration is big.
					size_t cropStrLen = strlen(cropStr);
					if(cropStrLen > 32) cropStrLen = 32;
					memset(finalCropStr, 0, 64);
					memcpy(finalCropStr, cropStr, cropStrLen);		
					frameDetectCount++;
					if(frameDetectCount > maxFramesDetect) {
						parseEnd = true;
						break;
					}
				}
			}

			// chars left (not enough for a line), copy it to read buffer
			int bytesLeft = 0;
			if(lineStr) bytesLeft = strlen(lineStr);
			bytesRead = bytesLeft;
			// Copy the last part string to the beginning of buffer.
			if (bytesLeft > 0) {
				memcpy(buf, lineStr, bytesLeft);
			}
			// Clear the back part of buffer 
			memset(buf+bytesRead, 0, bufSize+1-bytesRead);
			Sleep(50);
		} while (!parseEnd && proc.IsProcessRunning());

		// Parse final crop string
		char* pStart = strchr(finalCropStr, '=');
		if(pStart) pStart++;
		char* pEnd = strchr(finalCropStr, ' ');
		if(pEnd) *pEnd = 0;
					
		// Now pStart points to value part
		if(pStart) {
			std::vector<int> cropVals;
			char* curNum = pStart;
			while(curNum && *curNum) {
				cropVals.push_back(atoi(curNum));
				char* delim = strchr(curNum, ':');
				if(!delim) break;
				curNum = delim + 1;
			}
			if(cropVals.size() == 4) {
				rect.w = cropVals[0];
				rect.h = cropVals[1];
				rect.x = cropVals[2];
				rect.y = cropVals[3];
			}
		}
		
		proc.Cleanup();
		logger_info(LOGM_UTIL_YUVUTIL, "Detect black band finished:Crop=%d:%d:%d:%d, frame count:%d.\n", 
			rect.w, rect.h, rect.x, rect.y, frameDetectCount);
	} while(false);
	return rect;
}

bool CYuvUtil::Crop(const FrameRect& cropParam, int srcW, int srcH, uint8_t* srcYuv, uint8_t** dstYuv)
{
	return true;
}

bool CYuvUtil::Expand(const FrameRect& cropParam, int srcW, int srcH, uint8_t* srcYuv, uint8_t** dstYuv)
{
	return true;
}

void CYuvUtil::FreeBuffer(uint8_t** pBuffer)
{
	if(pBuffer) {
		free(*pBuffer);
		*pBuffer = NULL;
	}
}


typedef struct AVRational{
    int num; ///< numerator
    int den; ///< denominator
} AVRational;

static int av_reduce(int *dst_nom, int *dst_den, int64_t nom, int64_t den, int64_t max)
{
    AVRational a0={0,1};
	AVRational a1={1,0};
    int sign= (nom<0) ^ (den<0);
    int64_t gcd= GetGcd(_abs64(nom), _abs64(den));

    if(gcd){
        nom = _abs64(nom)/gcd;
        den = _abs64(den)/gcd;
    }
    if(nom<=max && den<=max){
		a1.num = (int)nom;
		a1.den = (int)den;
        den=0;
    }

    while(den){
        /*u*/int64_t x      = nom / den;
        int64_t next_den= nom - den*x;
        int64_t a2n= x*a1.num + a0.num;
        int64_t a2d= x*a1.den + a0.den;

        if(a2n > max || a2d > max){
            if(a1.num != 0) x= (max - a0.num) / a1.num;
            if(a1.den != 0) x= min(x, (max - a0.den) / a1.den);

			if (den*(2*x*a1.den + a0.den) > nom*a1.den) {
				a1.num = (int)(x*a1.num + a0.num);
				a1.den = (int)(x*a1.den + a0.den);
			}
            break;
        }

        a0= a1;
		a1.num = (int)a2n;
		a1.den = (int)a2d;
        nom= den;
        den= next_den;
    }
    if (!(GetGcd(a1.num, a1.den) <= 1U)) return 0;

    *dst_nom = sign ? -a1.num : a1.num;
    *dst_den = a1.den;

    return den==0;
}

static AVRational av_d2q(double d, int max){
    AVRational a = {0, 1};
	const double LOG2VAL = 0.69314718055994530941723212145817656807550013436025;
    int exponent= max( (int)(log(fabs(d) + 1e-20)/LOG2VAL), 0);
    int64_t den= 1LL << (61 - exponent);
    av_reduce(&a.num, &a.den, (int64_t)(d * den + 0.5), den, max);

    return a;
}

void GetFraction(double value, int* num, int* den)
{
	if(fabs(value - 29.97) < 0.01) {
		*num = 30000;
		*den = 1001;
		return;
	} else if (fabs(value - 23.976) < 0.01) {
		*num = 24000;
		*den = 1001;
		return;
	}
	AVRational r = av_d2q(value, 9999);
	*num = r.num;
	*den = r.den;
}

double CAspectRatio::GetPAR(int* par_num, int* par_den)
{
	double par = width ? dar / width * height : 1;
	if (par && par_num && par_den) {
		AVRational r = av_d2q(par, 9999);
		*par_num = r.num;
		*par_den = r.den;
	}
	return par;
}

double CAspectRatio::GetDAR(int* dar_num, int* dar_den)
{
	if (dar && dar_num && dar_den) {
		AVRational r = av_d2q(dar, 9999);
		*dar_num = r.num;
		*dar_den = r.den;
	}
	return dar;
}
