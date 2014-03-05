#include "videoEnhancer.h"
#include <stdlib.h>
#include <stdio.h>
#ifndef _WIN32
#include <malloc.h>
#endif
#include "string.h"
#include "assert.h"
#include "math.h"
#include "bitconfig.h"

#ifdef HAVE_VIDEO_ENHANCE
CVideoEnhancer::CVideoEnhancer()
{
	m_bInit = false;
    m_bSSE2 = SSE2Check(); 
    m_parameter = NULL;
    //SYSTEM_INFO si;
    //GetSystemInfo(&si);
    //m_numberOfProcessors = si.dwNumberOfProcessors;
   // m_ieData = NULL;
}

CVideoEnhancer::~CVideoEnhancer()
{}

bool CVideoEnhancer::isInit()
{
	return m_bInit;
}

bool CVideoEnhancer::isSSE2()
{
	return m_bSSE2;	
}

void CVideoEnhancer::align_free(void *p)
{
    if (p)
    {
#ifdef _WIN32
        _aligned_free(p);
#else
        free(p);
#endif
    }
}

void * CVideoEnhancer::align_malloc( int i_size )
{
#ifdef _WIN32
    return _aligned_malloc(i_size, 16);
#else
    return memalign(16, i_size);
#endif
}

bool CVideoEnhancer::init(int width, int height,int stride, int colorSpace)
{
	m_parameter = (videoEnhancer_parameter *)align_malloc(sizeof(videoEnhancer_parameter));
	if (NULL==m_parameter)
        return false;
	memset(m_parameter, 0, sizeof(videoEnhancer_parameter));
	
	
	m_parameter->width = width;
	m_parameter->height = height;
	m_parameter->stride = stride;
	
	m_parameter->sharpen_Level = 1.5f;
	m_parameter->color_level = 1.0f;
	m_parameter->contrast_threshold = 180;
	m_parameter->contrast_level = 0.1f;
	int size_luma =  width*height*sizeof(unsigned char)*3/2;

	m_parameter->laplacian = (unsigned char*)align_malloc(size_luma);
	if (NULL == m_parameter->laplacian) return false;	
	memset(m_parameter->laplacian, 0, size_luma);
	m_parameter->stride_laplacian = width;

	m_parameter->canny = (unsigned char*)align_malloc(size_luma);
	if (NULL == m_parameter->canny) return false;		
	memset(m_parameter->canny, 0, size_luma);
	m_parameter->stride_canny = width;

	m_parameter->sobel = (unsigned char*)align_malloc(size_luma);
	if (NULL == m_parameter->sobel) return false;		
	memset(m_parameter->sobel, 0, size_luma);
	m_parameter->stride_sobel = width;
	


	m_parameter->tmp = (unsigned char*)align_malloc(size_luma);
	if (NULL == m_parameter->tmp) return false;
	memset(m_parameter->tmp, 0, size_luma);
	m_parameter->stride_tmp = width;
	
	m_parameter->org = (unsigned char*)align_malloc(size_luma);
	if (NULL == m_parameter->org) return false;
	memset(m_parameter->org, 0, size_luma);
	m_parameter->stride_org = width;
	
	m_parameter->gaussian_blur_buf0 = (unsigned char*)align_malloc(size_luma);
	if (NULL == m_parameter->gaussian_blur_buf0) return false;
	memset(m_parameter->gaussian_blur_buf0, 0, size_luma);
	m_parameter->stride_gaussian_blur_buf0 = width;
	
	m_parameter->gaussian_blur_buf1 = (unsigned char*)align_malloc(size_luma);
	if (NULL == m_parameter->gaussian_blur_buf1) return false;
	memset(m_parameter->gaussian_blur_buf1, 0, size_luma);
	m_parameter->stride_gaussian_blur_buf1 = width;
	
	m_parameter->gaussian_blur_buf2 = (unsigned char*)align_malloc(size_luma);
	if (NULL == m_parameter->gaussian_blur_buf2) return false;
	memset(m_parameter->gaussian_blur_buf2, 0, size_luma);
	m_parameter->stride_gaussian_blur_buf2 = width;
	
	m_parameter->direction_label = (int*)align_malloc(sizeof(int)*width*height);
	if (NULL == m_parameter->direction_label) return false;
	memset(m_parameter->direction_label, 0, sizeof(int)*width*height);	
	m_parameter->stride_direction_label = width;
	
	m_bInit = true;
	return true;
}


