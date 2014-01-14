#ifndef _TS_DEF_H_
#define _TS_DEF_H_

#include <stdint.h> //for uint32_t

#define MAX_CAPABILITY 255
#define CODEC_NAME_LEN 128
 
typedef enum {
	STATE_DISCONNECTED = -1,
	STATE_CONNECTED = 0,
	STATE_STARTED,
	STATE_SHUTDOWN,
} node_state_t;

typedef enum {
	TASK_NOTSET = -1,
	TASK_READY = 0,
	TASK_ENCODING,
	TASK_ENCODING_PREPARE,
	TASK_MUXING,
	TASK_PAUSED,
	TASK_DONE,
	TASK_REUSABLE,
	TASK_CANCELLED,
	TASK_ERROR,
} task_state_t;

typedef enum {
	MASTER_UNINITED = -1,
	MASTER_INITED = 0,
	MASTER_RUNNING,
	MASTER_STOPPING,
	MASTER_STOPPED,
	MASTER_SHUTDOWN,
} master_state_t;

typedef enum _enum_task_priority {
	TASK_LOWEST,
	TASK_LOW,
	TASK_NORMAL,
	TASK_HIGH,
	TASK_HIGHEST
} TASK_PRIORITY; 

enum {
	QUERY_CMD_NODE_STATE,
	QUERY_CMD_TASK_STATE,
	QUERY_CMD_CHECK_CAP,
	QUERY_CMD_SET_IGNORE_ERRIDX
};

// 任务信息
typedef struct {
	int id;						// 任务ID
	int nodeId;					// 工作节点ID
	int localId;				// 工作节点上任务的本地ID
	float progress;				// 任务进度百分比（0~100）
	int elapsed_time;			// 任务经历的时间
	char speed[16];				// 转码速率
	task_state_t state;			// 任务状态
	task_state_t lastState;		// 上一个任务状态
	int	  error_code;			// 错误代码
	//char dstmd5[32];			// 目标文件Md5
} ts_task_info_t;

typedef struct {
	int error_task_num;			// 错误任务数
	int completed_task_num;		// 完成任务数
	int current_gpu_enable;		// 是否具备gpu编码能力
	int cpu_core_number;		// CPU核个数
	int cpu_usage;				// 内存使用率
	int phymem_size;			// 内存大小 (in KB)
	int phymem_usage;			// 内存使用率
	int run_time;				// Node 运行时间(Unit: sec)
} ts_node_info_t;

typedef struct {
	int up_time;				// 服务器启动之后总共运行的时间（秒）
	int active_streams;			// 目前正在处理的流的数量
	int overall_cpu_load;		// 总体CPU使用率
	int overall_gpu_load;		// 总体GPU使用率
	int request_count;			// 总请求数量
	int request_completed;		// 完成的请求数量
	int request_error;			// 出错的请求数量
} ts_stats_t;

typedef enum {
	ST_VIDEO,
	ST_AUDIO,
	ST_SUBTITLE,
	ST_MUXER
} stream_type_t;

typedef enum {
	VD_BYPASS,
	VD_MPLAYER,
	VD_MENCODER,
	VD_FFMPEG,
	VD_VLC,
	VD_AVS,
	VD_HKV,
	VD_DSHOW,
	VD_COPY
} video_decoder_t;

typedef enum {
	AD_BYPASS,
	AD_MPLAYER,
	AD_MENCODER,
	AD_FFMPEG,
	AD_VLC,
	AD_AVS,
	AD_LAME,
	AD_FLAC,
	AD_COPY
} audio_decoder_t;


typedef enum {
	VE_X264 = 0,
	VE_X265,
	VE_CUDA264,
	VE_JM264,
	VE_XVID,
	VE_THEORA,
	VE_VFW,
	VE_WM,
	VE_MENCODER,
	VE_FFMPEG,
	VE_VC1,
	VE_MII,
	VE_CAVS,
} video_encoder_t;

typedef enum {
	MUX_MP4 = 0,
	MUX_MKV,
	MUX_TSMUXER,
	MUX_MENCODER,
	MUX_FFMPEG,
	MUX_PMP,
	MUX_MP4CREATOR,
	MUX_MGP,
	MUX_VCD,
	MUX_DUMMY,
	MUX_WM
} muxer_type_t;

