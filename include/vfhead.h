#ifndef _VIDEO_FILTER_H_
#define _VIDEO_FILTER_H_

typedef enum {
	OPT_FILTER_TYPE = 0,
	OPT_CUBIN_PATH,
	OPT_CUDA_ARRAYS,
	OPT_FRAME_IN_WIDTH,
	OPT_FRAME_IN_HEIGHT,
	OPT_FRAME_OUT_WIDTH,
	OPT_FRAME_OUT_HEIGHT,
	OPT_FRAME_UP_RATIO,
	OPT_GPU_PERCENT,
} CONFIG_OPTIONS;

typedef enum
{
	FT_DeIntQk = 0x1,
	FT_DeInt = 0x2,
	FT_Resample = 0x10,
	FT_ResampleHq = 0x20, 
	FT_Pullup = 0x100,
	FT_DeNoise = 0x1000,
	FT_DeNoiseHq = 0x2000,
	FS_NoCuda = 0x10000,
} FILTER_TYPE;

typedef struct
{
	int				iframes;	// need fill
	int				oframes;	// need fill
	float			ratio;		// need fill
	int				iframeCount;
	int				id;			// need fill
	int				ow, oh;		// need fill
	int				iw, ih;		// need fill
	unsigned int	buf_size_dest;
	unsigned int	buf_size_src;
	unsigned char	*framesBuffer;  // need fill
	unsigned char	*outputBuffer;
	unsigned char   **lastFrame;
	int				steps;
	int				isDone; // don't modify 0 done, 1 not yet, -1 error 
	int				workId;
	int				isAbort; // 1 true, 0 false
	float			plOffset;
} STREAM_DATA;

typedef struct
{
	int				islast;
	unsigned char	*framesBuffer;  // need fill 
} STREAM_DATA_IN;

typedef struct
{
	unsigned char	*oframesBuffer;  // need fill 
	int				islast;
	int				frameCount;
	int				frameSize;
} STREAM_DATA_OUT;

class CVideoFilter 
{
public:
	virtual bool Init(int filterFlags, int threads) = 0;
	virtual bool Prepare(int threadid, int frameWindow, int inWidth, int inHeight, int outWidth, int outHeight, float fpsRatio) = 0;
	virtual void Fill(int threadid, int index, void* data) = 0;
	virtual void* getInputBuffer(int threadid, int index) = 0;
	virtual void* getOutputBuffer(int threadid, int index) = 0;
	virtual void* GetOutputBufferQuick() = 0;
	virtual bool Process(int threadid, int frames, int isLast) = 0;
	virtual bool wait(int threadid, int *oframes) = 0;
	virtual bool fetch(int threadid, int index, void* buffer) = 0;
	//virtual void cleanup(int threadid) = 0;
	virtual void Uninit() = 0;
	virtual char* getInfo() = 0;
protected:
	~CVideoFilter(){}
};

typedef CVideoFilter* (*PFN_vfNew)();
typedef void (*PFN_vfDelete)(CVideoFilter*);

#endif