void CVideoEnhancer::fini()
{
	m_bInit = false;
	if (m_parameter == NULL)
		return;	

	if (m_parameter->laplacian != NULL)
	{
		align_free(m_parameter->laplacian);
		m_parameter->laplacian = NULL;
	}
	

	if (m_parameter->tmp != NULL)
	{
		align_free(m_parameter->tmp);
		m_parameter->tmp = NULL;
	}

	
	if (m_parameter->canny != NULL)
	{
		align_free(m_parameter->canny);
		m_parameter->canny = NULL;
	}
	
	if (m_parameter->sobel != NULL)
	{
		align_free(m_parameter->sobel);
		m_parameter->sobel = NULL;
	}
	
	if (m_parameter->org != NULL)
	{
		align_free(m_parameter->org);
		m_parameter->org = NULL;
	}

	if (m_parameter->direction_label!= NULL)
	{
		align_free(m_parameter->direction_label);
		m_parameter->direction_label = NULL;
	}
	
	if (m_parameter->gaussian_blur_buf0!= NULL)
	{
		align_free(m_parameter->gaussian_blur_buf0);
		m_parameter->gaussian_blur_buf0 = NULL;
	}
	
	if (m_parameter->gaussian_blur_buf1!= NULL)
	{
		align_free(m_parameter->gaussian_blur_buf1);
		m_parameter->gaussian_blur_buf1 = NULL;
	}

	if (m_parameter->gaussian_blur_buf2!= NULL)
	{
		align_free(m_parameter->gaussian_blur_buf2);
		m_parameter->gaussian_blur_buf2 = NULL;
	}
	
	if (m_parameter!=NULL)
	{
		align_free(m_parameter);
		m_parameter = NULL;
	}
}

void  CVideoEnhancer::getWidth(int& width)
{
	width = m_parameter->width;
}

void CVideoEnhancer::getHeight(int& height)
{
	height = m_parameter->height;
}

// Sobel算子计算边缘
// 
// ----------                ---------- 
// |-1| 0| 1|                |1 | 2| 1| 
// ----------                ---------- 
// |-2| 0| 2|                | 0| 0| 0| 
// ----------                ---------- 
// |-1| 0| 1|                |-1|-2| 1| 
// ----------                ---------- 
#if defined(_WIN32) && !defined(_WIN64)
int CVideoEnhancer::filter_sobel_8x8_SSE2(const int x, const int y, unsigned char* pIn, unsigned char* pOut, const int stride)
{
	int width_8x8 = m_parameter->width >> BLOCK_8x8;
	int height_8x8 = m_parameter->height >> BLOCK_8x8;
	
	if ((x==0)||(x==width_8x8-1)||(y==0)||(y==height_8x8-1))
		return -1;

	int width = 8;
	int height = 8;

	int ox = x<<3;
	int oy = y<<3;

	unsigned char* p1 = pIn + stride*oy + ox;
	unsigned char* p2 = pOut + stride*oy +ox;
	
	int threshold = 30;

	__asm
	{
        mov esi, p1;
        mov edi, p2;
        mov ebx, stride;

        mov ecx, 0x8;
        pxor xmm7,xmm7;
        sub esi, ebx; // 上面的像素
D_loop:
        prefetchnta [esi + ebx - 1];
        prefetchnta [esi + 2*ebx - 1];
        prefetchnta [esi - 1];

        // 右
        movq xmm0, qword ptr [esi + ebx + 1]
        punpcklbw xmm0, xmm7
        psllw xmm0,1;          //*(p1+1)*2

        // 右下
        movq xmm1,qword ptr [esi + 2*ebx + 1]
        punpcklbw xmm1, xmm7 // xmm1: *(p1+stride+1)
        paddw xmm0, xmm1 //*(p1+stride+1)+*(p1+1)*2

        // 右上
        movq xmm2, qword ptr [esi + 1]
        punpcklbw xmm2, xmm7
        paddw xmm0, xmm2; //*(p1+stride+1)+*(p1+1)*2+*(p1-stride+1)

        // 左下
        movq xmm3,qword ptr[esi + 2*ebx -1]
		punpcklbw xmm3, xmm7
		psubw xmm0, xmm3; //*(p1+stride+1)+*(p1+1)*2+*(p1-stride+1)
			                 //-*(p1+stride-1)

        // 左上
        movq xmm1,qword ptr[esi - 1];
        punpcklbw xmm1, xmm7
        psubd xmm0, xmm1; //*(p1+stride+1)+*(p1+1)*2+*(p1-stride+1)
                          //-*(p1+stride-1)-*(p1-stride-1)

        // 左
        movq xmm2, qword ptr[esi + ebx -1]
        punpcklbw xmm2, xmm7
        psllw xmm2, 1
        psubd xmm0, xmm2; //*(p1+stride+1)+*(p1+1)*2+*(p1-stride+1)
                         //-*(p1+stride-1)-*(p1-stride-1)-*(p1-1)*2

        pxor xmm6,xmm6
        pcmpgtw xmm6, xmm0
        movdqa xmm5, xmm0
        paddw xmm5, xmm6
        pxor xmm5, xmm6 //xmm5 abs_Sx
        movdqa xmm4,xmm0//xmm4:Sx

        // 左上
        movq xmm0, qword ptr[esi - 1]
        punpcklbw xmm0, xmm7;//*(p1-stride-1) 

        // 上
        movq xmm1, qword ptr[esi]
        punpcklbw xmm1, xmm7
        psllw xmm1, 1
        paddw xmm0, xmm1 //*(p1-stride-1)+*(p1-stride)*2

        // 右上
        movq xmm2, qword ptr[esi + 1]
        punpcklbw xmm2, xmm7;
        paddw xmm0, xmm2; //*(p1-stride-1)+*(p1-stride)*2+*(p1-stride+1)

        // 左下
        movq xmm3, qword ptr[esi + 2*ebx -1];
        punpcklbw xmm3, xmm7;
        psubw xmm0, xmm3; //*(p1-stride-1)+*(p1-stride+1)+*(p1-stride)*2
                          //-*(p1+stride-1)

        // 下
		movq xmm1, qword ptr[esi + ebx]
		punpcklbw xmm1, xmm7
		psllw xmm1, 1
		psubw xmm0, xmm1 //*(p1-stride-1)+*(p1-stride+1)+*(p1-stride)*2
			             //-*(p1+stride-1)-*(p1+stride)*2

        // 右下
		movq xmm2, qword ptr[esi + 2*ebx + 1]
		punpcklbw xmm2, xmm7
		psubw xmm0, xmm2 //*(p1-stride-1)+*(p1-stride+1)+*(p1-stride)*2
                         //-*(p1+stride-1)-*(p1+stride)*2-*(p1+stride+1)

        pxor xmm6,xmm6
        pcmpgtw xmm6, xmm0
        movdqa xmm3, xmm0
        paddw xmm3, xmm6
        pxor xmm3, xmm6 //xmm3 abs_Sy
        movdqa xmm6,xmm0 //xmm6 Sy

        paddw xmm5, xmm3 //xmm5: abs_Sx+abs_Sy

        mov eax, threshold
        movd xmm1, eax
        pshuflw xmm1, xmm1, 0;
        punpcklwd xmm1, xmm1; //xmm1: threshold

        movdqa xmm2, xmm5
        pcmpgtw xmm2,xmm1 //xmm2(absx+absy)>xmm1(threshold)置1否则置0

        mov eax, 0xEB 
        movd xmm1, eax
        pshuflw xmm1, xmm1, 0;
        punpcklwd xmm1, xmm1; //xmm1: 235

        pand xmm1,xmm2

        mov eax, 0xffffffff
        movd xmm3, eax
        pshufd xmm3, xmm3, 0 //xmm3:0xffffffffffffffffff
        pxor xmm2, xmm3 //xmm2取反
		
		mov eax, 0x10 
		movd xmm0, eax
        pshuflw xmm0, xmm0, 0;
        punpcklwd xmm0, xmm0; //xmm1: 16

        pand xmm0, xmm2;
        por xmm0, xmm1;
        packuswb xmm0,xmm0;

        movq qword ptr[edi],xmm0;

        add esi, ebx;
        add edi, ebx;

		dec ecx
		jnz short D_loop
		emms
	}

}