typedef enum {
	VC_XVID = 0,
	VC_H264,
	VC_HEVC,
	VC_MPEG1,
	VC_MPEG2,
	VC_MPEG4,
	VC_FLV,
	VC_THEORA,
	VC_VC1,
	VC_WMV7,
	VC_WMV8,
	VC_WMV9,
	VC_DIRAC,
	VC_RM,
	VC_H263,
	VC_H263P,
	VC_H261,
	VC_MSMPEG4V2,
	VC_DV,
	VC_MJPEG,
	VC_LJPEG,
	VC_HUFFYUV,
	VC_SNOW,
	VC_AMV,
	VC_MII,
	VC_CAVS,

	// Vfw codec formats
	VC_MATROX_MPEG2I,
	VC_MATROX_MPEG2I_HD,
	VC_MATROX_DVCAM,
	VC_MATROX_DVPRO,
	VC_MATROX_DVPRO50,
	VC_CANOPUS_DV,

	VC_RAW,
	VC_COUNT,
} video_format_t;

typedef enum {
	CF_DEFAULT,
	CF_RAW,
	CF_AVI,
	CF_MP4,
	CF_MKV,
	CF_MPEG1,
	CF_MPEG2,
	CF_MPEG2TS,
	CF_M2TS,
	CF_FLV,
	CF_F4V,
	CF_ASF,
	CF_3GP,
	CF_3GP2,
	CF_MOV,
	CF_OGG,
	CF_MJPEG,
	CF_RM,
	CF_DV,
	CF_NUT,
	CF_PMP,
	CF_RTP,
	CF_UDP,
	CF_COUNT,
} container_format_t;

typedef enum {
	RAW_I420,
	RAW_RGB24
} rawvideo_format_t;

typedef enum {
	H264_AUTO,
	H264_BASE_LINE,
	H264_MAIN ,
	H264_HIGH 
} h264_profile_t;

typedef enum {
	XVID_PROFILE_NONE = 0,	/* unstrained */
	XVID_PROFILE_S_L0 ,		/* simple */
	XVID_PROFILE_S_L1 ,
	XVID_PROFILE_S_L2 ,
	XVID_PROFILE_S_L3 ,
	XVID_PROFILE_ARTS_L1 ,	/* advanced real time simple */
	XVID_PROFILE_ARTS_L2 ,
	XVID_PROFILE_ARTS_L3,
	XVID_PROFILE_ARTS_L4 ,
	XVID_PROFILE_AS_L0 ,	/* advanced simple */
	XVID_PROFILE_AS_L1 ,
	XVID_PROFILE_AS_L2 ,
	XVID_PROFILE_AS_L3 ,
	XVID_PROFILE_AS_L4 
} xvid_profile_t;

typedef enum {
	VF_NONE,
	VF_DECODER,		// For mplayer/mencoder vf
	VF_ENCODER,
	VF_AUTO,
	VF_CPU,
	VF_CUDA,
} vf_type_t;

typedef enum {
	AF_NONE,
	AF_DECODER,		// For mplayer/mencoder af
	AF_ENCODER,
	AF_CPU
} af_type_t;

typedef enum {
	AE_MP3 = 0,
	AE_VORBIS,
	AE_NEROREF,
	AE_FAAC,
	AE_CTAAC,
	AE_3GPPAAC,
	AE_WMA,
	AE_HMP3,
	AE_FIISMP3,
	AE_MPC,
	AE_SPEEX,
	AE_AMR,
	AE_FFMPEG,
	AE_DOLBY,
	AE_MENCODER,
	AE_CAVSP10,
} audio_encoder_t;

typedef enum {
	AC_DEFAULT = 0,
	AC_MP3,
	AC_VORBIS,
	AC_AAC_LC,
	AC_AAC_HE,
	AC_AAC_HEV2,
	AC_WMA7,
	AC_WMA8,
	AC_WMA9,
	AC_MP2,
	AC_AC3,
	AC_EAC3,
	AC_MPC,
	AC_SPEEX,
	AC_AMR,
	AC_FLAC,
	AC_APE,
	AC_CAVSP10,
	AC_WAVEPACK,
	AC_ALAC,
	AC_PCM,
	AC_ADPCM,
	AC_DTS,
} audio_format_t;

