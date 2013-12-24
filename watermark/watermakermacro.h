#ifndef WATER_MARK_MACRO
#define WATER_MARK_MACRO

#ifdef   WATERMARK_STATIC
	#define  WATERMARK_EXT  
#else
	#ifdef   WATERMARK_EXPORTS
		#define  WATERMARK_EXT   __declspec(dllexport)
	#else
		#define  WATERMARK_EXT   __declspec(dllimport)
	#endif
#endif

#endif