void CVideoEnhancer::filter_unsharp_8x8_SSE2(const int x, const int y, unsigned char* pIn, unsigned char* pOut, const int stride)
{
	int width_8x8 = m_parameter->width >> BLOCK_8x8;
	int height_8x8 = m_parameter->height >> BLOCK_8x8;
	
	if ((x==0)||(x==width_8x8-1)||(y==0)||(y==height_8x8-1))
		return;

	int width = 8;
	int height = 8;

	int ox = x<<3;
	int oy = y<<3;

	unsigned char* p1 = pIn + stride*oy + ox;
	unsigned char* p2 = m_parameter->gaussian_blur_buf1 + stride*oy + ox;
	unsigned char* p3 = m_parameter->gaussian_blur_buf0 + stride*oy + ox;
	unsigned char* p4 = pOut + stride*oy +ox;

	float delta = m_parameter->sharpen_Level;
	__asm
	{
		mov eax, p1
		movd mm0, eax //mm0:pIn
		mov eax, p2  
		movd mm1, eax //mm1:buf1(pIn blur)
		mov eax, p3
		movd mm2, eax //mm2:buf2(m_parameter->canny blur)
		
		mov ebx, stride
		mov ecx, 0x8
D_loop:
		//////////////////////////////////////////////////////////////
		movd eax, mm0
		movd xmm0,[eax]
		pxor xmm7,xmm7
		punpcklbw xmm0, xmm7
		punpcklwd xmm0, xmm7 //pIn
		

		movdqa   xmm5,xmm0

		movd eax, mm1
		movd xmm1,[eax]
		punpcklbw xmm1, xmm7
		punpcklwd xmm1, xmm7 //blur
		
		movd eax, mm2
		movd xmm2,[eax]
		punpcklbw xmm2, xmm7
		punpcklwd xmm2, xmm7 //canny
		
		psubd xmm0, xmm1 //(pIn-blur)

		mov eax, 0xFF
		movd xmm3, eax
		pshufd xmm3, xmm3, 0
		
		
		
		cvtdq2ps xmm0,xmm0 //转化为单精度浮点数
		cvtdq2ps xmm2,xmm2
		cvtdq2ps xmm3,xmm3 //转化为单精度浮点数
		
		mov		eax, delta
		movd	xmm4, eax
		pshufd	xmm4, xmm4,0

		Mulps xmm0, xmm2 //(pIn-blur)*canny
		Mulps xmm0, xmm4 //(pIn-blur)*canny*delta

		Divps xmm0, xmm3 //(pIn-blur)*canny/255
		cvttps2dq xmm0, xmm0 //转为整数
		paddd xmm0, xmm5 //pIn+(pIn-blur)*canny
		

		mov eax, 0xFF
		movd xmm3, eax
		pshufd xmm3, xmm3, 0
		
		cvtdq2ps xmm0,xmm0 //转化为单精度浮点数
		cvtdq2ps xmm3,xmm3 //转化为单精度浮点数
		maxps    xmm0,xmm7
		minps    xmm0,xmm3
		
		cvttps2dq xmm0, xmm0 //转为整数
		
		packuswb xmm0,xmm0	
		packuswb xmm0,xmm0

		movd eax, mm0
		movd [eax],xmm0
	
		add eax, 4
		movd mm0,eax

		movd eax, mm1
		add eax, 4
		movd mm1,eax

		movd eax, mm2
		add eax, 4
		movd mm2,eax
		//////////////////////////////////////////////////////////
		movd eax, mm0
		movd xmm0,[eax]
		punpcklbw xmm0, xmm7
		punpcklwd xmm0, xmm7 //pIn
		
		movdqa   xmm5,xmm0
		
		movd eax, mm1
		movd xmm1,[eax]
		punpcklbw xmm1, xmm7
		punpcklwd xmm1, xmm7 //blur
		
		movd eax, mm2
		movd xmm2,[eax]
		punpcklbw xmm2, xmm7
		punpcklwd xmm2, xmm7 //canny
		
		psubd xmm0, xmm1 //(pIn-blur)

		mov eax, 0xFF
		movd xmm3, eax
		pshufd xmm3, xmm3, 0
		
		cvtdq2ps xmm0,xmm0 //转化为单精度浮点数
		cvtdq2ps xmm2,xmm2
		cvtdq2ps xmm3,xmm3 //转化为单精度浮点数
		
		mov		eax, delta
		movd	xmm4, eax
		pshufd	xmm4, xmm4,0

		Mulps xmm0, xmm2 //(pIn-blur)*canny
		Mulps xmm0, xmm4

		Divps xmm0, xmm3 //(pIn-blur)*canny/255
		cvttps2dq xmm0, xmm0 //转为整数
		paddd xmm0, xmm5 //pIn+(pIn-blur)*canny
		
		mov eax, 0xFF
		movd xmm3, eax
		pshufd xmm3, xmm3, 0

		cvtdq2ps xmm0,xmm0 //转化为单精度浮点数
		cvtdq2ps xmm3,xmm3 //转化为单精度浮点数
		maxps    xmm0,xmm7
		minps    xmm0,xmm3
		
		cvttps2dq xmm0, xmm0 //转为整数

		packuswb xmm0,xmm0	
		packuswb xmm0,xmm0

		movd eax, mm0
		movd [eax],xmm0
		
		sub eax, 4
		add eax, ebx
		movd mm0,eax

		movd eax, mm1
		sub eax, 4
		add eax, ebx
		movd mm1,eax

		movd eax, mm2
		sub eax, 4
		add eax, ebx
		movd mm2,eax
		
		dec ecx
		jnz short D_loop
		emms
	}
}