typedef enum {
	VID_PROGRESSIVE = 0,
	VID_INTERLACE_TFF,
	VID_INTERLACE_BFF,
	VID_INTERLACE_MBAFF,
} scan_type_t;

//typedef struct {
//	video_encoder_t video_enc;	// 视频编码器
//	int rate_mode;				// 码率控制模式
//	int video_bitrate;			// 视频码率
//	int video_peakrate;			// 视频峰值码率
//	int width;					// 画面宽度（像素）
//	int height;					// 画面高度（像素）
//	int fps_num;				// 帧率（分子）
//	int fps_den;				// 帧率（分母）
//	audio_encoder_t audio_enc;	// 音频编码格式
//	int audio_bitrate;			// 音频码率
//	int sample_rate;			// 采样率
//	int channels;				// 声道数
//	int	disableAudio;
//	int	disableVideo;
//	char* prefXml;				// additional setting
//} ts_task_opts_t;

typedef struct {
	int cb_size;
	int format;
	int width;
	int height;
	int interlace;
	int fps_num;
	int fps_den;
	int pixelaspect_num;
	int pixelaspect_den;
	int frameCount;
} yuv_info_t;

typedef struct
{
	uint32_t riff;
	uint32_t file_length;
	uint32_t wave;
	uint32_t fmt;
	uint32_t fmt_length;
	uint16_t fmt_tag;
	uint16_t channels;
	uint32_t sample_rate;
	uint32_t bytes_per_second;
	uint16_t block_align;
	uint16_t bits;
	uint32_t data;
	uint32_t data_length;
} wav_info_t;

typedef struct {
	int num;
	int den;
} fraction_t;

typedef struct {
	int width;
	int height;
} resolution_t;

typedef enum {
	RC_MODE_ABR,		// Another variable bitrate method, only for lame mpe encode
	RC_MODE_VBR,
	RC_MODE_CBR,
	RC_MODE_2PASS,
	RC_MODE_3PASS
} rate_ctrl_mode;

typedef struct {
	char container[CODEC_NAME_LEN];
	int duration;
	int bitrate;
} attr_stream_t;

typedef struct {
	int id;
	char codec[CODEC_NAME_LEN];		// audio codec name string
	char profile[CODEC_NAME_LEN];	// audio codec profile
	char lang[32];					// audio language
	int duration;
	int bitrate;					// audio bit rate
	int bitratemode;				// bit rate mode (1 for CBR, 2 for VBR, 0 for unknown)
	int samplerate;					// sampling rate
	int channels;					// number of channels
	int samplebits;					// bits of samples
	int blockalign;					// block alginment
	int64_t samples;				// number of samples
	int delay;
	int index;						// audio stream index
} attr_audio_t;

typedef struct {
	int id;
	char fileFormat[CODEC_NAME_LEN];		// Video file format(TS/AVI)
	char codec[CODEC_NAME_LEN];				// video codec name string
	char profile[CODEC_NAME_LEN];			// video codec profile string
	char version[CODEC_NAME_LEN];			// video codec version
	char standard[32];						// video standard (PAL/NTSC/Component)
	int duration;
	int bitrate;					// video bit rate
	int width;						// video width in pixels
	int height;						// video height in pixels
	int fps_num;					// video frame rate
	int fps_den;					// video frame rate deniminator
	int dar_num;					// DAR
	int dar_den;					// DAR deniminator
	int bits_per_sample;			// bits per sample
	int interlaced;					// 0: progressive, 1: TFF, 2: BFF
	int is_vfr;						// 0: constant fps, 1: variable fps
	float originFps;				// original FPS, get from video stream, may not same with dar_num/dar_den
	int has_origin_res;				// If has original resolution (mov decoder select)
} attr_video_t;

typedef struct {
	container_format_t src_container;
	video_encoder_t encoder_type;
	video_format_t format;
	vf_type_t	vfType;
	resolution_t res_out;
	resolution_t res_in;
	fraction_t fps_out;
	fraction_t fps_in;
	fraction_t dest_par;
	fraction_t src_dar;
	fraction_t dest_dar;
	int interlaced;
	int duration;					// ms
	rawvideo_format_t raw_fromat;   // I420 or RGB
	int bits;						// I420:12, RGB:24
	int bitrate;					// Kbit
	int is_vfr;						// 0: constant fps, 1: variable fps
	int index;
} video_info_t;

