#ifndef VIDEO_ENHANCER
#define VIDEO_ENHANCER


#define LEVEL_SCALE 256
#define HORIZONTAL 1
#define VERTICAL   2
#define DIAG45     3
#define DIAG135    4

#define BLOCK_8x8 3

typedef struct videoEnhancer_parameter{
	unsigned char* org; // 原始视频缓冲区
	int stride_org;     // 

	unsigned char* tmp; // TODO: Delete
	int stride_tmp;
	
	unsigned char* canny; // canny算法结果
	int stride_canny;
	
	unsigned char* sobel; // sobel结果
	int stride_sobel;

	unsigned char* laplacian; // TODO: delete
	int stride_laplacian;
	
	unsigned char* gaussian_blur_buf0; // 高斯算法缓冲区
	int stride_gaussian_blur_buf0;

	unsigned char* gaussian_blur_buf1;
	int stride_gaussian_blur_buf1;

	unsigned char* gaussian_blur_buf2;
	int stride_gaussian_blur_buf2;

	int* direction_label;              // TODO: delete
	int stride_direction_label;

	int width;   // 原始视频宽度
	int height;  // 原始视频高度
	int stride;  // 步长

	float sharpen_Level;
	float color_level;
	int contrast_threshold;
	float contrast_level;
} videoEnhancer_parameter;

struct IE_Data;
class CVideoEnhancer
{
public:
	CVideoEnhancer();
	virtual ~CVideoEnhancer();
    // 初始化
    // 分配内存
    // colorSpace: YV12
	bool init(int width, int height, int stride, int colorSpace);
    // 反初始化
	void fini();
	bool isInit();
	bool isSSE2();
	void getWidth(int& width);
	void getHeight(int& height);
    // 
	//void adujst_progress(unsigned char* pIn, int beginAdjust, int endAdjust);
    // 画质增强处理函数
    // pIn 输入/输出视频数据
	void process(unsigned char* pIn);
	//void process(double aProgress, SubPicDesc &spd);
	void setSharpenLevel(float sharpenLevel);
	void setColorLevel(float colorLevel);
	void setContrastThreshlod(int contrastThreshold);
	void setContrastLevel(float contrastLevel);
private:
    // 可以使用CRT API
	void *align_malloc(int i_size);
	void align_free(void *p);

    // 对比度增强
    void filter_contrast(unsigned char *pIn, const int width, const int height);
    void filter_contrast_stretch(unsigned char *pIn, const int width, const int height);
	void filter_contrast_test(unsigned char *pIn, const int width, const int height);

       // 锐化
    // 原始视频 O 
    // 1. 边缘检测 算法：canny   C
    // 2. 从边缘中去掉黑边与视频的白线
    // 3. 对边缘结果应用高斯模糊 C
    // 4. 原始视频高斯模糊       G
    // 5. (O - G)*C*delta+O
	void filter_unsharp(unsigned char* pIn, const int width, const int height);
    // 
	void filter_canny(unsigned char* pIn, const int width, const int height);
	
	void filter_gaussian_blur(unsigned char* pIn, const int width, const int height, int mem_ID);
	void filter_gaussian_blur_8x8(const int x, const int y, unsigned char* pIn, unsigned char* pOut, const int stride);
	void filter_gaussian_blur_8x8_SSE2(const int x, const int y, unsigned char* pIn, unsigned char* pOut, const int stride);

	int filter_sobel_8x8(const int x, const int y, unsigned char* pIn, unsigned char* pOut, const int stride);
	int filter_sobel_8x8_SSE2(const int x, const int y, unsigned char* pIn, unsigned char* pOut, const int stride);

	void filter_unsharp_8x8(const int x, const int y, unsigned char* pIn, unsigned char* pOut, const int stride);
	void filter_unsharp_8x8_SSE2(const int x, const int y, unsigned char* pIn, unsigned char* pOut, const int stride);
	
	void filter_color(unsigned char *pIn, const int width, const int height);
	
	bool SSE2Check(); 
protected:
	videoEnhancer_parameter* m_parameter;
	bool m_bInit;
	bool m_bSSE2;
    //DWORD m_numberOfProcessors;
//public:
    //IE_Data* m_ieData;
};

#endif