void CVideoEnhancer::filter_gaussian_blur_8x8_SSE2(const int x, const int y, unsigned char* pIn, unsigned char* pOut,
                                                  const int stride)
{
	int width_8x8 = m_parameter->width >> BLOCK_8x8;
	int height_8x8 = m_parameter->height >> BLOCK_8x8;
	
	if ((x==0)||(x==width_8x8-1)||(y==0)||(y==height_8x8-1))
		return;

	int ox = x<<3;
	int oy = y<<3;
	unsigned char* p1 = pIn + stride*oy + ox;
	unsigned char* p2 = pOut + stride*oy + ox;

	__asm
	{
        mov esi, p1
        mov edi, p2
        mov ebx, stride
        sub esi, ebx;  // 上面的点

        pxor xmm7,xmm7;
        mov ecx, 0x8;
D_loop:
        prefetchnta [esi + ebx - 1];
        prefetchnta [esi + 2*ebx - 1];
        prefetchnta [esi - 1];

        // 中
        movsd xmm0,[esi + ebx]
        punpcklbw xmm0, xmm7 
        psllw xmm0, 2//*p1*4

        // 左
        movsd xmm1,[esi + ebx - 1]
        punpcklbw xmm1, xmm7
        psllw xmm1, 1
        paddw xmm0, xmm1 //中+左

        // 右
        movsd xmm2,[esi + ebx + 1];
        punpcklbw xmm2, xmm7;
        psllw xmm2, 1;
        paddw xmm0, xmm2 //中+左+右

        // 右下
        movsd xmm3,[esi + 2*ebx + 1]
        punpcklbw xmm3, xmm7;
        paddw xmm0, xmm3;

        // 下
        movsd xmm4,[esi + 2*ebx]
        punpcklbw xmm4, xmm7
        psllw xmm4, 1
        paddw xmm0, xmm4;

        // 左下
        movsd xmm5,[esi + 2*ebx - 1]
        punpcklbw xmm5, xmm7
        paddw xmm0, xmm5;

        // 左上
        movsd xmm6,[esi - 1]
        punpcklbw xmm6, xmm7
        paddw xmm0, xmm6;

        // 上
        movsd xmm1,[esi]
        punpcklbw xmm1, xmm7
        psllw xmm1, 1
        paddw xmm0, xmm1;

        // 右上
        movsd xmm2,[esi + 1]
        punpcklbw xmm2, xmm7
        paddw xmm0, xmm2;

        psraw xmm0, 4
        packuswb xmm0, xmm3
        movsd [edi],xmm0

        add esi, ebx;
        add edi, ebx;

        dec ecx
        jnz short D_loop
        emms
    }
}
#endif

