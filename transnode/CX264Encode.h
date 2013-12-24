#ifndef _X264_ENCODE_H_
#define _X264_ENCODE_H_

#include "VideoEncode.h"

#include "bit_osdep.h"
#include <stdint.h>

extern "C" {
#include "x264.h"
}

//#define SLEEP_TIME 3
//#define VIDEO_STREAM_FRAMES 16
//#define DEBUG_DUMP_H264

class CStreamOutput;
//struct cli_pulldown_t;

/* init with default values */
class CX264Encode : public CVideoEncoder
{
public:
	CX264Encode(const char* outFileName);
	~CX264Encode();

	bool Initialize();
	int EncodeFrame(uint8_t* pFrameBuf, int bufSize, bool bLast);
	bool Stop();
	int	 GetDelayedFrames();
private:
	CX264Encode();
	bool cleanUp();
	void fill_x264_pic(const uint8_t* frameBuf);
	int  encode_frame(x264_picture_t* pic, int64_t& lastDts);
	void setPreset(int presetId);
	void setTune(int tuneId);
	bool applyProfileRestrict(int profileNum);
	bool applyFastFirstPass(bool bTurbo);

#ifdef DEBUG_DUMP_H264
	CStreamOutput* dump_output;
#endif
	x264_t*			m_pX264Ctx;
	x264_param_t	m_x264Param;
	x264_picture_t	m_x264Pic;
	int				m_planeSize;
	
	int64_t m_lastDts;
	int64_t m_prevDts;
	int64_t m_firstDts;
	int64_t m_largestPts;
	int64_t m_secondLargestPts ;
	int64_t m_ticksPerFrame;

};

#endif
