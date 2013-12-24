#ifndef _X265_ENCODE_H_
#define _X265_ENCODE_H_

#include "VideoEncode.h"

#include "bit_osdep.h"
#include <stdint.h>

#include "x265.h"
#include "BiThread.h"

#define DUMP_YUV 1

class CStreamOutput;
//struct cli_pulldown_t;

/* init with default values */
class CX265Encode : public CVideoEncoder
{
public:
	CX265Encode(const char* outFileName);
	~CX265Encode();

	bool Initialize();
	int EncodeFrame(uint8_t* pFrameBuf, int bufSize, bool bLast);
	bool Stop();
	//int	 GetDelayedFrames();
private:
	CX265Encode();
	bool cleanUp();
	void fill_x265_pic(uint8_t* frameBuf);
	int  encode_frame(x265_picture* pic, int64_t& lastDts);
	void write_nals(x265_nal* pNal, int nal);
	
	x265_encoder*	m_pX265Ctx;
	//x265_nal*       m_pNal;
	x265_param	    m_x265Param;
	x265_picture	m_x265Pic;
	int				m_planeSize;
#ifdef DUMP_YUV
	CStreamOutput* dump_output;
#endif
	//CBIMutex m_nalWriteMutex;
};

#endif