void CVideoEnhancer::filter_unsharp_8x8(const int x, const int y, unsigned char* pIn, unsigned char* pOut, const int stride)
{
	int width_8x8 = m_parameter->width >> BLOCK_8x8;
	int height_8x8 = m_parameter->height >> BLOCK_8x8;
	
	if ((x==0)||(x==width_8x8-1)||(y==0)||(y==height_8x8-1))
		return ;

	int ox = x<<3;
	int oy = y<<3;
	
	int width = 8;
	int height = 8;
	unsigned char* p1 = pIn + stride*oy + ox;
	unsigned char* p2 = m_parameter->gaussian_blur_buf1 + stride*oy + ox; // 原始视频经过高斯模糊
	unsigned char* p3 = m_parameter->gaussian_blur_buf0 + stride*oy + ox; // 经过canny算法的边缘值，再经过高斯模糊

	unsigned char* p4 = pOut + stride*oy +ox;
	
	int i,j;
	int pix;
	double delta = 1.2;
	for (j=0; j< height; j++)
	{
		for (i=0; i< width; i++)
		{
			pix = (int)(*p1 + delta*(*p1-*p2)*(*p3)/255);	
			
			if (pix >255)
				*p4 =255;
			else if (pix <0)
				*p4 = 0;
			else
				*p4 = pix;
			p1++;
			p2++;
			p3++;
			p4++;
		}
		p1 += (stride-width);
		p2 += (stride-width);
		p3 += (stride-width);
		p4 += (stride-width);
	}
}





void CVideoEnhancer::filter_unsharp(unsigned char* pIn, const int width, const int height)
{
	filter_canny(pIn, width, height);
	
	int count_row;
	unsigned char* p1;
	int i,j;
	for(j=0; j<height; j++)
	{
		count_row = 0;
		for (i=0; i<width; i++)
		{
			p1 = m_parameter->canny+j*width+i;
			if (*p1 ==235)
				count_row++;
		}

		if (width*0.8 <= count_row)
		{
			for(i=0; i<width; i++)	
			{
				p1 = m_parameter->canny+j*width+i;
				*p1 = 16;
			}
		}
	}

	filter_gaussian_blur(m_parameter->canny, m_parameter->width, m_parameter->height, 0);
	filter_gaussian_blur(pIn, width, height, 1);
	
	int width_8x8 = width >> BLOCK_8x8;
	int height_8x8 =height >> BLOCK_8x8;
	
	for(j=1; j<height_8x8-1; j++)
	{
		for(i=1; i<width_8x8-1; i++)
		{
			//filter_unsharp_8x8(i,j,pIn,pIn,width);
#if defined(_WIN32) && !defined(_WIN64)
			filter_unsharp_8x8_SSE2(i,j,pIn,pIn,width);
#endif
			//filter_sobel_8x8(i,j,m_parameter->gaussian_blur_buf1, m_parameter->canny, width);
			//filter_unsharp_8x8(i,j,m_parameter->gaussian_blur_buf1, m_parameter->canny, width);
		}
	}
}

// Sobel算子计算边缘
// 
// ----------                ---------- 
// |-1| 0| 1|                |1 | 2| 1| 
// ----------                ---------- 
// |-2| 0| 2|                | 0| 0| 0| 
// ----------                ---------- 
// |-1| 0| 1|                |-1|-2| 1| 
// ----------                ---------- 
int CVideoEnhancer::filter_sobel_8x8(const int x, const int y, unsigned char* pIn, unsigned char* pOut, const int stride)
{
	int width_8x8 = m_parameter->width >> BLOCK_8x8;
	int height_8x8 = m_parameter->height >> BLOCK_8x8;
	
	if ((x==0)||(x==width_8x8-1)||(y==0)||(y==height_8x8-1))
		return -1;

	int width = 8;
	int height = 8;
	
	int ox = x<<3;
	int oy = y<<3;
	
	unsigned char* p1 = pIn + stride*oy + ox;
	unsigned char* p2 = pOut + stride*oy +ox;
	
	int i,j;
	int threshold = 30;
	int abs_Sx, abs_Sy;
	int Sx,Sy;
	int pix;
	for (j = 0; j < height; j++)
	{
		for (i = 0; i < width; i++)
		{
			Sx = -(*(p1-1))*2+(*(p1+1))*2
				 -*(p1-stride-1)+*(p1-stride+1)
				 -*(p1+stride-1)+*(p1+stride+1);
	
			Sy = +*(p1-stride-1)+*(p1-stride+1)+(*(p1-stride))*2
				 -*(p1+stride-1)-*(p1+stride+1)-(*(p1+stride))*2;
			
			abs_Sx = abs(Sx);
			abs_Sy = abs(Sy);


			pix = abs_Sx+abs_Sy;

			if (pix>threshold)
				*p2 = 235;
			else
				*p2 = 16;

			p1++; p2++;	
		}
		p1 += (stride-width);
		p2 += (stride-width);
	}
	return 0;
}


