/*****************************************************************************
 * pptv_level.h: defination for preset level
 *****************************************************************************
 * Copyright (C) 2015 www.pptv.com
 *
 * Authors: zjfeng <jianfengzheng@pptv.com>
 *****************************************************************************/

#ifndef __PPTV_LEVEL_H__
#define __PPTV_LEVEL_H__

typedef enum {
	PPTV_LEVEL_SD	= 0,
	PPTV_LEVEL_HD,
	PPTV_LEVEL_FHD,
	PPTV_LEVEL_BD,
	PPTV_LEVEL_ORI,
	PPTV_LEVEL_4K,
	PPTV_LEVEL_MAX,
};

typedef struct _pptv_level_def {
    int w_max;
    int h_max;                  /* max height for 16:9 */
    int h_max_43;               /* max height for  4:3 */
	int vbr;					/* default video bitrate */
	int vbr_int_for_lowbr;		
	int abr;					/* default audio bitrate */
	int abr_inc_for_music;
}ppl_def_t;


static const ppl_def_t ppl_def[] = 
{
	/*SD */	{  480,  360,  480,  290,  +210,	48,	  0  	},
	/*HD */	{  720,  408,  540,  850,  +650,	64,	 +32	},
	/*FHD*/	{ 1280,  720,  960, 1800,  +1200,   64,	 +64  	},
	/*BD */	{ 1920, 1088, 1440, 5000,   0,	    64,   0  	},
	/*ORI*/	{ 1920, 1088, 1440, 6000,   0,		192,  0 	},
	/*4k */	{ 3840, 2176, 2880, 3800,   0,		64,   0 	},
};

#endif // __PPTV_LEVEL_H__