typedef struct {
	audio_encoder_t encoder_type;
	audio_format_t format;
	audio_format_t srcformat;
	af_type_t	afType;
	int in_srate;		// In sample rate
	int out_srate;		// Out sample rate
	int in_channels;
	int out_channels;
	int bits;
	int in_sample_bytes;
	int in_bytes_per_sec;
	int aid;
	int delay;
	float volGain;
	int index;
	int duration;		// ms
	char lang[32];
} audio_info_t;

typedef enum {
	EC_NO_ERROR = 0,			// no error
	EC_NOT_INITED = 1,				// not initialized
	EC_INIT_ERROR = 2,				// initialization error
	EC_START_ERROR = 3,				// worker start error
	EC_CREATE_THREAD_ERROR = 4,		// thread create failed
	EC_DECODER_ERROR = 5,			// decoder error
	EC_AUDIO_INIT_ERROR = 6,		// audio init erro
	EC_VIDEO_INIT_ERROR = 7,		// video init erro
	EC_INVALID_PARAM = 8,			// invalid params
	EC_INVALID_PREFS = 9,			// invalid preset loaded
	EC_INVALID_LANG_FILE = 10,		// invalid language file loaded
	EC_DIR_IO_ERROR = 11,			// directory I/O access error
	EC_FILE_IO_ERROR = 12,			// file I/O access error
	EC_INVALID_SETTINGS = 13,		// invalid combination of settings
	EC_INVALID_AUDIO_FORMAT = 14,	// invalid audio format
	EC_INVALID_VIDEO_FORMAT = 15,	// invalid video format
	EC_INVALID_CONTAINER = 16,		// invalid container format
	EC_AUDIO_SOURCE_ERROR = 17,		// an audio source error occurred
	EC_AUDIO_ENCODER_ERROR = 18,		// an audio encoder error occurred
	EC_VIDEO_SOURCE_ERROR = 19,		// an video source error occurred
	EC_VIDEO_ENCODER_ERROR = 20,		// an video encoder error occurred
	EC_MUXER_ERROR = 21, 			// an muxer error occurred
	EC_PREF_KEY_NOT_FOUND = 22,		// pref key not found
	EC_TAG_WRITE_ERROR = 23,			// tag writing error
	EC_NODE_DOWN = 24,				// a node has broken down
	EC_XMLRPC_SOCKET_ERROR = 25,		// xmlrpc socket timeout or other error 
	EC_XMLRPC_INVALID_PAYLOAD = 26,  // xmlrpc: invaild payload
	EC_XMLRPC_OTHER_ERROR = 27,		 // other xmlrpc error, TODO: breaking down is needed
	EC_AUDIO_ENCODER_ABNORMAL_EXIT = 28,	// audio encoder exit unexpectedly
	EC_VIDEO_ENCODER_ABNORMAL_EXIT = 29,	// video encoder exit unexpectedly
	EC_INVILID_AUDIO_ATTRIB = 30,			// Invalide audio attribute
	EC_INVILID_VIDEO_ATTRIB = 31,			// Invalide video attribute
	EC_DECODER_ABNORMAL_EXIT = 32,			// decoder exit unexpectedly
	EC_AV_DURATION_BIG_DIFF = 33,			// a/v duration differs more than 20s.
	EC_ENCODER_NOT_EXIT = 34,			// CLI Encoder is forced to shut down.
	EC_NOT_ENOUGH_DISK_SPACE = 35,		// Disk space is not enough.
	EC_BAD_AV_DATA = 36,				// Outpu mp4 audio/video data corrupt(No frame/audio element 0.0)
	EC_GEN_THUMBNAIL_ERROR = 37,		// Generate ipk file failed.
} error_code_t;

typedef enum {
	WORKER_TYPE_DEFAULT,
	WORKER_TYPE_SEP,
	WORKER_TYPE_UNIFY,
	WORKER_TYPE_REMOTE,
} worker_type_t;

typedef enum {
	AJE_SUCCESS = 0,
	AJE_INVALID_PREF = -1,
	AJE_CREATE_WORKER_FAIL = -2,
	AJE_INIT_WORKER_FAIL = -3,
	AJE_START_WORKER_FAIL = -4,
} assign_job_error;

#endif