void CVideoEnhancer::filter_canny(unsigned char* pIn, const int width, const int height)
{
	int width_8x8 = width >> BLOCK_8x8;
	int height_8x8 =height >> BLOCK_8x8;
	
	int i,j;

	filter_gaussian_blur(pIn, m_parameter->width, m_parameter->height, 1);
	
	for(j=1; j<height_8x8-1; j++)
	{
		for(i=1; i<width_8x8-1; i++)
		{
			//filter_sobel_8x8(i,j,m_parameter->gaussian_blur_buf1, m_parameter->canny, width);
#if defined(_WIN32) && !defined(_WIN64)
			filter_sobel_8x8_SSE2(i,j,m_parameter->gaussian_blur_buf1, m_parameter->canny, width);
#endif
		}
	}
}


bool CVideoEnhancer::SSE2Check()
{
#if defined(_WIN32) && !defined(_WIN64)
	unsigned int flag;
	__asm
	{
		mov eax,1
		cpuid
		and edx, 004000000h
		mov flag, edx
	}
	if (flag!=0)
		return true;
	else 
#endif
		return false;
}



// 高斯模糊
// 
// ----------
// | 1| 2| 1|
// ----------
// | 2| 4| 2|
// ----------
// | 1| 2| 1|
// ----------
void CVideoEnhancer::filter_gaussian_blur_8x8(const int x, const int y, unsigned char* pIn, unsigned char* pOut, const int stride)
{
	int width_8x8 = m_parameter->width >> BLOCK_8x8;
	int height_8x8 = m_parameter->height >> BLOCK_8x8;
	
	if ((x==0)||(x==width_8x8-1)||(y==0)||(y==height_8x8-1))
		return;

	int width = 8;
	int height = 8;
	
	int ox = x<<3;
	int oy = y<<3;
	unsigned char* p1 = pIn + stride*oy + ox;
	unsigned char* p2 = pOut + stride*oy +ox;
	//int avg;
	int i,j;
	for (j = 0; j < height; j++)
	{
		for (i = 0; i < width; i++)
		{
			*p2 = ((*p1)*4+(*(p1-1))*2+(*(p1+1))*2
				  +(*(p1-stride))*2+*(p1-stride-1)+*(p1-stride+1)
				  +(*(p1+stride))*2+*(p1+stride-1)+*(p1+stride+1))>>4;
			p1++; p2++;
		}	
		p1 += (stride-width);
		p2 += (stride-width);
	}
}

void CVideoEnhancer::filter_gaussian_blur(unsigned char* pIn, const int width, const int height, int mem_ID)
{
	int width_8x8 = width >> BLOCK_8x8;
	int height_8x8 = height >> BLOCK_8x8;
	
	unsigned char* pOut;
	switch(mem_ID)
	{
		case 0:
			pOut = m_parameter->gaussian_blur_buf0;
			break;
		case 1:
			pOut = m_parameter->gaussian_blur_buf1;
			break;
		case 2:
			pOut = m_parameter->gaussian_blur_buf2;
			break;
		default:
			pOut =  m_parameter->gaussian_blur_buf0;
			break;
	}

	int i,j;

	for(j=1; j<height_8x8-1; j++)
	{
		for(i=1; i<width_8x8-1; i++)
		{
			#if defined(_WIN32) && !defined(_WIN64)
			filter_gaussian_blur_8x8_SSE2(i,j,pIn, pOut, width);	
			#endif
		}
	}
	

}

/*void CVideoEnhancer::adujst_progress(unsigned char* pIn, int beginAdjust, int endAdjust)
{
	int i,j;
	unsigned char* p1;
	unsigned char* p2;
	
	int width = m_parameter->width;
	int height = m_parameter->height;
	

	for(j = 0; j<height; j++)
	{
		for(i=endAdjust; i<width;  i++)
		{
			p1 = pIn + j*width + i;
			p2 = m_parameter->org + j*width +i;
			*p1 = *p2;
		}
	}
	
	for(j = 0; j<height/2; j++)
	{
		for(i=endAdjust/2; i<width/2;  i++)
		{
			p1 = pIn+width*height + j*width/2 + i;
			p2 = m_parameter->org+width*height + j*width/2 +i;
			*p1 = *p2;
		}
	}
	
	for(j = 0; j<height/2; j++)
	{
		for(i=endAdjust/2; i<width/2;  i++)
		{
			p1 = pIn +width*height*5/4 + j*width/2 + i;
			p2 = m_parameter->org+width*height*5/4 + j*width/2 +i;
			*p1 = *p2;
		}
	}
}
*/

void CVideoEnhancer::setContrastThreshlod(int contrastThreshold)
{
	m_parameter->contrast_threshold = contrastThreshold;
}

void CVideoEnhancer::setContrastLevel(float contrastLevel)
{
	m_parameter->contrast_level = contrastLevel;
}

void CVideoEnhancer::setSharpenLevel(float sharpenLevel)
{
	m_parameter->sharpen_Level = sharpenLevel;
}
	
void CVideoEnhancer::setColorLevel(float colorLevel)
{
	m_parameter->color_level = colorLevel;
}

/*
 * 直方图均衡化
 */
