#ifndef __LOGGER_H__
#define __LOGGER_H__

// defined in mplayer.c and mencoder.c
//extern int verbose;

// verbosity elevel:

/* Only messages level LOGL_FATAL-LOGL_STATUS should be translated,
 * messages level LOGL_V and above should not be translated. */

#define LOGL_FATAL 0  // will exit/abort
#define LOGL_ERR 1    // continues
#define LOGL_WARN 2   // only warning
#define LOGL_HINT 3   // short help message
#define LOGL_INFO 4   // info
#define LOGL_STATUS 5 // verbose
#define LOGL_VERBOSE  LOGL_STATUS
#define LOGL_DEBUG 6
#define LOGL_V        LOGL_DEBUG
#define LOGL_DBG1 7
#define LOGL_DBG2 8
#define LOGL_DBG3 9
#define LOGL_MAX      LOGL_DBG3
#define LOGL_DEFAULT  LOGL_STATUS

#define LOGL_FIXME 1  // for conversions from printf where the appropriate MSGL is not known; set equal to ERR for obtrusiveness
#define LOGM_FIXME 0  // for conversions from printf where the appropriate MSGT is not known; set equal to GLOBAL for obtrusiveness

typedef struct log_module_struct {

} log_module;
// code/module:

enum {
// Transever
	LOGM_GLOBAL = 0,	// common player stuff errors
	LOGM_APT,			// Adapter
	LOGM_TS,			// Transcoder
	LOGM_TS_WM,			// Transcoder WorkManager
	LOGM_TS_WK,			// Transcoder Worker (base class)
	LOGM_TS_WK_SEP,     // Transcoder seperate worker
	LOGM_TS_WK_UNI,     // Transcoder uniform worker
	LOGM_TS_FQ,			// Worker File Queue
	LOGM_TS_VD,
	LOGM_TS_VD_MPLAYER,
	LOGM_TS_VD_MENCODER,
	LOGM_TS_VD_AVS,
    LOGM_TS_E_CLI,      // Transcoder CLI Encoder
	LOGM_TS_VE,			// Transcoder Video Encoder
	LOGM_TS_VE_H264,	// Transcoder h264 video encoder
	LOGM_TS_VE_H265,	// Transcoder h265 video encoder
	LOGM_TS_VE_XVID,	// Transcoder Xvid video encoder
	LOGM_TS_VE_CUDA264,	// Transcoder CUDA264 video encoder
	LOGM_TS_VE_FFMPEG,	// Transcoder FFMPEG video encoder
	LOGM_TS_VE_WM,      // Transcoder WM video encoder
	LOGM_TS_VE_VC1,     // Transcoder VC-1 video encoder
	LOGM_TS_VE_MII,		// Mii encoder of MTK
	LOGM_TS_VE_COPY,    // Transcoder WM video encoder
	LOGM_TS_AE,			// Transcoder Audio Encoder
	LOGM_TS_AE_MP3,		// Transcoder Lame mp3 encoder
	LOGM_TS_AE_FAAC,	// Transcoder Faac encoder
	LOGM_TS_AE_FDK,		// Transcoder FDK aac encoder
	LOGM_TS_AE_EAC3,	// Dolby EAC-3 encoder
	LOGM_TS_AE_FFMPEG,	// Transcoder FFMPEG audio encoder
	LOGM_TS_AE_WM,		// Transcoder WM audio encoder
	LOGM_TS_AE_COPY,		// Transcoder audio copier
	LOGM_TS_AVENC,			// AV encoder
	LOGM_TS_AVENC_FFMPEG,   // AV encoder ffmpeg
	LOGM_TS_SO,			// Transcoder Stream output
	LOGM_TS_MUX,		// Transcoder muxer
	LOGM_TS_HTTPD,		// Transcoder Httpd
	LOGM_VF_CUDA,		// VF_CUDA
	LOGM_VF_CPU,		// VF_CPU
	LOGM_AF_RESAMPLE,   // Audio Resampler
	LOGM_REQ,			// Request handler
	LOGM_PROCWRAP,		//Processwrapper
	LOGM_WATERMARK,     // Water mark
	
//tsmaster
	LOGM_MASTER_TASK,
	LOGM_MASTER_TASKQ,
	LOGM_MASTER_NODE,
	LOGM_MASTER_NODEQ,
	LOMG_MASTER_WATCHFOLDER,
	LOMG_MASTER_WATCHFOLDERQ,
	LOGM_MASTER_MASTER,
	LOGM_MASTER_MAIN,
	LOGM_MASTER_USERS,
	LOGM_MASTER_UIHANDLER,

//common
	LOGM_UTIL_SOCKET,
	LOGM_UTIL_FTP,
	LOGM_UTIL_DISCOVERER,
	LOGM_UTIL_SYSPERFINFO,
	LOGM_UTIL_STREAMPREFS,
	LOGM_UTIL_ROOTPREFS,
	LOGM_UTIL_PLAYLIST_GEN,
	LOGM_UTIL_SENDMAIL,
	LOGM_UTIL_UTIL,
// Database saving
	LOGM_DATA_SAVER,
//streaming
	LOGM_STREAMING_HTTP,
	LOGM_UTIL_YUVUTIL,
//END
	LOGM_MAX,			// MAX
};

extern "C" {
void logger_init(const char *log_file);
void logger_uninit(void);
void logger_set_verbose(int lev);
void logger_set_mod_verbose(int mod, int lev);
int  logger_test(int mod, int lev);

#ifndef WIN32
void logger_set_coloring(int color);
#endif

#ifdef __GNUC__
void logger_log(int mod, int lev, const char *format, ... ) __attribute__ ((format (printf, 3, 4)));
#   ifdef _DEBUG
#      define logger_dbg(mod,lev, args... ) logger_log(mod, lev, ## args )
#   else
#      define logger_dbg(mod,lev, args... ) /* only useful for developers */
#   endif
#   define logger_fatal(mod, args... ) logger_log(mod, LOGL_FATAL, ## args )
#   define logger_err(mod, args... ) logger_log(mod, LOGL_ERR, ## args )
#   define logger_warn(mod, args... ) logger_log(mod, LOGL_WARN, ## args )
#   define logger_hint(mod, args... ) logger_log(mod, LOGL_HINT, ## args )
#   define logger_info(mod, args... ) logger_log(mod, LOGL_INFO, ## args )
#   define logger_status(mod, args... ) logger_log(mod, LOGL_STATUS, ## args )
#else // not GNU C
void logger_log(int mod, int lev, const char *format, ... );
#   ifdef _DEBUG
#      define logger_dbg(mod,lev, ... ) logger_log(mod, lev, __VA_ARGS__)
#   else
#      define logger_dbg(mod,lev, ... ) /* only useful for developers */
#   endif

#   define logger_fatal(mod, ... ) logger_log(mod, LOGL_FATAL, __VA_ARGS__)
#   define logger_err(mod, ... ) logger_log(mod, LOGL_ERR, __VA_ARGS__)
#   define logger_warn(mod, ... ) logger_log(mod, LOGL_WARN, __VA_ARGS__)
#   define logger_hint(mod, ... ) logger_log(mod, LOGL_HINT, __VA_ARGS__)
#   define logger_info(mod, ... ) logger_log(mod, LOGL_INFO, __VA_ARGS__)
#   define logger_status(mod, ... ) logger_log(mod, LOGL_STATUS, __VA_ARGS__)

#endif /* __GNUC__ */
}
#endif /* __LOGGER_H__ */