void CVideoEnhancer::filter_contrast(unsigned char *pIn, const int width, const int height)
{
	int Histogram[256] = {0};
	unsigned char* p;
	p = pIn;
	int pix=0;
	int i,j;
	int low = 30;
	int high = 200;
	for (j=0; j<height*height; j++)
	{
			pix = *p++;
			Histogram[pix]++;
	}


	int levelMin1, levelMax1;
	levelMin1 = 0;
	levelMax1 =255;
	int percent1 = int(width*height*5/1000);
	int count_min = 0;
	for (int i = 0; i <= 255; ++i)
	{
		count_min += Histogram[i];
		if (count_min >= percent1)
		{
			levelMin1 = i;
			break;
		}
	}
	
	int count_max = 0;
	for (int i = 255; i >= 0; i--)
	{
		count_max += Histogram[i];
		if (count_max >= percent1)
		{
			levelMax1 = i;
			break;
		}
	}

	if (levelMax1 <= levelMin1)
		return;

	
	float aHistogram[256] = {0.0f};
	float s = 0.12f;
	float sum = 0.0f;
	for (j=0; j<256; j++)
	{
		aHistogram[j] = pow((float)(Histogram[j]+1), s);
		sum += aHistogram[j];
	}
	float pdf[256];
	for (j=0; j<256; j++)
	{
		pdf[j] = aHistogram[j]/sum;
	}
	float cdf[256] = {0.0};
	for (j=0; j<256; j++)
	{
		for (i=0; i<=j; i++)
		{
			cdf[j] += pdf[i];
		}
	}
	int contrast[256];
	for (j=0; j<256;j++)
	{
		
		//contrast[j] = (levelMax1-levelMin1)*cdf[j];
		pix = (int)((levelMax1-levelMin1)*cdf[j]);
		
		
		contrast[j] = pix*255/(levelMax1-levelMin1);
		
	}
	
    unsigned char* p1;
    for(j=0; j<height;j++)
    {
        p1 = pIn + j*width;
        for(i=0;i<width;i++,p1++)
        {
            *p1 = contrast[*p1];
        }
    }
}

/*
 * 对比度增强后，视觉色彩饱和度会有所降低
 * 色彩修正，适度的增加饱和度
 */
void CVideoEnhancer::filter_color(unsigned char *pIn, const int width, const int height)
{
	unsigned char* p1;
	unsigned char* p2;
	unsigned char* p3;
	unsigned char* p4;
	int i,j;

	//float s =1.1;
	//m_parameter->color_level = 1.2;
	int pix;
	for(j=0; j<height/2; j++)
	{
		for(i=0; i<width/2; i++)
		{
			p1 = pIn +j*width*2 +i*2;
			p2 = m_parameter->org + j*width*2 +i*2;
			p3 = pIn + width*height + j*width/2 +i;
			p4 = pIn + width*height*5/4 +j*width/2+i;
			if (*p2 <= 16)
			{
                *p2 = 16;
			}

			pix = (int)(m_parameter->color_level*(*p1)*(*p3-128)/(*p2)+128);
			if (pix>240)
				*p3 = 240;
			else if  (pix<16)
				*p3 = 16;
			else
				*p3 = pix;

			pix = (int)(m_parameter->color_level*(*p1)*(*p4-128)/(*p2)+128);
			if (pix>240)
				*p4 = 240;
			else if  (pix<16)
				*p4 = 16;
			else
				*p4 =pix;
		}
	}
}
void CVideoEnhancer::filter_contrast_stretch(unsigned char *pIn, const int width, const int height)
{
	int i,j;
	int levelMin1 = 0;     // 千分之一最亮的点
	int levelMax1 = LEVEL_SCALE-1;   // 千分之一最暗的点

	int levelMin2 = 0;    // 百分之五最亮的点
	int levelMax2 = LEVEL_SCALE-1;  // 百分之五最暗的点

	long percent1 = long(width*height*1/1000);
	long percent2 = long(width*height*50/1000);
	
	// 计算 最暗的和最亮的点
	int aHistogram[LEVEL_SCALE] = {0};
	
	unsigned char* p;
	p = pIn;
	int pix=0;
	for (j=0; j<height*height; j++)
	{
			pix = *p++;
			aHistogram[pix]++;
	}
	
	int min = 16;
	int max = 235;

	long count = 0;
	for (int i = min; i <= max; ++i)
	{
		count += aHistogram[i];
		if (count >= percent1)
		{
			levelMin1 = i;
		}
		if (count >= percent2)
		{
			levelMin2 = i;
			break;
		}
	}

	count = 0;
	for (int i = max; i >= min; --i)
	{
		count += aHistogram[i];
		if (count >= percent1)
		{
			levelMax1 = i;
		}
		if (count >= percent2)
		{
			levelMax2 = i;
			break;
		}
	}


	// 计算 平均亮度
	int64_t brightness = 0;
	for (int i = 0; i <= min; ++i)
	{
		brightness += aHistogram[i]*min;
	}
	for (int i = min + 1; i < max; ++i)
	{
		brightness += aHistogram[i]*i;
	}
	for (int i = max; i < LEVEL_SCALE; ++i)
	{
		brightness += aHistogram[i]*max;
	}

	unsigned char avgBrightness = (unsigned char)(brightness/(width*height)); // 平均亮度
	
	unsigned char beforeDark = ((levelMin1+levelMin2)/2 + min)/2;
	unsigned char beforeBright = ((levelMax1+levelMax2)/2 + max)/2;
	// 亮度均衡就是把 beforeDark -> aBlack, beforeBright -> aWhite, 其他的值线性变换

	if (avgBrightness <= (min + max)/2)
	{
		beforeDark   -= 2*(avgBrightness - (max + min)/2)/(min + max)*(levelMin2 - beforeDark);
		beforeBright -= 2*((min + max)/2 - avgBrightness)/(min + max)*(beforeBright - levelMax2);
	}
	else
	{
		beforeDark   += 2*(avgBrightness - (min + max)/2)/(max + min)*(levelMin2 - beforeDark);
		beforeBright += 2*((min + max)/2 - avgBrightness)/(max + min)*(beforeBright - levelMax2);
	}

	if (beforeDark < min) { beforeDark = min; }
	if (beforeBright > max) { beforeBright = max; }
	
	//if (beforeDark < aBlack) { beforeDark = aBlack; }
	//if (beforeBright > aWhite) { beforeBright = aWhite; }
	
	if (beforeBright < m_parameter->contrast_threshold) 
	{ 
		beforeBright = m_parameter->contrast_threshold;
	}


    // 加上阀值，防止把视频调整太黑
    if (beforeDark > min + 4)
    {
        beforeDark = min + 4;
    }

	int contrast[256] = {0};
	int diff = beforeBright - beforeDark;
	if (diff ==0) diff = 1;
	for (j=0; j<beforeDark; j++)
		contrast[j] = min;
	for(j=beforeBright+1; j<256;j++)
		contrast[j] = max;
	
	for (j=beforeDark; j<=beforeBright; j++)
	{
		contrast[j] = min + (j - beforeDark)*(max-min)/diff;	
	}
	unsigned char* p1;

	for(j=0; j<height;j++)
	{
		for(i=0;i<width;i++)
		{
			p1 = pIn + j*width +i;
			*p1 = contrast[*p1];
		}
	}
}


void CVideoEnhancer::filter_contrast_test(unsigned char *pIn, const int width, const int height)
{
	int Histogram[256] = {0};
	unsigned char* p;
	p = pIn;
	int pix=0;
	int i,j;
	//int low = 40;
	//int high = 200;
	const int piexls = (int)(width*height*0.6f);
	for (j=0; j<piexls; j++)
	{
			pix = *p++;
			Histogram[pix]++;
	}
	
	/*int count_low = 0;
	for (j=0; j<low; j++)
	{
		count_low +=  Histogram[j];
	}
	float ratio_low = (float)count_low/(width*height);

	int count_high = 0;
	for (j=count_high;j<256;j++)
	{
		count_high+= Histogram[j];	
	}
	float ratio_high =  (float)count_high/(width*height);
	
	float k1 =  ratio_low-ratio_high;
	float k2 = 4*(ratio_low-ratio_high);
	float k3 = 9 +4*(ratio_low-ratio_high);
	float s = (float)1.0/(9+4*(ratio_low-ratio_high));*/

	float aHistogram[256] = {0.0};
	float s = m_parameter->contrast_level;
	float sum = 0.0;
	for (j=0; j<256; j++)
	{
		aHistogram[j] = pow((float)(Histogram[j]+1), s);
		sum += aHistogram[j];
	}
	float pdf[256];
	for (j=0; j<256; j++)
	{
		pdf[j] = aHistogram[j]/sum;
	}
	float cdf[256] = {0.0};
	for (j=0; j<256; j++)
	{
		for (i=0; i<=j; i++)
		{
			cdf[j] += pdf[i];
		}
	}
	int contrast[256];
	for (j=0; j<256;j++)
	{
		contrast[j] = (int)(255*cdf[j]);
	}
	
	unsigned char* p1;
	//unsigned char* p2;
	for(j=0; j<height;j++)
	{
		for(i=0;i<width;i++)
		{
			p1 = pIn + j*width +i;
			*p1 = contrast[*p1];
		}
	}
}

void CVideoEnhancer::process(unsigned char* pIn)
{
	int width,height;
    width = m_parameter->width;
    height = m_parameter->height;
	memcpy(m_parameter->org, pIn, sizeof(unsigned char)*width*height*3/2);
	 
	//filter_contrast_stretch(pIn, width, height);
    filter_contrast_test(pIn, width, height);
	#if defined(_WIN32) && !defined(_WIN64)
    if (m_bSSE2)
    {
        filter_unsharp(pIn, width, height);
    }
	#endif
    filter_color(pIn, width, height);
}


/*BOOL IE_YV12(IE_Data* aData, double aProgress, SubPicDesc& spd);
void CVideoEnhancer::process(double aProgress, SubPicDesc &spd)
{
    int width,height;
    width = m_parameter->width;
    height = m_parameter->height;

    unsigned char* pIn = (unsigned char*)spd.bits;
    // 原始视频缓冲区
    memcpy_accel(m_parameter->org, pIn, sizeof(unsigned char)*width*height*3/2);

    // 直方图均衡化

	filter_contrast(pIn, width, height);
    //if (m_ieData)
    //{
        //IE_YV12(m_ieData, aProgress, spd);
    //}
    // 如果CPU支持SSE2，执行USM
    if (m_bSSE2 && m_numberOfProcessors >= 2)
    {
        filter_unsharp(pIn, width, height);
    }
    filter_color(pIn, width, height);
}
*/
#endif
