#include <string.h>
#include <sstream>

#include "clihelper.h"
#include "logger.h"
#include "tsdef.h"
#include "MEvaluater.h"
#include "util.h"
#include "MediaTools.h"
#include "bit_osdep.h"
#include "StrPro/charset.h"
#include "util.h"
#include "yuvUtil.h"
#include "bitconfig.h"
#include "svnver.h"
#include "pptv_level.h"

#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>
#endif

/////////////////////////////////////////////////////////////////////////////////////////
//<node key=\"audiofilter.resample.downSamplingOnly\">false</node>\n
//<node key=\"videoenc.x264.showInfo\">true</node>\n
//<node key=\"videofilter.denoise.mode\">3</node>\n\
//<node key=\"videofilter.denoise.mode\">3</node>\n\
//		<node key=\"videofilter.denoise.luma\">2</node>\n\
//		<node key=\"videofilter.denoise.chroma\">2</node>\n\
//		<node key=\"videofilter.denoise.strength\">4</node>\n
//<node key=\"videoenc.x265.keyint\">250</node>\n\
//		<node key=\"videoenc.x265.sao\">true</node>\n\
//		<node key=\"videoenc.x265.amp\">true</node>\n\
//<node key=\"videoenc.x265.reframes\">2</node>\n\
//		<node key=\"videoenc.x265.bframes\">4</node>\n\
//<node key=\"videoenc.x265.merange\">48</node>\n\
//	    <node key=\"videoenc.x265.subme\">3</node>\n\
//<node key=\"videoenc.x265.tskip\">true</node>\n\
//		<node key=\"videoenc.x265.tskipFast\">false</node>\n\
//		<node key=\"videoenc.x265.signHide\">true</node>\n\
//		<node key=\"videoenc.x265.badapt\">1</node>\n\
//		<node key=\"videoenc.x265.weightp\">false</node>\n\
//		<node key=\"videoenc.x265.weightb\">false</node>\n\
//		<node key=\"videoenc.x265.wpp\">true</node>\n\
//		<node key=\"videoenc.x265.ctu\">64</node>\n\
//<node key=\"videoenc.x265.frameThreads\">5</node>\n\
//		<node key=\"videoenc.x265.psnr\">false</node>\n\
//		<node key=\"videoenc.x265.loopFilter\">true</node>\n\
//<node key=\"videoenc.x265.preset\">fast</node>\n\
//		<node key=\"videoenc.x265.bpyramid\">true</node>\n\

const char *prefsTemplate = 
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
<root>\n\
<input></input>\n\
<output>\n\
<stream type=\"video\" id=\"1\" pid=\"1\"/>\n\
<stream type=\"audio\" id=\"1\" pid=\"1\"/>\n\
<stream type=\"muxer\" aid=\"1\" vid=\"1\" pid=\"1\"/>\n\
<presets>\n\
    <MediaCoderPrefs name=\"PPTV-Mp4-Template\" id=\"1\">\n\
		<node key=\"overall.task.alignAVData\">true</node>\n\
		<node key=\"overall.audio.enabled\">true</node>\n\
		<node key=\"overall.audio.format\">LC-AAC</node>\n\
		<node key=\"overall.audio.encoder\">FAAC</node>\n\
		<node key=\"overall.audio.channels\">3</node>\n\
		<node key=\"overall.audio.insertBlank\">true</node>\n\
		<node key=\"audiofilter.resample.samplerate\">44100</node>\n\
		<node key=\"audiofilter.extra.brdown\">true</node>\n\
		<node key=\"audiofilter.resample.downSamplingOnly\">false</node>\n\
		<node key=\"overall.audio.copy\">false</node>\n\
		<node key=\"audioenc.ffmpeg.bitrate\">48</node>\n\
		<node key=\"audioenc.faac.bitrate\">48</node>\n\
		<node key=\"audioenc.faac.mode\">1</node>\n\
		<node key=\"audioenc.nero.mode\">2</node>\n\
	    <node key=\"overall.video.enabled\">true</node>\n\
	    <node key=\"overall.video.encoder\">x264</node>\n\
	    <node key=\"videofilter.scale.enabled\">true</node>\n\
	    <node key=\"videofilter.scale.width\">320</node>\n\
	    <node key=\"videofilter.scale.height\">240</node>\n\
	    <node key=\"videofilter.scale.algorithm\">Bilinear</node>\n\
	    <node key=\"videofilter.deint.mode\">0</node>\n\
	    <node key=\"videofilter.deint.algorithm\">YADIF</node>\n\
        <node key=\"videofilter.frame.enabled\">false</node>\n\
        <node key=\"videofilter.frame.fps\">25</node>\n\
        <node key=\"videofilter.frame.fpsScale\">1</node>\n\
		<node key=\"videofilter.extra.brdown\">false</node>\n\
        <node key=\"overall.video.mode\">3</node>\n\
        <node key=\"overall.video.bitrate\">320</node>\n\
		<node key=\"overall.video.quality\">62</node>\n\
        <node key=\"overall.video.format\">H.264</node>\n\
        <node key=\"overall.video.copy\">false</node>\n\
        <node key=\"videofilter.crop.mode\">3</node>\n\
        <node key=\"videoenc.x264.profile\">3</node>\n\
        <node key=\"videoenc.x264.level\">0</node>\n\
        <node key=\"videoenc.x264.preset\">Custom</node>\n\
	    <node key=\"videoenc.x264.weight_p\">2</node>\n\
        <node key=\"videoenc.x264.bframes\">4</node>\n\
	    <node key=\"videoenc.x264.b_adapt\">2</node>\n\
	    <node key=\"videoenc.x264.weight_b\">true</node>\n\
	    <node key=\"videoenc.x264.b_pyramid\">2</node>\n\
	    <node key=\"videoenc.x264.frameref\">4</node>\n\
	    <node key=\"videoenc.x264.ratetol\">1</node>\n\
	    <node key=\"videoenc.x264.me\">2</node>\n\
	    <node key=\"videoenc.x264.me_range\">24</node>\n\
	    <node key=\"videoenc.x264.subme\">9</node>\n\
	    <node key=\"videoenc.x264.fast_pskip\">false</node>\n\
	    <node key=\"videoenc.x264.dct_decimate\">true</node>\n\
	    <node key=\"videoenc.x264.mixed_refs\">true</node>\n\
	    <node key=\"videoenc.x264.rc_lookahead\">60</node>\n\
	    <node key=\"videoenc.x264.direct_pred\">Auto</node>\n\
	    <node key=\"videoenc.x264.partitions\">All</node>\n\
	    <node key=\"videoenc.x264.trellis\">2</node>\n\
	    <node key=\"videoenc.x264.psy_rd\">0.0</node>\n\
	    <node key=\"videoenc.x264.psy_trellis\">0.0</node>\n\
	    <node key=\"videoenc.x264.showInfo\">2</node>\n\
	    <node key=\"videoenc.x264.qpmin\">0</node>\n\
		<node key=\"videoenc.x264.keyint\">0</node>\n\
		<node key=\"videoenc.x264.keyint_min\">0</node>\n\
		<node key=\"videoenc.x265.reframes\">3</node>\n\
	    <node key=\"videoenc.x265.me\">3</node>\n\
		<node key=\"videoenc.x265.subme\">2</node>\n\
		<node key=\"videoenc.x265.rdLevel\">4</node>\n\
		<node key=\"videoenc.x265.openGop\">false</node>\n\
		<node key=\"videoenc.x265.tskip\">false</node>\n\
		<node key=\"videoenc.x265.ctuIntra\">1</node>\n\
		<node key=\"videoenc.x265.ctuInter\">1</node>\n\
		<node key=\"videoenc.x265.maxMerge\">5</node>\n\
		<node key=\"videoenc.x265.weightp\">false</node>\n\
		<node key=\"videoenc.x265.badapt\">2</node>\n\
		<node key=\"videoenc.x265.preset\">slow</node>\n\
		<node key=\"videoenc.x265.rect\">false</node>\n\
		<node key=\"videoenc.x265.cuTree\">true</node>\n\
		<node key=\"videoenc.x265.amp\">true</node>\n\
		<node key=\"videoenc.x265.aqMode\">1</node>\n\
		  <node key=\"overall.dolby.sixchOnly\">false</node>\n\
		  <node key=\"overall.container.format\">3GP</node>\n\
		  <node key=\"overall.container.muxer\">MP4Box</node>\n\
		  <config key=\"video.streams\">all</config>\n\
		  <config key=\"audio.streams\">all</config>\n\
		  <node key=\"overall.task.destdir\">OUTPUT</node>\n\
		  <node key=\"overall.task.decoderTrim\">true</node>\n\
       </MediaCoderPrefs>\n\
    </presets>\n\
  </output>\n\
</root>";

// IPTV ts template
const char *iptvTemplate = 
	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
	<root>\n\
	<input></input>\n\
	<output>\n\
	<stream type=\"video\" id=\"1\" pid=\"1\"/>\n\
	<stream type=\"audio\" id=\"1\" pid=\"1\"/>\n\
	<stream type=\"muxer\" aid=\"1\" vid=\"1\" pid=\"1\"/>\n\
	<presets>\n\
      <MediaCoderPrefs name=\"IPTV-TS-Template\" id=\"1\">\n\
		  <node key=\"overall.audio.enabled\">true</node>\n\
		  <node key=\"overall.audio.format\">MP2</node>\n\
		  <node key=\"overall.audio.encoder\">FFmpeg</node>\n\
		  <node key=\"overall.audio.channels\">3</node>\n\
		  <node key=\"audiofilter.resample.samplerate\">44100</node>\n\
		  <node key=\"audiofilter.extra.brdown\">true</node>\n\
		  <node key=\"overall.audio.copy\">false</node>\n\
		  <node key=\"audioenc.ffmpeg.bitrate\">128</node>\n\
		  <node key=\"audioenc.faac.bitrate\">48</node>\n\
		  <node key=\"audioenc.faac.mode\">1</node>\n\
		  <node key=\"audioenc.nero.mode\">2</node>\n\
	    <node key=\"overall.video.enabled\">true</node>\n\
	    <node key=\"overall.video.encoder\">x264</node>\n\
	    <node key=\"videofilter.scale.enabled\">true</node>\n\
	    <node key=\"videofilter.scale.width\">320</node>\n\
	    <node key=\"videofilter.scale.height\">240</node>\n\
	    <node key=\"videofilter.scale.algorithm\">Lanczos</node>\n\
	    <node key=\"videofilter.deint.mode\">0</node>\n\
	    <node key=\"videofilter.deint.algorithm\">YADIF</node>\n\
        <node key=\"videofilter.frame.enabled\">false</node>\n\
        <node key=\"videofilter.frame.fps\">25</node>\n\
        <node key=\"videofilter.frame.fpsScale\">1</node>\n\
		<node key=\"videofilter.extra.brdown\">true</node>\n\
        <node key=\"overall.video.mode\">3</node>\n\
        <node key=\"overall.video.bitrate\">320</node>\n\
        <node key=\"overall.video.format\">H.264</node>\n\
        <node key=\"overall.video.copy\">false</node>\n\
        <node key=\"videofilter.crop.mode\">3</node>\n\
        <node key=\"videoenc.x264.profile\">Main</node>\n\
        <node key=\"videoenc.x264.level\">0</node>\n\
        <node key=\"videoenc.x264.preset\">Custom</node>\n\
	    <node key=\"videoenc.x264.weight_p\">2</node>\n\
        <node key=\"videoenc.x264.bframes\">3</node>\n\
	    <node key=\"videoenc.x264.b_adapt\">2</node>\n\
	    <node key=\"videoenc.x264.weight_b\">true</node>\n\
	    <node key=\"videoenc.x264.b_pyramid\">2</node>\n\
	    <node key=\"videoenc.x264.frameref\">3</node>\n\
	    <node key=\"videoenc.x264.ratetol\">1.0</node>\n\
	    <node key=\"videoenc.x264.me\">2</node>\n\
	    <node key=\"videoenc.x264.me_range\">24</node>\n\
	    <node key=\"videoenc.x264.subme\">9</node>\n\
	    <node key=\"videoenc.x264.fast_pskip\">false</node>\n\
	    <node key=\"videoenc.x264.dct_decimate\">true</node>\n\
	    <node key=\"videoenc.x264.mixed_refs\">true</node>\n\
	    <node key=\"videoenc.x264.rc_lookahead\">60</node>\n\
	    <node key=\"videoenc.x264.direct_pred\">Auto</node>\n\
	    <node key=\"videoenc.x264.partitions\">2</node>\n\
	    <node key=\"videoenc.x264.trellis\">1</node>\n\
	    <node key=\"videoenc.x264.psy_rd\">1.0</node>\n\
	    <node key=\"videoenc.x264.psy_trellis\">0.1</node>\n\
	    <node key=\"videoenc.x264.showInfo\">false</node>\n\
	    <node key=\"videoenc.x264.qpmin\">0</node>\n\
		<node key=\"videoenc.x264.keyint\">50</node>\n\
		<node key=\"videoenc.x264.keyint_min\">0</node>\n\
		<node key=\"videoenc.x264.nalhrd\">2</node>\n\
		  <node key=\"overall.container.format\">3GP</node>\n\
		  <node key=\"overall.container.muxer\">MP4Box</node>\n\
		  <node key=\"muxer.tsmuxer.mode\">2</node>\n\
		  <config key=\"video.streams\">all</config>\n\
		  <config key=\"audio.streams\">all</config>\n\
		  <node key=\"overall.task.destdir\">OUTPUT</node>\n\
       </MediaCoderPrefs>\n\
    </presets>\n\
    <!-- split type=\"normal\" prefix=\"\" subfix=\"\" unitType=\"time\">\n\
      <clips>0-</clips>\n\
    </split -->\n\
  </output>\n\
</root>";

/////////////////////////////////////////////////////////////////////////////////////////
#define BUF_LEN 32
#define MAX_LOGO_FILE_NUMBER 30
#define MAX_LOGO_POS_NUMBER 8
#define MAX_THUMBNAIL_NUM 2

struct rectangle_t {
	int left, top, right, bottom;
};

struct float_rect_t {
	float left, top, right, bottom;
};

struct timed_rect_t {
	float_rect_t rect;
	int startTime, endTime;		// ms
};

enum movie_type_t {
	MOVIE_TYPE_MOVIE,		// Movie
	MOVIE_TYPE_EPISODE,		// Episode
	MOVIE_TYPE_INFO,		// Information
	MOVIE_TYPE_ANIME,		// Anime
};

struct source_config_t {
	int audio_stream;
	int audio_channel;
	int track_config;	// 0:单音轨, 1:原始音轨数, 2:双音轨
	int mix;
	int video_stream;
	float_rect_t video_rect;
	int crop_mode;
	int threshold;
	timed_rect_t delogo_pos[MAX_LOGO_POS_NUMBER];
	int delogo_num;
	std::vector<int> clipStartSet;		// clip start time (ms)
	std::vector<int> clipEndSet;		// clip end time (ms)
	int audio_decoder;
	int video_decoder;
	int type;
	int clip_mode;
	int demuxer;
};

struct video_codec_t {
	char name[BUF_LEN];
	int bitrate;
	rectangle_t rect;
	rectangle_t pos;
	int fpsNum;
	int fpsDen;
	int darNum;
	int darDen;
	int dim_nobigger;
	int rcmode;		
	int keyint;		// If keyint < 10, it's senconds else it's frame num.
	int reframes;	// reframes num
	int bframes;	// bframes num
	int lower_bitrate;
	int video_enhance;
	char profile[BUF_LEN];
	int brdown;		// Only downgrade video bitrate, if settings is bigger than source, use source bitrate
	int encoding;	// if encoding or not encoding, default will encode.
};

struct audio_codec_t {
	char name[BUF_LEN];
	int bitrate;
	int samplerate;
	int timeshift; //in ms
	float volGain;
	int autoVolGain;
	int brdown;		// Only downgrade audio bitrate, if settings is bigger than source, use source bitrate
};

struct logo_config_t {
	char graph_files[MAX_LOGO_FILE_NUMBER+1][MAX_PATH];
	int graph_interval;
	timed_rect_t log_pos[MAX_LOGO_POS_NUMBER];
	int relativePos;
	float offsetx;
	float offsety;
	float width;
	float height;
	float opaque;
	int speed;
	int waitTime;
	char routes[BUF_LEN];
	int logo_pos_num;
};

enum deint_mode_t {
	DEINT_AUTO = 0,
	DEINT_ENABLE = 1,
	DEINT_DISABLE = 2,
};

enum deint_algorithm_t {
	DEINT_YADIF = 0,
	DEINT_LINEAR = 1,
	DEINT_BLEND = 2
};

enum denoise_mode_t {
	DENOISE_DISABLE = 0,
	DENOISE_TEMPORAL = 1,
	DENOISE_NORMAL3D = 2,
	DENOISE_HQ3D = 3
};

struct filter_videorender_config_t {
	float contrastLevel;
	float sharpLevel;
	float colorLevel;
};

struct filter_noise_config_t {
	int depth;
	int mask;
};

struct filter_denoise_config_t {
	int denoiseMode;
	int lumaVal;
	int chromaVal;
	int temporalVal;
};

struct filter_deint_config_t {
	int deintMode;
	int deintAlgorithm;
};

struct filter_config_t {
	filter_videorender_config_t videorender;
	filter_noise_config_t noise;
	filter_denoise_config_t denoise;
	filter_deint_config_t deint;
};

#define X264_ZONE_NUM 2
struct x264_zone_t {
	float startTime;
	float endTime;
	float ratio;
};

enum {
	X264_ADDITION_OPTION = 0,
	X265_ADDITION_OPTION,
	MP4BOX_ADDITION_OPTION,
	ADDITION_OPTION_NUMBER,
};

struct codec_addition_t {
	char option[MAX_PATH];
	x264_zone_t* x264Zones[X264_ZONE_NUM]; 
};

struct segment_t {
	int type;				// 0/1/2(none/average/normal)
	int unit;				// 0/1 (size/time)
	int size;				// segment size (secs or kbytes)
	char postfix[MAX_PATH];
	int threshold;			// segment if duration is bigger than threshold(secs)
	int lastSegPercent;		// If last segment is bigger than percent% of size, then split.
};

struct playlist_t {
	char type[BUF_LEN];
	char name[MAX_PATH];
	char postfix[MAX_PATH];
	bool bLive;
};

struct hls_t {
	int dur;	//duration
	int listSize;	// item count in list
	int startIndex;	// segment start index(start file number), default is 0
	char postfix[MAX_PATH];
};

struct image_src_t {
	char imagePath[MAX_PATH];
	char imageFolder[MAX_PATH];
	int cropMode;
	int duration;
};

struct thumbnail_t {
	int start;			// Start time
	int end;			// End time
	int interval;		// Thumbnail interval(if set this, then count should be 0)
	int row;			// Tile image row
	int col;			// Tile image column
	int type;			// Image type(1:png, 2:jpg, 3:bmp, 4:gif)
	int count;			// Thumbnail count(if set this, then interval should be 0)
	int width;			// Thumbnail width
	int height;			// Thumbnail height
	int quality;		// Jpeg quality(max:100->best)
	char postfix[64];	// Postfix of thumbnail name
	bool bStitch;		// Stitch thumbnails to bigger image
	bool bPack;			// Pack thumbnail to ipk file
	bool bOptimize;		// Whether optimize jpeg image
};

struct target_config_t {
	audio_codec_t acodec;
	video_codec_t vcodec;
	int   filesize;
	float subtitle_timeshift;		// sec
	int   sub_id;
	int   extract_sub_id;			// sub id that will be extracted (-2:no extract, -1:extract all, other:specific sub id)
	logo_config_t logo;
	filter_config_t filter;
	char container_format[BUF_LEN];
	codec_addition_t codecConfig[ADDITION_OPTION_NUMBER];
	segment_t segmentConfig;
	playlist_t playlistConfig;
	image_src_t imageTail;
	thumbnail_t thumbnails[MAX_THUMBNAIL_NUM];
	hls_t hlsConfig;
	int ignoreErrIdx;	// ignoreErrIdx: 0(no ignore), 1(ignore 32), 2(ignore33), 3(ignore both)
	int coresNum;
	int disable_audio;	
	int disable_video;
	int disable_muxer;
	int disable_insert_blank_audio;
	int disable_padding_avdata;
	int enable_insert_blank_video;
	char ignoreErrCodeStr[BUF_LEN];
};

typedef struct {
	source_config_t source;
	target_config_t target;
} transcode_config_t;

/////////////////////////////////////////////////////////////////////////////////////////
// parsing rectangle string, eg {0,120,1280,600}
static bool ParseRect(const char *strRect, struct rectangle_t *retRect /*OUT*/)
{
	if (strRect == NULL || retRect == NULL) return false;

	if (*strRect == '{') strRect++;

	const char *delimiters = ",";
	rectangle_t rect;
	char *tmpRectStr = _strdup(strRect);
	char* tmpStr = tmpRectStr;
	char* pch = Strsep(&tmpStr, delimiters);
	if (pch == NULL) return false;
	rect.left = atoi(pch);
	EnsureMultipleOfDivisor(rect.left, 2);

	pch = Strsep(&tmpStr, delimiters);
	if (pch == NULL) return false;
	rect.top = atoi(pch);
	EnsureMultipleOfDivisor(rect.top, 2);

	pch = Strsep(&tmpStr, delimiters);
	if (pch == NULL) return false;
	rect.right = atoi(pch);
	rect.right -= rect.right % 2;

	pch = Strsep(&tmpStr, delimiters);
	if (pch == NULL) return false;
	rect.bottom = atoi(pch);
	rect.bottom -= rect.bottom % 2;

	*retRect = rect;
	free(tmpRectStr);
	return true;
}

// parsing rectangle string float, eg {0,120,1280,600} or {0,0.12,1,0.5}
static bool ParseRectFloat(const char *strRect, struct float_rect_t *retRect /*OUT*/)
{
	if (strRect == NULL || retRect == NULL) return false;

	if (*strRect == '{') strRect++;

	const char *delimiters = ",";
	char *tmpRectStr = _strdup(strRect);
	char* tmpStr = tmpRectStr;
	char* pch = Strsep(&tmpStr, delimiters);
	if (pch == NULL) return false;
	retRect->left = (float)atof(pch);
	
	pch = Strsep(&tmpStr, delimiters);
	if (pch == NULL) return false;
	retRect->top = (float)atof(pch);
	
	pch = Strsep(&tmpStr, delimiters);
	if (pch == NULL) return false;
	retRect->right = (float)atof(pch);
	
	pch = Strsep(&tmpStr, delimiters);
	if (pch == NULL) return false;
	retRect->bottom = (float)atof(pch);

	free(tmpRectStr);
	return true;
}

static std::string GetLocalIp()
{
	std::string localIp;
#ifdef WIN32
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 1), &wsaData) && 
		WSAStartup(MAKEWORD(1, 1), &wsaData )) {
		logger_err(LOGM_GLOBAL,"WSAStartup failed.\n");
		return "";
	}

	if (localIp.empty()) {
		char hostname[256] = {0};
		if (gethostname(hostname, sizeof(hostname)) != 0) {
			logger_err(LOGM_GLOBAL, "gethostname failed.\n");
			return "";
		}

		hostent *hp = gethostbyname(hostname);
		if (hp == NULL) return "";
		struct in_addr  **pptr = (struct in_addr **)hp->h_addr_list;

		while (pptr != NULL && *pptr != NULL) {
			char* curIp = inet_ntoa(**(pptr++));
			if(curIp && *curIp && strcmp(curIp, "127.0.0.1")) {
				localIp = curIp;
				break;
			}
		}
	}

	WSACleanup();
#else
    struct ifaddrs *ifaddr = 0, *ifa = 0;
    int s = 0;
    char host[NI_MAXHOST] = {0};

    if (getifaddrs(&ifaddr) == -1) return "";

    /* Walk through linked list, maintaining head pointer so we
      can free list later */

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {

        if (ifa->ifa_addr == NULL ||
            (ifa->ifa_name != NULL && strcmp(ifa->ifa_name, "lo") == 0)) continue;

        /* For an AF_INET interface address, display the address */

        if (ifa->ifa_addr->sa_family == AF_INET) {
            s = getnameinfo(ifa->ifa_addr,
                    sizeof(struct sockaddr_in),
                    host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                logger_err(LOGM_GLOBAL, "getnameinfo() failed: %s\n", gai_strerror(s));
                continue;
            }

            if(*host && strcmp(host, "127.0.0.1")) {
                localIp = host;
                break;
            }
        }
    }

    freeifaddrs(ifaddr);
#endif
	return localIp;
}

static void ParseX264Zone(StrPro::CXML2& xmlConfig, x264_zone_t* x264Zone)
{
	if(!x264Zone) return;
	x264Zone->startTime = ParsingFloatTimeString(xmlConfig.getAttribute("start"));
	x264Zone->endTime = ParsingFloatTimeString(xmlConfig.getAttribute("end"));
	const char* ratioStr = xmlConfig.getAttribute("ratio");
	if(ratioStr && *ratioStr) {
		x264Zone->ratio = (float)atof(ratioStr);
	}
}

static bool GetConfigFromXml(const std::string &strXmlConfig, transcode_config_t *config)
{
	bool ret = false;

	if (config == NULL) {
		logger_err(LOGM_GLOBAL, "Config is NULL.\n");
		return false;
	}

	StrPro::CXML2 xmlConfig;
	if(xmlConfig.Read(strXmlConfig.c_str()) != 0) {
		logger_err(LOGM_GLOBAL, "Invalid XML config.\n");
		return false;
	}

	memset(config, 0, sizeof(transcode_config_t));
	config->source.type = -1;
	config->target.filesize = 0;
	config->target.vcodec.darNum = -1;
	config->target.vcodec.darDen = -1;
	config->target.vcodec.lower_bitrate = 1;	// Default enable lower bitrate
	config->target.vcodec.video_enhance	= 1;	// Default enable video enhance
	config->target.vcodec.bframes = -1;
	config->target.vcodec.encoding = 1;
	config->target.acodec.autoVolGain = 1;		// Default enable volume auto gain
	config->target.vcodec.brdown = 1;
	config->target.acodec.brdown = 1;
	config->target.acodec.bitrate = 0;
	config->target.hlsConfig.dur = 5;
	config->target.extract_sub_id = -2;			// default:no extract sub
	strcpy(config->target.hlsConfig.postfix, ".mp4");
	do {
		if (xmlConfig.goRoot() == NULL) break;

		////////////////////////////////////////////////////////////////////////////////////
		//Source node
		if (xmlConfig.findChildNode("source") == NULL) {
			logger_err(LOGM_GLOBAL, "Can not find the source node\n");
			break;
		}

		// Movie type
		if (xmlConfig.findChildNode("type") != NULL) {
			config->source.type = xmlConfig.getNodeValueInt();
			xmlConfig.goParent();
		}

		// Demuxer type
		if (xmlConfig.findChildNode("demuxer") != NULL) {
			config->source.demuxer = xmlConfig.getNodeValueInt();
			xmlConfig.goParent();
		}

		//source audio
		if (xmlConfig.findChildNode("audio") != NULL) {
			config->source.audio_stream = xmlConfig.getAttributeInt("stream");
			config->source.track_config = xmlConfig.getAttributeInt("track");
			const char *channel = xmlConfig.getAttribute("channel");
			if (channel != NULL) {
				if (strcmp(channel, "left") == 0) {
					config->source.audio_channel = 0;
				} else if(strcmp(channel, "right") == 0) {
					config->source.audio_channel = 1;
				} else if(strcmp(channel, "both") == 0) {
					config->source.audio_channel = 2;
				} else if(strcmp(channel, "mix") == 0) {
					config->source.audio_channel = 2;
					config->source.mix = 1;
				} else if((strcmp(channel, "original") == 0)) {
					config->source.audio_channel = 3;
				}
			}
			const char* decoderStr = xmlConfig.getAttribute("decoder");
			if(decoderStr) {
				if(!_stricmp(decoderStr, "Mplayer")) {
					config->source.audio_decoder = AD_MPLAYER;
				} else if(!_stricmp(decoderStr, "Mencoder")) {
					config->source.audio_decoder = AD_MENCODER;
				} else if(!_stricmp(decoderStr, "FFMpeg")) {
					config->source.audio_decoder = AD_FFMPEG;
				} else if(!_stricmp(decoderStr, "AVS")) {
					config->source.audio_decoder = AD_AVS;
				} else if(!_stricmp(decoderStr, "Copy")) {
					config->source.audio_decoder = AD_COPY;
				}
			}

			xmlConfig.goParent();
		}

		//source video
		if (xmlConfig.findChildNode("video") != NULL) {
			config->source.video_stream = xmlConfig.getAttributeInt("stream");
			ParseRectFloat(xmlConfig.getAttribute("rectangle"), &(config->source.video_rect));
			// Default crop mode is manual crop
			config->source.crop_mode = 1;
			const char* cropStr = xmlConfig.getAttribute("crop");
			if(cropStr && *cropStr) {
				if(_stricmp(cropStr, "auto") == 0) {	// auto detect black band(4)
					config->source.crop_mode = 4;
				} else if(_stricmp(cropStr, "calc") == 0){
					config->source.crop_mode = 2;
				} else if(_stricmp(cropStr, "expand") == 0){
					config->source.crop_mode = 3;
				}
			} 
			config->source.threshold = xmlConfig.getAttributeInt("threshold");

			const char* decoderStr = xmlConfig.getAttribute("decoder");
			if(decoderStr) {
				if(!_stricmp(decoderStr, "Mplayer")) {
					config->source.video_decoder = VD_MPLAYER;
				} else if(!_stricmp(decoderStr, "Mencoder")) {
					config->source.video_decoder = VD_MENCODER;
				} else if(!_stricmp(decoderStr, "FFMpeg")) {
					config->source.video_decoder = VD_FFMPEG;
				} else if(!_stricmp(decoderStr, "AVS")) {
					config->source.video_decoder = VD_AVS;
				} else if(!_stricmp(decoderStr, "Copy")) {
					config->source.video_decoder = VD_COPY;
				}
			}
			xmlConfig.goParent();
		}

		// Clip mode
		if (xmlConfig.findChildNode("clipmode") != NULL) {
			config->source.clip_mode = xmlConfig.getNodeValueInt();
			xmlConfig.goParent();
		}

		// Clips
		void* clipNode = xmlConfig.findChildNode("clip");
		bool hasClipChild = clipNode ? true : false;
		while(clipNode) {
			int startTime = (int)(ParsingFloatTimeString(xmlConfig.getAttribute("start"))*1000);
			int endTime = (int)(ParsingFloatTimeString(xmlConfig.getAttribute("end"))*1000);
			if(startTime >= 0 && endTime > startTime) {
				config->source.clipStartSet.push_back(startTime);
				config->source.clipEndSet.push_back(endTime);
			}
			clipNode = xmlConfig.findNextNode("clip");
		}
		if(hasClipChild) xmlConfig.goParent();

		xmlConfig.goParent();

		//target node
		if (xmlConfig.findChildNode("target") == NULL) {
			logger_err(LOGM_GLOBAL, "Can not find the target node\n");
			break;
		}
		//target audio
		if (xmlConfig.findChildNode("audio") != NULL) {
			const char* disableStr = xmlConfig.getAttribute("disable");
			bool bEnableAudio = true;
			if(disableStr && !_stricmp(disableStr, "true")) {
				bEnableAudio = false;
				config->target.disable_audio = 1;
			}
			if(bEnableAudio) {
				if (xmlConfig.findChildNode("codec") != NULL || xmlConfig.findChildNode("codes") != NULL) {
					//format
					const char *format = xmlConfig.getAttribute("format");
					if (format != NULL) {
						strncpy(config->target.acodec.name, format, BUF_LEN);
					}
					//bitrate
					const char *bitrate = xmlConfig.getAttribute("bitrate");
					if (bitrate != NULL) {
						config->target.acodec.bitrate = atoi(bitrate);
						if (bitrate[strlen(bitrate) - 1] != 'k' &&  bitrate[strlen(bitrate) - 1] != 'K') {
							config->target.acodec.bitrate /= 1000;
						}
					}
					//samplerate
					config->target.acodec.samplerate = xmlConfig.getAttributeInt("samplerate");
					//timeshift (ms)
					config->target.acodec.timeshift = (int)(xmlConfig.getAttributeFloat("timeshift") * 1000);
				
					const char* brdown = xmlConfig.getAttribute("brdown");
					if(brdown && !_stricmp(brdown, "false")) {
						config->target.acodec.brdown = 0;
					}

					xmlConfig.goParent();
				}

				// volume control
				if (xmlConfig.findChildNode("volgain") != NULL) {
					config->target.acodec.volGain = xmlConfig.getAttributeFloat("value");
					const char* gainMode = xmlConfig.getAttribute("mode");
					if(gainMode && _stricmp(gainMode, "manual") == 0) {
						config->target.acodec.autoVolGain = 0;
					}
					xmlConfig.goParent();
				}
			}
			xmlConfig.goParent();
		}

		//target video
		if (xmlConfig.findChildNode("video") != NULL) {
			const char* disableStr = xmlConfig.getAttribute("disable");
			bool bEnableVideo = true;
			if(disableStr && !_stricmp(disableStr, "true")) {
				bEnableVideo = false;
				config->target.disable_video = 1;
			}

			if(bEnableVideo) {
				if (xmlConfig.findChildNode("rcmode") != NULL) {
					int rcMode = xmlConfig.getNodeValueInt();
					if(rcMode >= 1 && rcMode <= 4) {
						config->target.vcodec.rcmode = rcMode;
					}
					xmlConfig.goParent();
				}

				if (xmlConfig.findChildNode("codec") != NULL) {
					//format
					const char *format = xmlConfig.getAttribute("format");
					if (format != NULL) {
						strncpy(config->target.vcodec.name, format, BUF_LEN);
					}
					//bitrate
					const char *bitrate = xmlConfig.getAttribute("bitrate");
					if (bitrate != NULL) {
						config->target.vcodec.bitrate = atoi(bitrate);
						if (bitrate[strlen(bitrate) - 1] != 'k' && bitrate[strlen(bitrate) - 1] != 'K') {
							config->target.vcodec.bitrate /= 1000;
						}
						config->target.filesize = 0;
					}
					//rectangle
					ParseRect(xmlConfig.getAttribute("rectangle"), &(config->target.vcodec.rect));
					//pos
					ParseRect(xmlConfig.getAttribute("position"), &(config->target.vcodec.pos));

					// Dimension no bigger than the original size
					const char* nobigger = xmlConfig.getAttribute("nobigger");
					if(nobigger && strcmp(nobigger, "true") == 0) {
						config->target.vcodec.dim_nobigger = 1;
					} else {
						config->target.vcodec.dim_nobigger = 0;
					}

					const char* brdown = xmlConfig.getAttribute("brdown");
					if(brdown && !_stricmp(brdown, "false")) {
						config->target.vcodec.brdown = 0;
					}

					const char* encodeStr = xmlConfig.getAttribute("encode");
					if(encodeStr && !_stricmp(encodeStr, "false")) {
						config->target.vcodec.encoding = 0;
					}
					xmlConfig.goParent();
				}

				if (xmlConfig.findChildNode("fps") != NULL) {
					int fpsIndex = xmlConfig.getNodeValueInt();
					int fpsNum = 0, fpsDen = 0;
					switch(fpsIndex) {
					case 1: fpsNum = 8; fpsDen = 1; break;
					case 2: fpsNum = 12; fpsDen = 1; break;
					case 3: fpsNum = 15; fpsDen = 1; break;
					case 4: fpsNum = 18; fpsDen = 1; break;
					case 5: fpsNum = 20; fpsDen = 1; break;
					case 6: fpsNum = 24000; fpsDen = 1001; break;
					case 7: fpsNum = 24; fpsDen = 1; break;
					case 8: fpsNum = 25; fpsDen = 1; break;
					case 9: fpsNum = 30000; fpsDen = 1001; break;
					case 10: fpsNum = 30; fpsDen = 1; break;
					case 11: fpsNum = 5; fpsDen = 1; break;
					case 12: fpsNum = 10; fpsDen = 1; break;
					case 13: fpsNum = 6; fpsDen = 1; break;
					case 14: fpsNum = 7; fpsDen = 1; break;
					}
					config->target.vcodec.fpsNum = fpsNum;
					config->target.vcodec.fpsDen = fpsDen;
					xmlConfig.goParent();
				}
				if (xmlConfig.findChildNode("dar") != NULL) {
					float darValue = xmlConfig.getNodeValueFloat();
					int darNum = 0, darDen = 0;
					if(darValue > 0.001f) {
						GetFraction(darValue, &darNum, &darDen);
					}
					config->target.vcodec.darNum = darNum;
					config->target.vcodec.darDen = darDen;
					xmlConfig.goParent();
				}
				if (xmlConfig.findChildNode("keyint") != NULL) {
					int keyint = xmlConfig.getNodeValueInt();
					if(keyint >= 0) {
						config->target.vcodec.keyint = keyint;
					}
					xmlConfig.goParent();
				}
				if (xmlConfig.findChildNode("ref") != NULL) {
					int reframes = xmlConfig.getNodeValueInt();
					if(reframes > 0) {
						config->target.vcodec.reframes = reframes;
					}
					xmlConfig.goParent();
				}
				if (xmlConfig.findChildNode("bframes") != NULL) {
					int bframes = xmlConfig.getNodeValueInt();
					if(bframes >= 0) {
						config->target.vcodec.bframes = bframes;
					}
					xmlConfig.goParent();
				}
				if (xmlConfig.findChildNode("lowerbitrate") != NULL) {
					config->target.vcodec.lower_bitrate = xmlConfig.getNodeValueInt();
					xmlConfig.goParent();
				}

				if (xmlConfig.findChildNode("videoenhance") != NULL) {
					config->target.vcodec.video_enhance = xmlConfig.getNodeValueInt();
					xmlConfig.goParent();
				}

				if (xmlConfig.findChildNode("profile") != NULL) {
					strncpy(config->target.vcodec.profile, xmlConfig.getNodeValue(), BUF_LEN);
					xmlConfig.goParent();
				}
			}
			xmlConfig.goParent();
		}

		//filesize in Kbytes
        if (xmlConfig.findChildNode("filesize") != NULL) {
            const char *filesize = xmlConfig.getNodeValue();
            if (filesize != NULL) {
                config->target.filesize = atoi(filesize);
                char unit = filesize[strlen(filesize) - 1];
                if (unit != 'k' && unit != 'K') {
                    config->target.filesize /= 1000;
                }
            }
			xmlConfig.goParent();
        }

		// Bind process to some cores
		if (xmlConfig.findChildNode("cores") != NULL) {
			config->target.coresNum = xmlConfig.getNodeValueInt();
			xmlConfig.goParent();
		}

		//target subtitle
		if (xmlConfig.findChildNode("subtitle") != NULL) {
			//timeshift (unit: sec)
			config->target.subtitle_timeshift = xmlConfig.getAttributeFloat("timeshift");
			config->target.sub_id = xmlConfig.getAttributeInt("id", -1);
			config->target.extract_sub_id = xmlConfig.getAttributeInt("extract", -2);
			xmlConfig.goParent();
		}

		// delogo
		if (xmlConfig.findChildNode("delogo") != NULL) {
			void* cnode = xmlConfig.findChildNode("pos");
			bool isHasPosChild = (cnode != NULL ? true : false);
			int delogoIdx = 0;
			while (cnode) {
				timed_rect_t& delogoPos = config->source.delogo_pos[delogoIdx];
				const char* posStr = xmlConfig.getAttribute("position");
				if(!ParseRectFloat(posStr, &delogoPos.rect)) {
					cnode = xmlConfig.findNextNode("pos");
					continue;
				}

				if(FloatEqual(delogoPos.rect.left,0) && FloatEqual(delogoPos.rect.right,0) &&
					FloatEqual(delogoPos.rect.top,0) && FloatEqual(delogoPos.rect.bottom,0)) {
						cnode = xmlConfig.findNextNode("pos");
						continue;
				}

				if(delogoPos.rect.left < 0 || delogoPos.rect.right < 0 || 
					delogoPos.rect.top < 0 || delogoPos.rect.bottom < 0) {
					logger_err(LOGM_GLOBAL, "Delogo parameter is invalid, invalid XML config.\n");
					return false;
				}

				delogoPos.startTime = (int)(ParsingFloatTimeString(xmlConfig.getAttribute("start"))*1000);
				delogoPos.endTime = (int)(ParsingFloatTimeString(xmlConfig.getAttribute("end"))*1000);
				delogoIdx++;
				if(delogoIdx >= MAX_LOGO_POS_NUMBER) break;
				cnode = xmlConfig.findNextNode("pos");
			}
			config->source.delogo_num = delogoIdx;

			if(isHasPosChild) xmlConfig.goParent();
			xmlConfig.goParent();
		}

		//target logo
		if (xmlConfig.findChildNode("logo") != NULL) {
			if (xmlConfig.findChildNode("graph") != NULL) {
				//file
				const char *file = xmlConfig.getAttribute("file");
				if (file != NULL) {
					char *tmpFileStr = _strdup(file);
					char* tmpStr = tmpFileStr;
					const char *delimiters = ",";
					int index = 0;
					char *pch ;
					while ((pch = Strsep(&tmpStr, delimiters))!= NULL && index < MAX_LOGO_FILE_NUMBER) {
						if(*pch == '{') {	// remove first "{"
							pch += 1;
						}

						if(*(pch+strlen(pch)-1) == '}') {	// remove last "}"
							*(pch+strlen(pch)-1) = '\0';
						}
						strncpy(config->target.logo.graph_files[index++], pch, MAX_PATH);
					}
					free(tmpFileStr);
				}
				//interval
				config->target.logo.graph_interval = xmlConfig.getAttributeInt("interval");
				xmlConfig.goParent();
			}

			if (xmlConfig.findChildNode("show")) {
				if(xmlConfig.goChild()) {
					int index = 0;
					do {
						if(xmlConfig.isMatched("relativepos")) {	// Relative position
							config->target.logo.relativePos = xmlConfig.getAttributeInt("corner");
							config->target.logo.offsetx = xmlConfig.getAttributeFloat("offsetx");
							config->target.logo.offsety = xmlConfig.getAttributeFloat("offsety");
							config->target.logo.width = xmlConfig.getAttributeFloat("width");
							config->target.logo.height = xmlConfig.getAttributeFloat("height");
							config->target.logo.waitTime = xmlConfig.getAttributeInt("wait");
							config->target.logo.speed = xmlConfig.getAttributeInt("speed");
							config->target.logo.opaque = xmlConfig.getAttributeFloat("opaque");
							const char* routeStr = xmlConfig.getAttribute("route");
							if(routeStr && *routeStr) {
								strncpy(config->target.logo.routes, routeStr, BUF_LEN-1);
							}
							config->target.logo.logo_pos_num = 1;
							break;
						}

						// Absolute position
						if (!xmlConfig.isMatched("pos")) continue;
						timed_rect_t& logoPos = config->target.logo.log_pos[index];
						if (!ParseRectFloat(xmlConfig.getAttribute("position"), &logoPos.rect)) continue;
						logoPos.startTime = (int)(ParsingFloatTimeString(xmlConfig.getAttribute("start"))*1000);
						if (logoPos.startTime < 0) continue;
						logoPos.endTime = (int)(ParsingFloatTimeString(xmlConfig.getAttribute("end"))*1000);
						if (logoPos.endTime < 0) continue;
						index++;
					} while (xmlConfig.goNext() && index < MAX_LOGO_POS_NUMBER);
					if(index > 0) config->target.logo.logo_pos_num = index;
					xmlConfig.goParent();
				}
				xmlConfig.goParent();
			}
			xmlConfig.goParent();
		}

		//target filter
		if (xmlConfig.findChildNode("filter") != NULL) {
			if(config->source.type==3) {	//Anime
				//contrastlevel<(0,1),default:0.12>, sharplevel<(1,2),default:1.5>, colorlevel<(1,2),default:1.1>
				if (xmlConfig.findChildNode("videorender") != NULL) {
					config->target.filter.videorender.contrastLevel = xmlConfig.getAttributeFloat("contrastlevel");
					config->target.filter.videorender.sharpLevel = xmlConfig.getAttributeFloat("sharplevel");
					config->target.filter.videorender.colorLevel = xmlConfig.getAttributeFloat("colorlevel");
					xmlConfig.goParent();
				}
				else {
					if(config->target.vcodec.video_enhance==1) {
						config->target.filter.videorender.contrastLevel = 0.05;
						config->target.filter.videorender.sharpLevel = 1.5;
						config->target.filter.videorender.colorLevel = 1.05;
					}
				}
			}
			else {	//other types
				config->target.filter.videorender.contrastLevel = 0.12;
				config->target.filter.videorender.sharpLevel = 1.2;
				config->target.filter.videorender.colorLevel = 1.1;
			}
			//noise
			if (xmlConfig.findChildNode("noise") != NULL) {
				config->target.filter.noise.depth = xmlConfig.getAttributeInt("depth");
				config->target.filter.noise.mask = xmlConfig.getAttributeInt("mask");
				xmlConfig.goParent();
			}
			//denoise
			if (xmlConfig.findChildNode("denoise") != NULL) {
				config->target.filter.denoise.denoiseMode = xmlConfig.getAttributeInt("mode");
				config->target.filter.denoise.lumaVal = xmlConfig.getAttributeInt("luma");
				config->target.filter.denoise.chromaVal = xmlConfig.getAttributeInt("chroma");
				config->target.filter.denoise.temporalVal = xmlConfig.getAttributeInt("temporal");
				xmlConfig.goParent();
			}
			//deinterlace
			if (xmlConfig.findChildNode("deinterlace") != NULL) {
				config->target.filter.deint.deintMode = xmlConfig.getAttributeInt("mode");
				config->target.filter.deint.deintAlgorithm = xmlConfig.getAttributeInt("algorithm");
				xmlConfig.goParent();
			}
			xmlConfig.goParent();
		}

		//target container
		if (xmlConfig.findChildNode("mux") != NULL) {
			const char* disableStr = xmlConfig.getAttribute("disable");
			bool bEnableMux = true;
			if(disableStr && !_stricmp(disableStr, "true")) {
				bEnableMux = false;
				config->target.disable_muxer = 1;
			}

			//format
			if(bEnableMux) {
				const char *format = xmlConfig.getAttribute("format");
				if (format != NULL) {
					strncpy(config->target.container_format, format, BUF_LEN);
				}
			}
			
			xmlConfig.goParent();
		}

		// Segment configuration
		if (xmlConfig.findChildNode("segment") != NULL) {
			const char* type = xmlConfig.getAttribute("type");
			if(type && !strcmp(type, "average")) {
				config->target.segmentConfig.type = 1;
			} else if(type && !strcmp(type, "normal")) {
				config->target.segmentConfig.type = 2;
			}
			const char* unit = xmlConfig.getAttribute("unit");
			if(unit && !strcmp(unit, "time")) {
				config->target.segmentConfig.unit = 1;
			}
			int size = xmlConfig.getAttributeInt("size");
			if(size > 0) {
				config->target.segmentConfig.size = size;
			}
			int threshold = xmlConfig.getAttributeInt("threshold");
			if(threshold > 0) {
				config->target.segmentConfig.threshold = threshold;
			}
			const char* postfix = xmlConfig.getAttribute("postfix");
			if(postfix && *postfix) {
				strcpy(config->target.segmentConfig.postfix, postfix);
			}
			int lastsegpercent = xmlConfig.getAttributeInt("lastsegpercent");
			if(lastsegpercent >= 0) {
				config->target.segmentConfig.lastSegPercent = lastsegpercent;
			}
			xmlConfig.goParent();
		}

		// Playlist configuration
		if (xmlConfig.findChildNode("playlist") != NULL) {
			const char* type = xmlConfig.getAttribute("type");
			if(type) {
				strcpy(config->target.playlistConfig.type, type);
			}
			const char* name = xmlConfig.getAttribute("name");
			if(name) {
				strcpy(config->target.playlistConfig.name, name);
			}
			const char* postfix = xmlConfig.getAttribute("postfix");
			if(postfix) {
				strcpy(config->target.playlistConfig.postfix, postfix);
			}
			const char* bLiveStr = xmlConfig.getAttribute("live");
			config->target.playlistConfig.bLive = false;
			if(bLiveStr && !strcmp(bLiveStr, "true")) {
				config->target.playlistConfig.bLive = true;
			}
			xmlConfig.goParent();
		}

		// Image tail
		if (xmlConfig.findChildNode("imagetail") != NULL) {
			const char* imagePath = xmlConfig.getAttribute("image");
			const char* folderPath = xmlConfig.getAttribute("folder");
			if((imagePath && *imagePath) || (folderPath && *folderPath)) {
				if(imagePath && *imagePath) {
					strcpy(config->target.imageTail.imagePath, imagePath);
					int dur = xmlConfig.getAttributeInt("duration");
					config->target.imageTail.duration = dur;
				} else {
					strcpy(config->target.imageTail.imageFolder, folderPath);
				}
				int cropMode = xmlConfig.getAttributeInt("cropmode");
				config->target.imageTail.cropMode = cropMode;
			}
			xmlConfig.goParent();
		}

		// Apple hls
		if (xmlConfig.findChildNode("hls") != NULL) {
			config->target.hlsConfig.dur = xmlConfig.getAttributeInt("dur");
			config->target.hlsConfig.listSize = xmlConfig.getAttributeInt("listsize");
			config->target.hlsConfig.startIndex = xmlConfig.getAttributeInt("startindex");
			const char* postfixStr = xmlConfig.getAttribute("postfix");
			if(postfixStr && *postfixStr) {
				memset(config->target.hlsConfig.postfix, 0, MAX_PATH);
				strncpy(config->target.hlsConfig.postfix, postfixStr, strlen(postfixStr));
			}
			xmlConfig.goParent();
		}

		// Thumbnail
		if (xmlConfig.findChildNode("thumbnail") != NULL) {
			config->target.thumbnails[0].start = ParsingTimeString(xmlConfig.getAttribute("start"));
			config->target.thumbnails[0].end = ParsingTimeString(xmlConfig.getAttribute("end"));
			config->target.thumbnails[0].interval = xmlConfig.getAttributeInt("interval");
			config->target.thumbnails[0].row = xmlConfig.getAttributeInt("row");
			config->target.thumbnails[0].col = xmlConfig.getAttributeInt("col");
			config->target.thumbnails[0].type = xmlConfig.getAttributeInt("type");
			config->target.thumbnails[0].count = xmlConfig.getAttributeInt("count");
			config->target.thumbnails[0].width = xmlConfig.getAttributeInt("width");
			config->target.thumbnails[0].height = xmlConfig.getAttributeInt("height");
			config->target.thumbnails[0].quality = xmlConfig.getAttributeInt("quality");
			const char* stitchStr = xmlConfig.getAttribute("stitch");
			const char* packStr = xmlConfig.getAttribute("pack");
			const char* optimizeStr = xmlConfig.getAttribute("optimize");
			const char* postfixStr = xmlConfig.getAttribute("postfix");
			if(stitchStr && !_stricmp(stitchStr, "true")) {
				config->target.thumbnails[0].bStitch = true;
			} else {
				config->target.thumbnails[0].bStitch = false;
			}
			if(packStr && !_stricmp(packStr, "true")) {
				config->target.thumbnails[0].bPack = true;
			} else {
				config->target.thumbnails[0].bPack = false;
			}
			if(optimizeStr && !_stricmp(optimizeStr, "true")) {
				config->target.thumbnails[0].bOptimize = true;
			} else {
				config->target.thumbnails[0].bOptimize = false;
			}
			if(postfixStr && *postfixStr) {
				strncpy(config->target.thumbnails[0].postfix, postfixStr, 63);
			}
			
			// Parse another thumbnail
			if(xmlConfig.findNextNode("thumbnail")) {
				config->target.thumbnails[1].start = ParsingTimeString(xmlConfig.getAttribute("start"));
				config->target.thumbnails[1].end = ParsingTimeString(xmlConfig.getAttribute("end"));
				config->target.thumbnails[1].interval = xmlConfig.getAttributeInt("interval");
				config->target.thumbnails[1].row = xmlConfig.getAttributeInt("row");
				config->target.thumbnails[1].col = xmlConfig.getAttributeInt("col");
				config->target.thumbnails[1].type = xmlConfig.getAttributeInt("type");
				config->target.thumbnails[1].count = xmlConfig.getAttributeInt("count");
				config->target.thumbnails[1].width = xmlConfig.getAttributeInt("width");
				config->target.thumbnails[1].height = xmlConfig.getAttributeInt("height");
				config->target.thumbnails[1].quality = xmlConfig.getAttributeInt("quality");
				const char* stitchStr = xmlConfig.getAttribute("stitch");
				const char* packStr = xmlConfig.getAttribute("pack");
				const char* optimizeStr = xmlConfig.getAttribute("optimize");
				const char* postfixStr = xmlConfig.getAttribute("postfix");
				if(stitchStr && !_stricmp(stitchStr, "true")) {
					config->target.thumbnails[1].bStitch = true;
				} else {
					config->target.thumbnails[1].bStitch = false;
				}
				if(packStr && !_stricmp(packStr, "true")) {
					config->target.thumbnails[1].bPack = true;
				} else {
					config->target.thumbnails[1].bPack = false;
				}
				if(optimizeStr && !_stricmp(optimizeStr, "true")) {
					config->target.thumbnails[1].bOptimize = true;
				} else {
					config->target.thumbnails[1].bOptimize = false;
				}
				if(postfixStr && *postfixStr) {
					strncpy(config->target.thumbnails[1].postfix, postfixStr, 63);
				}
			}
			xmlConfig.goParent();
		}

		// Ignore error index (32/33 erro code)
		if (xmlConfig.findChildNode("errorignore") != NULL) {
			int ignoreErrIdx = xmlConfig.getNodeValueInt();
			if(ignoreErrIdx < 0) ignoreErrIdx = 0;
			config->target.ignoreErrIdx = ignoreErrIdx;
			xmlConfig.goParent();
		}
		
		// A/V align setting
		if (xmlConfig.findChildNode("avalign") != NULL) {
			const char* strEnableInsertAudio = xmlConfig.getAttribute("blankaudio");
			const char* strEnableInsertVideo = xmlConfig.getAttribute("blankvideo");
			const char* strEnablePadding = xmlConfig.getAttribute("padding");
			if(strEnableInsertAudio && !_stricmp(strEnableInsertAudio, "false")) {
				config->target.disable_insert_blank_audio = 1;
			}
			if(strEnableInsertVideo && !_stricmp(strEnableInsertVideo, "true")) {
				config->target.enable_insert_blank_video = 1;
			}
			if(strEnablePadding && !_stricmp(strEnablePadding, "false")) {
				config->target.disable_padding_avdata = 1;
			}
			xmlConfig.goParent();
		}

		if(xmlConfig.findChildNode("ignorecode") != NULL) {
			const char* strCodes = xmlConfig.getNodeValue();
			if(strCodes && *strCodes) {
				strncpy(config->target.ignoreErrCodeStr, strCodes, BUF_LEN);
			}
			xmlConfig.goParent();
		}

		//addition (compatible with misspelling)
		if (xmlConfig.findChildNode("addition") != NULL || xmlConfig.findChildNode("additon") != NULL) {
			if (xmlConfig.findChildNode("x264") != NULL) {
				const char *optionStr = xmlConfig.getAttribute("option");
				if (optionStr != NULL) {
					strncpy(config->target.codecConfig[X264_ADDITION_OPTION].option, optionStr, MAX_PATH);
				}

				if(xmlConfig.findChildNode("zone")) {	// Parse x264 zone
					x264_zone_t* startZone = new x264_zone_t;
					ParseX264Zone(xmlConfig, startZone);
					config->target.codecConfig[X264_ADDITION_OPTION].x264Zones[0] = startZone;
					if(xmlConfig.findNextNode("zone")) {
						x264_zone_t* endZone = new x264_zone_t;
						ParseX264Zone(xmlConfig,endZone);
						config->target.codecConfig[X264_ADDITION_OPTION].x264Zones[1] = endZone;
					}
					xmlConfig.goParent();
				}
				xmlConfig.goParent();
			}

			if(xmlConfig.findChildNode("mp4") != NULL) {
				const char *optionStr = xmlConfig.getAttribute("option");
				if (optionStr != NULL) {
					strncpy(config->target.codecConfig[MP4BOX_ADDITION_OPTION].option, optionStr, MAX_PATH);
				}
				xmlConfig.goParent();
			}

			if(xmlConfig.findChildNode("x265") != NULL) {
				const char *optionStr = xmlConfig.getAttribute("option");
				if (optionStr != NULL) {
					strncpy(config->target.codecConfig[X265_ADDITION_OPTION].option, optionStr, MAX_PATH);
				}
				xmlConfig.goParent();
			}
		}
		ret = true;
	} while (false);

	return ret;
}
/////////////////////////////////////////////////////////////////////////////////////////

bool CCliHelperPPLive::InitOutStdPrefs(const char* templateFile)
{
	if (m_inConfig.empty()) {
		logger_err(LOGM_GLOBAL, "input config is empty.\n");
		return false;
	}

	if(templateFile && *templateFile) {
		char* fileContent = ReadTextFile(templateFile);
		if(!fileContent) return false;
		m_outStdPrefs.clear();
		m_outStdPrefs = fileContent;
		size_t templateStart = m_outStdPrefs.find("<MediaCoderPrefs>");
		size_t templateEnd = m_outStdPrefs.find("</MediaCoderPrefs>");
		if(templateStart != string::npos && templateEnd != string::npos) {
			templateStart += strlen("<MediaCoderPrefs>");
			std::string tempContent = m_outStdPrefs.substr(templateStart, templateEnd-templateStart);
			m_outStdPrefs = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\
						 <root>\n\
						 <input />\n\
						 <output>\n\
						 <stream type=\"audio\">\n";
			m_outStdPrefs += tempContent + "</stream>" + "<stream type=\"video\">\n";
			m_outStdPrefs += tempContent + "</stream>" + "<muxer>\n";
			m_outStdPrefs += tempContent + "</muxer> </output> </root>";
		}
		free(fileContent);
	} else {
		m_outStdPrefs = prefsTemplate;
	}
	
	return true;
}

bool CCliHelperPPLive::AdjustPreset(const char *inMediaFile, const char *outDir, const char *tmpDir, int mainUrlIndex, const char *outMediaFile)
{
	if (m_inConfig.empty() || m_outStdPrefs.empty()) {
		logger_err(LOGM_GLOBAL, "config is empty.\n");
		return false;
	}

	CRootPrefs prefs;
	StrPro::CCharset charConvert;
	// Add input file in task preset
	if (inMediaFile != NULL && *inMediaFile != '\0') {
		std::string inputFile = inMediaFile;
		StrPro::StrHelper::prepareForXML(inputFile);
		char* utf8InputFile = charConvert.ANSItoUTF8(inputFile.c_str());
		if(utf8InputFile) {
			char urlPart[512] = {0};
			if(!tmpDir || !(*tmpDir)) {
				sprintf(urlPart, "<url>%s</url><index>%d</index>", utf8InputFile, mainUrlIndex);
			} else {
				// Add tmp dir
				char* utf8TmpDir = charConvert.ANSItoUTF8(tmpDir);
				if(utf8TmpDir) {
					sprintf(urlPart, "<url>%s</url><index>%d</index><tempdir>%s</tempdir>", utf8InputFile, mainUrlIndex, utf8TmpDir);
					free(utf8TmpDir);
				} else {
					sprintf(urlPart, "<url>%s</url><index>%d</index>", utf8InputFile, mainUrlIndex);
				}
			}

			size_t insertPos = m_outStdPrefs.find("</input>");
			if(insertPos != std::string::npos) {
				m_outStdPrefs.insert(insertPos, urlPart);
			}
			free(utf8InputFile);
		}
	}

	if(!prefs.InitRootForMaster(m_outStdPrefs.c_str())) {
		logger_err(LOGM_GLOBAL, "Invalid config input.\n");
		return false;
	}

	//outfile
	if (outMediaFile != NULL && *outMediaFile != '\0') {
		char* utf8Filepath = charConvert.ANSItoUTF8(outMediaFile);
		if(utf8Filepath) {
			prefs.SetStreamPref("overall.muxer.url", utf8Filepath, STMUXER);
			free(utf8Filepath);
		}
	}

	//outdir
	if (outDir != NULL && *outDir != '\0') {
		char* utf8Dir = charConvert.ANSItoUTF8(outDir);
		if(utf8Dir) {
			prefs.SetStreamPref("overall.task.destdir", utf8Dir, STMUXER);
			free(utf8Dir);
		}
	}

	transcode_config_t conf;
	if (!GetConfigFromXml(m_inConfig, &conf)) {
		logger_err(LOGM_GLOBAL, "The input config is invalid.\n");
		return false;
	}

	// Source clip
	if(conf.source.clip_mode == 1) {
		prefs.SetStreamPref("overall.task.losslessClip", true, STVIDEO);
	}

	size_t clipsCount = conf.source.clipStartSet.size();
	if(clipsCount >= 1){
		prefs.SetStreamPref("overall.clips.count", (int)clipsCount, STVIDEO);
		for(size_t i=0; i<clipsCount; ++i) {
			int startMs = conf.source.clipStartSet.at(i);
			int endMs = conf.source.clipEndSet.at(i);
			char clipKey[32] = {0};
			sprintf(clipKey, "overall.clips.start%d", i+1);
			prefs.SetStreamPref(clipKey, startMs, STVIDEO);
			memset(clipKey, 0, 32);
			sprintf(clipKey, "overall.clips.end%d", i+1);
			prefs.SetStreamPref(clipKey, endMs, STVIDEO);
		}
	}
	
	// ===========Identify preset level============
	int presetLevel = 0;
	// Mp4(H.264+AAC) format
	if(!_stricmp(conf.target.container_format, "mp4") && 
		!_stricmp(conf.target.acodec.name, "aac") &&
		!_stricmp(conf.target.vcodec.name, "h264") ) {
		int i;
        for (i=0; i< PPTV_LEVEL_MAX; ++i) {
            if(conf.target.vcodec.bitrate >= ppl_def[i].vbr) {
                presetLevel = i;
            }
        }
	}
	prefs.SetStreamPref("overall.task.ppLevel", presetLevel, STAUDIO);

	if(conf.target.acodec.bitrate==0) {
		// If user doesn't set the audio bitrate, use preset level's value judges by the video bitrate
		conf.target.acodec.bitrate = ppl_def[presetLevel].abr;
		// 0:Music 1:Movie 2:Episode 3:Anime 4:TVShow 5:Sport 6:Game
		if(conf.source.type == 0) {
			conf.target.acodec.bitrate += ppl_def[presetLevel].abr_inc_for_music;
		}
	}
	//else {
	//	// 0:Music 1:Movie 2:Episode 3:Anime 4:TVShow 5:Sport 6:Game
	//	if(conf.source.type == 0) {		// Increase audio bitrate of music
	//		conf.target.acodec.bitrate += 32;
	//	}
	//}

	// ---------------广告设置----------------
	if (conf.target.filesize > 0) {
		prefs.SetStreamPref("overall.output.filesize", conf.target.filesize, STMUXER);
		
		prefs.SetStreamPref("overall.audio.format", "HE-AAC", STAUDIO);
		prefs.SetStreamPref("overall.audio.encoder", "FDK AAC", STAUDIO);
		prefs.SetStreamPref("audioenc.faac.bitrate", 64, STAUDIO);
		prefs.SetStreamPref("audioenc.fdkaac.bitrate", 64, STAUDIO);
		prefs.SetStreamPref("overall.audio.channels", 2, STAUDIO);

        prefs.SetStreamPref("videofilter.scale.enabled", true, STVIDEO);
        prefs.SetStreamPref("videofilter.scale.width", 640, STVIDEO);

        conf.target.vcodec.lower_bitrate = 0;
	}

	int idx = 0;
	////////////////////////////////////////////////////////////////////////////
	//audio
	if(!conf.target.disable_audio) {
		struct {
			const char * name;
			int fmt;
			const char *psz_fmt;
			int encoder;
			const char *psz_encoder;
		} audio_map[] = {
			/*{"aac", AC_AAC_HE, "HE-AAC", AE_NEROREF, "Nero Encoder"},
			{"lcaac", AC_AAC_LC, "LC-AAC", AE_FAAC, "FAAC"},*/
			{"aac", AC_AAC_HE, "HE-AAC", AE_FDK, "FDK AAC"},
			{"lcaac", AC_AAC_LC, "LC-AAC", AE_FDK, "FDK AAC"},
			{"mp3", AC_MP3, "MP3", AE_FFMPEG, "FFmpeg"},
			{"mp2", AC_MP2, "MP2", AE_FFMPEG, "FFmpeg"},
			{"ac3", AC_AC3, "AC3", AE_FFMPEG, "FFmpeg"},
			{"eac3", AC_EAC3, "E-AC3", AE_DOLBY, "Dolby Encoder"},
			{"copy", AC_AC3, "AC3", AE_FFMPEG, "FFmpeg"},
			{"amr", AC_AMR, "AMR", AE_FFMPEG, "FFmpeg"}, 
			{"disable", AC_AC3, "AC3", AE_FFMPEG, "FFmpeg"},		
			{"", 0, 0}
		};

		// Audio track selection
		if(conf.source.track_config > 0) {
			prefs.SetStreamPref("extension.audio.tracknum", conf.source.track_config, STAUDIO);
		}

		if(conf.source.audio_stream > 0) {
			prefs.SetStreamPref("extension.audio.track1", conf.source.audio_stream, STAUDIO);
		}

		for (; *audio_map[idx].name != '\0'; idx++) {
			if (strcmp(conf.target.acodec.name, audio_map[idx].name) == 0) {
				break;
			}
		}

		if(idx == 0 && conf.target.acodec.bitrate >= 96) {
			idx = 1;	// If bitrate > 96 use faac instead of neroAacEnc
		}
        
		if(strcmp(conf.target.acodec.name, "disable") == 0) {   // disable audio
			prefs.SetStreamPref("overall.audio.enabled", false, STAUDIO);
		}

		if(strcmp(conf.target.acodec.name, "copy") == 0) {	// Copy audio
			prefs.SetStreamPref("overall.audio.copy", true, STAUDIO);
			prefs.SetStreamPref("overall.video.autoSource", false, STVIDEO);
		}

		prefs.SetStreamPref("overall.audio.format", audio_map[idx].psz_fmt, STAUDIO);
		prefs.SetStreamPref("overall.audio.encoder", audio_map[idx].psz_encoder, STAUDIO);

		if (audio_map[idx].encoder == AE_NEROREF) {
			prefs.SetStreamPref("audioenc.nero.mode", 1, STAUDIO);
		} else if(audio_map[idx].encoder == AE_DOLBY) {	// E-AC3 only support 48k samplerate, should up sample
			prefs.SetStreamPref("audiofilter.resample.downSamplingOnly", false, STAUDIO);
		} else if (audio_map[idx].encoder == AE_FDK) {
			if (audio_map[idx].fmt == AC_AAC_LC) {
				prefs.SetStreamPref("audioenc.fdkaac.profile", "MPEG4 LC", STAUDIO);
			}
		}

		if (conf.target.acodec.bitrate > 0) {
			prefs.SetStreamPref("audioenc.faac.bitrate", conf.target.acodec.bitrate, STAUDIO);
			prefs.SetStreamPref("audioenc.fdkaac.bitrate", conf.target.acodec.bitrate, STAUDIO);
			prefs.SetStreamPref("audioenc.ffmpeg.bitrate", conf.target.acodec.bitrate, STAUDIO);
			prefs.SetStreamPref("audioenc.nero.bitrate", conf.target.acodec.bitrate, STAUDIO);
			int dolbyBrcode = 0;
			if(conf.target.acodec.bitrate <= 96) {
				dolbyBrcode = 0;
			} else if(conf.target.acodec.bitrate <= 192) {
				dolbyBrcode = 1;
			} else if(conf.target.acodec.bitrate <= 256) {
				dolbyBrcode = 2;
			} else if(conf.target.acodec.bitrate <= 384) {
				dolbyBrcode = 3;
			} else if(conf.target.acodec.bitrate <= 448) {
				dolbyBrcode = 4;
			} else {
				dolbyBrcode = 5;
			}
			prefs.SetStreamPref("audioenc.dolby.bitrate", dolbyBrcode, STAUDIO);
			prefs.SetStreamPref("audioenc.faac.mode", 1, STAUDIO);
		}

		if (conf.target.acodec.samplerate > 0) {
			prefs.SetStreamPref("audiofilter.resample.samplerate", conf.target.acodec.samplerate, STAUDIO);
		}

		if(conf.target.acodec.brdown == 0) {
			prefs.SetStreamPref("audiofilter.extra.brdown", "false", STAUDIO);
		}

		// channel
		if(conf.source.mix == 1) {
			prefs.SetStreamPref("audiofilter.channels.mix", 1, STAUDIO);
			prefs.SetStreamPref("overall.audio.channels", 3, STAUDIO);
			//prefs.SetStreamPref("audiofilter.volume.gain", 5, STAUDIO);
		} else {
			switch(conf.source.audio_channel) {
			case 0:
				prefs.SetStreamPref("overall.audio.channels", 1, STAUDIO);
				break;
			case 1:
				prefs.SetStreamPref("overall.audio.channels", 2, STAUDIO);
				break;
			case 2:
				prefs.SetStreamPref("overall.audio.channels", 3, STAUDIO);
				break;
			case 3:
				prefs.SetStreamPref("overall.audio.channels", 0, STAUDIO);
				break;
			}
		}
		// audio decoder
		if(conf.source.audio_decoder > 0) {
			prefs.SetStreamPref("overall.video.autoSource", false, STVIDEO);
			prefs.SetStreamPref("overall.audio.source", conf.source.audio_decoder, STAUDIO);
		} 

		// timeshift (ms)
		int audioDelay = conf.target.acodec.timeshift;
		if(audioDelay != 0) {
			prefs.SetStreamPref("overall.audio.delay1", audioDelay, STAUDIO);
		}

		// audio volume adjustment
		bool bAutoGain = conf.target.acodec.autoVolGain ? true : false;
		prefs.SetStreamPref("audiofilter.volume.autoGain", bAutoGain, STAUDIO);

		if(!FloatEqual(conf.target.acodec.volGain, 0.f)) {	// not zero
			prefs.SetStreamPref("audiofilter.volume.gain", conf.target.acodec.volGain, STAUDIO);
		}

		if(conf.target.disable_audio) {
			prefs.SetStreamPref("overall.audio.enabled", false, STAUDIO);
		}
	}
	

	// ---------------video & filter----------------
	struct {
		const char * name;
		int fmt;
		const char *psz_fmt;
		int encoder;
		const char *psz_encoder;
	} video_map[] = {
		{"h264", VC_H264, "H.264", VE_X264, "X264"},
		{"xvid", VC_XVID, "XVID", VE_XVID, "XVID"},
		{"h263", VC_H263, "H.263", VE_FFMPEG, "FFmpeg"},
		{"wmv9", VC_WMV9, "WMV9", VE_FFMPEG, "FFmpeg"},
		{"mpeg2", VC_MPEG2, "MPEG2", VE_FFMPEG, "FFmpeg"},
		{"mii", VC_MII, "Mii", VE_MII, "Mii Encoder"},
		{"copy", VC_RAW, "H.264", VE_X264, "X264"},
		{"hevc", VC_HEVC, "HEVC", VE_X265, "x265"},
                {"disable", VC_RAW, "H.264", VE_X264, "X264"},
		{"", 0, 0}
	};

        if(strcmp(conf.target.vcodec.name, "disable") == 0) {   // disable video
             prefs.SetStreamPref("overall.video.enabled", false, STVIDEO);
        }

	// select video decoder
	if(conf.source.video_decoder > 0) {
		prefs.SetStreamPref("overall.video.autoSource", false, STVIDEO);
		prefs.SetStreamPref("overall.video.source", conf.source.video_decoder, STVIDEO);
	} else {
		prefs.SetStreamPref("overall.video.autoSource", true, STVIDEO);
	}
	
	// delogo filter(3 property: delogo position string, delogo num, delogo enable)
	int delogoIdx = 0;
	for(; delogoIdx<conf.source.delogo_num; ++delogoIdx) {
		timed_rect_t& delogoPos = conf.source.delogo_pos[delogoIdx];
		
		char keyName[32] = {0};
		sprintf(keyName, "videofilter.delogo.pos%d", delogoIdx+1);

		char timePos[128] = {0};
		float rectW = delogoPos.rect.right-delogoPos.rect.left;
		float rectH = delogoPos.rect.bottom-delogoPos.rect.top;
		sprintf(timePos, "%d,%d,%f,%f,%f,%f", delogoPos.startTime, delogoPos.endTime,
			delogoPos.rect.left, delogoPos.rect.top, rectW, rectH);
		prefs.SetStreamPref(keyName, timePos, STVIDEO);
	}
	if(delogoIdx > 0) {
		prefs.SetStreamPref("videofilter.delogo.enabled", true, STVIDEO);
		prefs.SetStreamPref("videofilter.delogo.num", delogoIdx, STVIDEO);
	}

	for (idx = 0; *video_map[idx].name != '\0'; idx++) {
		if (strcmp(conf.target.vcodec.name, video_map[idx].name) == 0) {
			break;
		}
	}

	if(strcmp(conf.target.vcodec.name, "copy") == 0) {	// Copy video
		prefs.SetStreamPref("overall.video.copy", true, STVIDEO);
		prefs.SetStreamPref("overall.audio.source", 3, STAUDIO);	// Use ffmpeg to decode audio
		prefs.SetStreamPref("overall.video.autoSource", false, STAUDIO);
	}
	prefs.SetStreamPref("overall.video.format", video_map[idx].psz_fmt, STVIDEO);
	prefs.SetStreamPref("overall.video.encoder", video_map[idx].psz_encoder, STVIDEO);

	if(conf.target.vcodec.brdown == 0) {
		prefs.SetStreamPref("videofilter.extra.brdown", "false", STVIDEO);
	}

	// Key frame interval
	if(conf.target.vcodec.keyint >= 0) {
		const char* keyintStr = "videoenc.x264.keyint";
		switch(video_map[idx].fmt) {
		case VC_XVID:
			keyintStr = "videoenc.xvid.keyint";
			break;
		case VC_H263: case VC_WMV9: case VC_MPEG2:
			keyintStr = "videoenc.ffmpeg.keyint";
			break;
		case VC_MII:
			keyintStr = "videoenc.mii.keyint";
			break;
		case VC_HEVC:
			keyintStr = "videoenc.x265.keyint";
			break;
		}
		prefs.SetStreamPref(keyintStr, conf.target.vcodec.keyint, STVIDEO);
	}

	if(conf.target.vcodec.reframes > 0) {
		const char* refStr = "videoenc.x264.frameref";
		switch(video_map[idx].fmt) {
		case VC_HEVC:
			refStr = "videoenc.x265.reframes";
			break;
		}
		prefs.SetStreamPref(refStr, conf.target.vcodec.reframes, STVIDEO);
	}

	if(conf.target.vcodec.bframes >= 0) {
		const char* refStr = "videoenc.x264.bframes";
		switch(video_map[idx].fmt) {
		case VC_HEVC:
			refStr = "videoenc.x265.bframes";
			break;
		}
		prefs.SetStreamPref(refStr, conf.target.vcodec.bframes, STVIDEO);
	}

	// Set decoder and encoder threads number
	if(conf.target.coresNum > 0) {
		const char* refStr = "overall.task.cores";
		prefs.SetStreamPref(refStr, conf.target.coresNum, STVIDEO);
	}

	// Adjust x264 and bitrate params according to movie type
	// if it's not ts/3gp output, reduce bitrate
	if(_stricmp(conf.target.container_format, "ts") && 
		_stricmp(conf.target.container_format, "3gp")) {	
		// Use video enhance
		if(conf.target.vcodec.video_enhance == 1) {

			prefs.SetStreamPref("videofilter.eq.mode", 1, STVIDEO);	// Intelligence enhance
			//prefs.SetStreamPref("videofilter.eq.colorlevel", 1.f, STVIDEO);
			//prefs.SetStreamPref("videofilter.eq.sharpness", 1.05f, STVIDEO);
			//prefs.SetStreamPref("videofilter.eq.contrastThreshold", 230, STVIDEO);
			//prefs.SetStreamPref("videofilter.eq.contrastLevel", 0.05f, STVIDEO);

			prefs.SetStreamPref("videofilter.eq.colorlevel", conf.target.filter.videorender.colorLevel, STVIDEO);
			prefs.SetStreamPref("videofilter.eq.sharpness", conf.target.filter.videorender.sharpLevel, STVIDEO);
			prefs.SetStreamPref("videofilter.eq.contrastThreshold", 180, STVIDEO);
			prefs.SetStreamPref("videofilter.eq.contrastLevel", conf.target.filter.videorender.contrastLevel, STVIDEO);
		}
		// Use MinQp to constrain peak bitrate
		if((conf.target.vcodec.rcmode == 0 || conf.target.vcodec.rcmode == 4) &&	// 2pass
			conf.target.vcodec.lower_bitrate == 1) {	// enable lower bitrate
			prefs.SetStreamPref("videoenc.x264.savePsnr", true, STVIDEO);
			//prefs.SetStreamPref("videoenc.x264.adjustMinQP", true, STVIDEO);
			//prefs.SetStreamPref("videoenc.x264.minQPType", 2, STVIDEO);
			//prefs.SetStreamPref("videoenc.x264.incMinQpThreshold", 27, STVIDEO);
			//prefs.SetStreamPref("videoenc.x264.incBrThreshold", 36, STVIDEO);
			//prefs.SetStreamPref("videoenc.x264.incBrPercent", 5, STVIDEO);
			//prefs.SetStreamPref("videoenc.x264.pass2MaxMinQp", 36, STVIDEO);
			//prefs.SetStreamPref("videoenc.x264.pass2MinMinQp", 20, STVIDEO);
			prefs.SetStreamPref("videoenc.x264.targetQp", 28, STVIDEO);
			prefs.SetStreamPref("videoenc.x264.removeQpLog", true, STVIDEO);

			// Increase video bitrate in two pass mode(video lower bitrate algorithm 2nd version)
        	conf.target.vcodec.bitrate += ppl_def[presetLevel].vbr_int_for_lowbr;
		}
	}

	if (conf.target.vcodec.bitrate > 0) {
		prefs.SetStreamPref("overall.video.bitrate", conf.target.vcodec.bitrate, STVIDEO);
		// Changed at 2012-0518 (Use MinQp to constrain peak bitrate)
		int maxBitRate = conf.target.vcodec.bitrate*2;
		prefs.SetStreamPref("videoenc.x264.vbv_maxrate", maxBitRate, STVIDEO);
		prefs.SetStreamPref("videoenc.x264.vbv_bufsize", maxBitRate/2, STVIDEO);
		if(video_map[idx].fmt == VC_MPEG2) {
			maxBitRate = conf.target.vcodec.bitrate;
			prefs.SetStreamPref("videoenc.ffmpeg.vrcMinRate", maxBitRate, STVIDEO);
			prefs.SetStreamPref("videoenc.ffmpeg.vrcMaxRate", maxBitRate, STVIDEO);
			prefs.SetStreamPref("videoenc.ffmpeg.vrcBufSize", maxBitRate/2, STVIDEO);
			prefs.SetStreamPref("videoenc.ffmpeg.me_range", 24, STVIDEO);
			prefs.SetStreamPref("videoenc.ffmpeg.subq", 8, STVIDEO);
			prefs.SetStreamPref("videoenc.ffmpeg.bframes", 2, STVIDEO);
			prefs.SetStreamPref("videoenc.ffmpeg.keyint", 15, STVIDEO);
		}
	}

	// Decoder performs filter
	prefs.SetStreamPref("videofilter.generic.applicator", VF_DECODER, STVIDEO);

	//video crop
	prefs.SetStreamPref("videofilter.expand.afterScale", true, STVIDEO);
	// Manual crop or auto-detect mode
	prefs.SetStreamPref("videofilter.crop.mode", conf.source.crop_mode, STVIDEO);
	prefs.SetStreamPref("videofilter.crop.detectLimit", conf.source.threshold, STVIDEO);
	float_rect_t& srcCropRect = conf.source.video_rect;
	float cropW = srcCropRect.right-srcCropRect.left;
	float cropH = srcCropRect.bottom-srcCropRect.top;
	if(cropW > 0 && cropH > 0) {
		float cropx = srcCropRect.left;
		float cropy = srcCropRect.top;
		if(cropx < 0) cropx = 0;
		if(cropy < 0) cropy = 0;
		if(cropx > 1) {
			cropx = ((int)(cropx/2+0.5f))*2.f;	// 左边如果是奇数，取大一点的偶数
		}
		if(cropy > 1) {
			cropy = ((int)(cropy/2)+0.5f)*2.f;	// 顶边如果是奇数，取大一点的偶数
		}
		if(cropW > 1) {
			cropW = ((int)(cropW/2))*2.f;
		}
		if(cropH > 1) {
			cropH = ((int)(cropH/2))*2.f;
		}
		prefs.SetStreamPref("videofilter.crop.left", cropx, STVIDEO);
		prefs.SetStreamPref("videofilter.crop.top", cropy, STVIDEO);
		prefs.SetStreamPref("videofilter.crop.width", cropW, STVIDEO);
		prefs.SetStreamPref("videofilter.crop.height", cropH, STVIDEO);
	}
	
	// Scale 
	rectangle_t destPos = conf.target.vcodec.pos;
	rectangle_t destVideoSize = conf.target.vcodec.rect;
	int destW = destVideoSize.right;
	int destH = destVideoSize.bottom;
	if(destW > 0 && destH > 0) {
		int darNum = 0, darDen = 0;
		int commonGcd = GetGcd(destW, destH);
		if(conf.target.vcodec.darNum < 0) {
			conf.target.vcodec.darNum = destW/commonGcd;
			conf.target.vcodec.darDen = destH/commonGcd;
		}
	}

	int picW = destPos.right - destPos.left;
	int picH = destPos.bottom - destPos.top;
	if(picW <= 0 && picH <= 0) {
		if(destW > 0 || destH > 0) {
			picW = destW;
			picH = destH;
		}
	}
	prefs.SetStreamPref("videofilter.scale.enabled", true, STVIDEO);
	prefs.SetStreamPref("videofilter.scale.width", picW, STVIDEO);
	prefs.SetStreamPref("videofilter.scale.height", picH, STVIDEO);

	if(picW > 800) {
		// When resolution is very large, defualt b_adapt(2) in first pass is very slow
		if(video_map[idx].fmt == VC_H264) {
			prefs.SetStreamPref("videoenc.x264.b_adapt", 0, STVIDEO);	// Disable b_adapt
		}
	} else {
		// Reduce x265 ctu when resolution is small
		if(video_map[idx].fmt == VC_HEVC) {
			prefs.SetStreamPref("videoenc.x265.ctu", 32, STVIDEO);
		}
	}

	if(conf.target.disable_video) {
		prefs.SetStreamPref("overall.video.enabled", false, STVIDEO);
	}
	if(conf.target.vcodec.encoding == 0) {
		prefs.SetStreamPref("overall.video.encode", false, STVIDEO);
	}
	
	// When resolution is small than 480, don't use psy, reduce FPS and key frame interval
	//if(abs(480-picW) < abs(640-picW)) {		
	//	prefs.SetStreamPref("videofilter.frame.enabled", true, STVIDEO);
	//	prefs.SetStreamPref("videofilter.frame.fps", 20, STVIDEO);
	//	prefs.SetStreamPref("videofilter.frame.fpsScale", 1, STVIDEO);
	//	prefs.SetStreamPref("videoenc.x264.keyint", 120, STVIDEO);
	//}

	// Video expand
	//if( destPos.left >= 0 && destPos.top >= 0 && (destW > 0 || destH > 0)
	//	&& !conf.target.vcodec.dim_nobigger) {
	//	prefs.SetStreamPref("videofilter.expand.enabled", true, STVIDEO);
	//	prefs.SetStreamPref("videofilter.expand.x", destPos.left, STVIDEO);
	//	prefs.SetStreamPref("videofilter.expand.y", destPos.top, STVIDEO);
	//	prefs.SetStreamPref("videofilter.expand.width", destW, STVIDEO);
	//	prefs.SetStreamPref("videofilter.expand.height", destH, STVIDEO);
	//}

	// logo
	logo_config_t& logoConfig = conf.target.logo;
	if(logoConfig.logo_pos_num > 0) {
		prefs.SetStreamPref("videofilter.overlay.enabled", true, STVIDEO);
		//prefs.SetStreamPref("videofilter.overlay.posNum", logoConfig.logo_pos_num, STVIDEO);
		prefs.SetStreamPref("videofilter.overlay.interval", logoConfig.graph_interval, STVIDEO);
		prefs.SetStreamPref("videofilter.overlay.opaque", 0.f, STVIDEO);
		prefs.SetStreamPref("videofilter.overlay.colored", true, STVIDEO);
		// Set image files
		std::string imageFiles;
		for (int i=0; i <= MAX_LOGO_FILE_NUMBER; ++i) {	// File name is separated by '|'
			if(logoConfig.graph_files[i] && *logoConfig.graph_files[i]) {
				imageFiles += logoConfig.graph_files[i];
				if(i < MAX_LOGO_FILE_NUMBER) {
					imageFiles += "|";
				}
			} else {
				break;
			}
		}
		if(imageFiles.size() > 0 && imageFiles[imageFiles.size()-1] == '|') {
			imageFiles.erase(imageFiles.end()-1);
		}

		prefs.SetStreamPref("videofilter.overlay.image", imageFiles.c_str(), STVIDEO);

		if(logoConfig.relativePos > 0) {	// Relative position
			prefs.SetStreamPref("videofilter.overlay.pos", logoConfig.relativePos, STVIDEO);
			prefs.SetStreamPref("videofilter.overlay.offsetX", logoConfig.offsetx, STVIDEO);
			prefs.SetStreamPref("videofilter.overlay.offsetY", logoConfig.offsety, STVIDEO);
			prefs.SetStreamPref("videofilter.overlay.width", logoConfig.width, STVIDEO);
			prefs.SetStreamPref("videofilter.overlay.height", logoConfig.height, STVIDEO);
			if(logoConfig.speed > 0) {
				prefs.SetStreamPref("videofilter.overlay.speed", logoConfig.speed, STVIDEO);
				prefs.SetStreamPref("videofilter.overlay.waitTime", logoConfig.waitTime, STVIDEO);
				prefs.SetStreamPref("videofilter.overlay.route", logoConfig.routes, STVIDEO);
				prefs.SetStreamPref("videofilter.overlay.opaque", logoConfig.opaque, STVIDEO);
			}
		} else {
			for (int i=0; i<logoConfig.logo_pos_num; ++i) {
				char posKey[36] = {0};
				sprintf(posKey, "videofilter.overlay.pos%d", i+1);
				char timePos[128] = {0};
				timed_rect_t& curLogo = logoConfig.log_pos[i];
				sprintf(timePos, "%d,%d,%f,%f,%f,%f", curLogo.startTime, 
					curLogo.endTime, curLogo.rect.left, curLogo.rect.top,
					curLogo.rect.right-curLogo.rect.left, curLogo.rect.bottom-curLogo.rect.top);
				prefs.SetStreamPref(posKey, timePos, STVIDEO);
			}
		}
	}

	// target video size is no bigger than original size
	if(conf.target.vcodec.dim_nobigger) {
		prefs.SetStreamPref("videofilter.scale.scaleDown", true, STVIDEO);
	} else {
		prefs.SetStreamPref("videofilter.scale.scaleDown", false, STVIDEO);
	}

	// rate control mode
	if(strcmp(conf.target.vcodec.name, "h264") != 0 && strcmp(conf.target.vcodec.name, "hevc") != 0) {	
		conf.target.vcodec.rcmode = 1;
	}

	if(conf.target.vcodec.rcmode >= 1) {
		prefs.SetStreamPref("overall.video.mode", conf.target.vcodec.rcmode-1, STVIDEO);
	}

	// fps
	if(conf.target.vcodec.fpsNum > 0) {
		prefs.SetStreamPref("videofilter.frame.enabled", true, STVIDEO);
		prefs.SetStreamPref("videofilter.frame.fps", conf.target.vcodec.fpsNum, STVIDEO);
		prefs.SetStreamPref("videofilter.frame.fpsScale", conf.target.vcodec.fpsDen, STVIDEO);
	}

	// dar
	if(conf.target.vcodec.darNum >= 0) {
		prefs.SetStreamPref("overall.video.ar", 2, STVIDEO);
		prefs.SetStreamPref("overall.video.arNum", conf.target.vcodec.darNum, STVIDEO);
		prefs.SetStreamPref("overall.video.arDen", conf.target.vcodec.darDen, STVIDEO);
	}

	// deinterlace
	int deintMode = 0;
	switch(conf.target.filter.deint.deintMode) {
	case DEINT_DISABLE: deintMode = 2; break;
	case DEINT_ENABLE: deintMode = 1; break;
	}
	int deintAlgo = 7;	//YADIF
	switch(conf.target.filter.deint.deintAlgorithm) {
	case DEINT_BLEND: deintAlgo = 0; break;
	case DEINT_LINEAR: deintAlgo = 1; break;
	}
	prefs.SetStreamPref("videofilter.deint.mode", deintMode, STVIDEO);
	prefs.SetStreamPref("videofilter.deint.algorithm", deintAlgo, STVIDEO);

	// denoise
	if(conf.target.filter.denoise.denoiseMode > 0) {
		prefs.SetStreamPref("videofilter.denoise.mode", conf.target.filter.denoise.denoiseMode, STVIDEO);
		prefs.SetStreamPref("videofilter.denoise.luma", conf.target.filter.denoise.lumaVal, STVIDEO);
		prefs.SetStreamPref("videofilter.denoise.chroma", conf.target.filter.denoise.chromaVal, STVIDEO);
		prefs.SetStreamPref("videofilter.denoise.strength", conf.target.filter.denoise.temporalVal, STVIDEO);
	}

	// subtitle delay
	prefs.SetStreamPref("overall.subtitle.delay", conf.target.subtitle_timeshift, STVIDEO);
	prefs.SetStreamPref("overall.subtitle.sid", conf.target.sub_id, STVIDEO);
	prefs.SetStreamPref("overall.subtitle.extract", conf.target.extract_sub_id, STVIDEO);
	
	// Write current machine ip to user data
	std::string curLocalIp = GetLocalIp();
	if(curLocalIp.size() > 4) {
		prefs.SetStreamPref("videoenc.x264.userData", curLocalIp.c_str(), STVIDEO);
	}

	// Codec spicified options (x264 and x265)
	bool baselineProfile = false;
	if(video_map[idx].fmt == VC_H264) {
		const char* profileStr = conf.target.vcodec.profile;
		if(*profileStr) {
			prefs.SetStreamPref("videoenc.x264.profile", profileStr, STVIDEO);
			if(!_stricmp(profileStr, "baseline")) {
				baselineProfile = true;
			}
		}
		const char* x264Options = conf.target.codecConfig[X264_ADDITION_OPTION].option;
		if(x264Options && *x264Options) {
			const char* x264Extras[] = {"ref","bframes","me", "subme", "merange","b-adapt", 
				"b-pyramid", "aq-mode", "weightp", "weightb", "trellis",
				"mbtree", "nal-hrd", "scenecut", "rc-lookahead", "psnr", "ssim"};

			const char* x264PresetItem[] = {"videoenc.x264.frameref", "videoenc.x264.bframes",
				"videoenc.x264.me", "videoenc.x264.subme", "videoenc.x264.me_range",
				"videoenc.x264.b_adapt", "videoenc.x264.b_pyramid", "videoenc.x264.aq_mode",
				"videoenc.x264.weight_p", "videoenc.x264.weight_b", "videoenc.x264.trellis", 
				"videoenc.x264.mbtree", "videoenc.x264.nalhrd", "videoenc.x264.scenecut",
				"videoenc.x264.rc_lookahead", "videoenc.x264.psnr", "videoenc.x264.ssim" };

			char *opts = _strdup(x264Options);
			char* tmpStr = opts, *pch = NULL;
			while(pch = Strsep(&tmpStr, ":")) {
				char* equalSign = strchr(pch, '=');
				if(!equalSign) {
					logger_err(LOGM_GLOBAL, "Invalid x264 options format.\n");
					break;
				}
				char* key = pch, *val = equalSign+1;
				*equalSign = NULL;
				size_t optsLen = sizeof(x264Extras)/sizeof(x264Extras[0]);
				for(size_t i=0; i<optsLen; ++i) {
					if(!_stricmp(key, x264Extras[i])) {
						prefs.SetStreamPref(x264PresetItem[i], val, STVIDEO);
						break;
					}
					if(i == optsLen-1) {
						logger_warn(LOGM_GLOBAL, "Unknown x264 option:%s.\n", key);
					}
				}
			}
			free(opts);
		}

		// x264 zone
		x264_zone_t* startZone = conf.target.codecConfig[X264_ADDITION_OPTION].x264Zones[0];
		if(startZone) {
			float zoneStartTime = startZone->startTime;
			float zoneEndTime = startZone->endTime;
			float zoneRf = startZone->ratio;
			if(zoneStartTime > -0.0001f && zoneEndTime > zoneStartTime) {
				prefs.SetStreamPref("videoenc.x264.zoneStart1", zoneStartTime, STVIDEO);
				prefs.SetStreamPref("videoenc.x264.zoneEnd1", zoneEndTime, STVIDEO);
				prefs.SetStreamPref("videoenc.x264.zoneRf1", zoneRf, STVIDEO);
			}
			delete startZone;
		}

		x264_zone_t* endZone = conf.target.codecConfig[X264_ADDITION_OPTION].x264Zones[1];
		if(endZone) {
			float zoneStartTime = endZone->startTime;
			float zoneEndTime = endZone->endTime;
			float zoneRf = endZone->ratio;
			if(zoneStartTime > -0.0001f && zoneEndTime > zoneStartTime) {
				prefs.SetStreamPref("videoenc.x264.zoneStart2", zoneStartTime, STVIDEO);
				prefs.SetStreamPref("videoenc.x264.zoneEnd2", zoneEndTime, STVIDEO);
				prefs.SetStreamPref("videoenc.x264.zoneRf2", zoneRf, STVIDEO);
			}
			delete endZone;
		}
	} else if (video_map[idx].fmt == VC_HEVC) {
		char* x265Options = conf.target.codecConfig[X265_ADDITION_OPTION].option;
		if(x265Options && *x265Options) {
			const char* x265Extras[] = {"ref", "bframes", "me", "subme", "sao", "amp", "rect", "b-adapt", "wpp", "ctu",
					   "frame-threads", "rd", "lft", "b-pyramid", "cutree", "aq-mode", "weightp", "weightb", "open-gop",
			           "vbv-bufsize", "vbv-maxrate", "vbv-init", "psnr", "ssim", "max-merge", "rc-lookahead", 
					   "tu-intra-depth", "tu-inter-depth", "b-intra", "psnr", "ssim"};

			const char* x265PresetItem[] = {"videoenc.x265.reframes", "videoenc.x265.bframes", "videoenc.x265.me", 
						"videoenc.x265.subme", "videoenc.x265.sao", "videoenc.x265.amp", "videoenc.x265.rect", "videoenc.x265.badapt",
						"videoenc.x265.wpp", "videoenc.x265.ctu", "videoenc.x265.frameThreads", "videoenc.x265.rdLevel",
						"videoenc.x265.loopFilter", "videoenc.x265.bpyramid", "videoenc.x265.cuTree",
						"videoenc.x265.aqMode", "videoenc.x265.weightp", "videoenc.x265.weightb", "videoenc.x265.openGop", 
						"videoenc.x265.vbvBufferSize", "videoenc.x265.vbvMaxrate", "videoenc.x265.vbvBufferInit", 
						"videoenc.x265.psnr", "videoenc.x265.ssim","videoenc.x265.maxMerge", "videoenc.x265.lookahead",
						"videoenc.x265.ctuIntra", "videoenc.x265.ctuInter", "videoenc.x265.bIntra", "videoenc.x265.pnsr", "videoenc.x265.ssim"};
			char *opts = _strdup(x265Options);
			char* tmpStr = opts, *pch = NULL;
			while(pch = Strsep(&tmpStr, ":")) {
				char* equalSign = strchr(pch, '=');
				if(!equalSign) {
					logger_err(LOGM_GLOBAL, "Invalid x265 options format:%s.\n", pch);
					break;
				}
				char* key = pch, *val = equalSign+1;
				*equalSign = NULL;
				size_t optsLen = sizeof(x265Extras)/sizeof(x265Extras[0]);
				for(size_t i=0; i<optsLen; ++i) {
					if(!_stricmp(key, x265Extras[i])) {
						prefs.SetStreamPref(x265PresetItem[i], val, STVIDEO);
						break;
					}
					if(i == optsLen-1) {
						logger_warn(LOGM_GLOBAL, "Unknown x265 option:%s.\n", key);
					}
				}
			}
			free(opts);
		}
	}

	// TODO: video color and video bright and denoise filter
	//conf.target.filter.color
	//conf.target.filter.noise

	// ----------------------container------------------------------
	if(!conf.target.disable_muxer) {
		struct {
			const char * name;
			int fmt;
			const char * psz_fmt;
			int muxer;
			const char * psz_muxer;
		} container_map[] = {
			{"3gp", CF_3GP, "3GP", MUX_MP4, "MP4Box"},
			{"3gp2", CF_3GP2, "3GP2", MUX_MP4, "MP4Box"},
			{"mp4", CF_MP4, "MP4", MUX_MP4, "MP4Box"},
			{"flv", CF_FLV, "FLV", MUX_FLV, "FLVMuxer"},
			{"f4v", CF_F4V, "F4V", MUX_MP4, "MP4Box"},
			{"mkv", CF_MKV, "Matroska", MUX_MKV, "MKVMerge"},
			{"ts", CF_MPEG2TS, "MPEG TS", MUX_TSMUXER, "TSMuxer"},
			{"hls", CF_HLS, "HLS", MUX_MP4, "MP4Box"},
			{"", 0, 0}
		};

		for (idx = 0; *container_map[idx].name != '\0'; idx++) {
			if (_stricmp(container_map[idx].name, conf.target.container_format) == 0) {
				break;
			}
		}

		prefs.SetStreamPref("overall.container.format", container_map[idx].psz_fmt, STMUXER);
		prefs.SetStreamPref("overall.container.muxer", container_map[idx].psz_muxer, STMUXER);

		if(idx == 6) {	// Muxer is TS
			prefs.SetStreamPref("muxer.tsmuxer.vbrmax", conf.target.vcodec.bitrate*3, STMUXER);
			// zoominla(preset for Jilin guangdian)
			/*prefs.SetStreamPref("videofilter.frame.enabled", true, STVIDEO);
			prefs.SetStreamPref("videofilter.frame.fps", 25, STVIDEO);
			prefs.SetStreamPref("videofilter.frame.fpsScale", 1, STVIDEO);
			prefs.SetStreamPref("videoenc.x264.weight_p", 2, STVIDEO);
			prefs.SetStreamPref("videoenc.x264.bframes", 3, STVIDEO);
			prefs.SetStreamPref("videoenc.x264.frameref", 3, STVIDEO);
			prefs.SetStreamPref("videoenc.x264.keyint", 24, STVIDEO);
			prefs.SetStreamPref("videoenc.x264.keyint_min", 24, STVIDEO);
			prefs.SetStreamPref("videoenc.x264.b_adapt", 0, STVIDEO);
			prefs.SetStreamPref("videoenc.x264.b_pyramid", 0, STVIDEO);
			prefs.SetStreamPref("videoenc.x264.me_range", 16, STVIDEO);
			prefs.SetStreamPref("videoenc.x264.subme", 7, STVIDEO);
			prefs.SetStreamPref("audiofilter.extra.brdown", false, STAUDIO);
			prefs.SetStreamPref("videofilter.extra.brdown", false, STVIDEO);
			prefs.SetStreamPref("videofilter.scale.scaleDown", false, STVIDEO);*/
		
		
			// When encoding TS format, input ts file must be good source, use lavf demuxer
			/*
			conf.source.demuxer = 1;
			if (conf.target.vcodec.bitrate > 0) {
				prefs.SetStreamPref("videoenc.x264.vbv_maxrate", conf.target.vcodec.bitrate, STVIDEO);
				prefs.SetStreamPref("videoenc.x264.vbv_bufsize", conf.target.vcodec.bitrate/3, STVIDEO);
				prefs.SetStreamPref("muxer.tsmuxer.cbrrate", conf.target.vcodec.bitrate+200, STMUXER);
			}
			prefs.SetStreamPref("muxer.tsmuxer.mode", 2, STMUXER);
			prefs.SetStreamPref("videoenc.x264.profile", "Main", STVIDEO);
			prefs.SetStreamPref("videoenc.x264.weight_p", 2, STVIDEO);
			prefs.SetStreamPref("videoenc.x264.bframes", 3, STVIDEO);
			prefs.SetStreamPref("videoenc.x264.frameref", 3, STVIDEO);
			prefs.SetStreamPref("videoenc.x264.keyint", 50, STVIDEO);
			prefs.SetStreamPref("videoenc.x264.nalhrd", 2, STVIDEO);
			if(destW > 0 && destH > 0) {	// If output width & height are set, ts output
				// Add black band to reach 720p ts output
				prefs.SetStreamPref("videofilter.crop.mode", 3, STVIDEO);
			}
			*/
		} else if(idx <= 2) {	// Mp4/3GP/3GP2
			// Change muxer according to video codec and mux format
			if(idx == 0 && conf.target.vcodec.name && !_stricmp(conf.target.vcodec.name, "xvid")) {
				prefs.SetStreamPref("overall.container.muxer", "FFmpeg", STMUXER);
			}

			prefs.SetStreamPref("overall.tagging.tag", true, STMUXER);
			char cprtStr[64] = {0};
			// Add dynamic segment strategy in copyright box
			int firstSegSize = 4, commonSegSize = 40;
			if(presetLevel >= PPTV_LEVEL_BD) {		    // BD
				firstSegSize = 18;
				commonSegSize = 90;
			} else if(presetLevel == PPTV_LEVEL_FHD) {	// CQ
				firstSegSize = 12;
				commonSegSize = 70;
			} else if(presetLevel == PPTV_LEVEL_HD) {	// HD
				firstSegSize = 8;
				commonSegSize = 40;
			} else if(presetLevel == PPTV_LEVEL_SD) {	// SD
				firstSegSize = 4;
				commonSegSize = 20;
			}

			firstSegSize = 20;
			commonSegSize = 20;
			// Temporally set segsize = 20M, use old segment method
			sprintf(cprtStr, "PPTV-"TS_MAJOR_VERSION"."TS_MINOR_VERSION"."SVN_REVISION"(%d,%d)[1]",
				firstSegSize, commonSegSize);
			prefs.SetStreamPref("overall.tagging.copyright", cprtStr, STMUXER);
			const char* mp4Option = conf.target.codecConfig[MP4BOX_ADDITION_OPTION].option;
			if(mp4Option && *mp4Option) {
				const char* pCh = NULL;
				if(pCh = strstr(mp4Option, "-hint")) {	// hint
					std::string hintStr = GetStringBetweenDelims(pCh, "=", ",");
					if(hintStr.compare("true") == 0) {
						prefs.SetStreamPref("muxer.mp4box.hint", true, STMUXER);
					}
				}
				if(pCh = strstr(mp4Option, "-sbr")) {	// sbr
					std::string hintStr = GetStringBetweenDelims(pCh, "=", ",");
					int sbrNum = atoi(hintStr.c_str());
					prefs.SetStreamPref("muxer.mp4box.sbr", sbrNum, STMUXER);
				}

				if(pCh = strstr(mp4Option, "-brand")) {	// brand
					std::string brandStr = GetStringBetweenDelims(pCh, "=", ",");
					prefs.SetStreamPref("muxer.mp4box.brand", brandStr.c_str(), STMUXER);
				}
				if(pCh = strstr(mp4Option, "-version")) {	// version number
					std::string verStr = GetStringBetweenDelims(pCh, "=", ",");
					int verNum = atoi(verStr.c_str());
					prefs.SetStreamPref("muxer.mp4box.version", verNum, STMUXER);
				}
			}
		}
	}
	

	if(conf.source.demuxer == 1) {	// Select demuxer to ts files
		prefs.SetStreamPref("videosrc.mencoder.lavfdemux", true, STVIDEO);
	}

	if(conf.target.disable_muxer) {
		prefs.SetStreamPref("overall.container.enabled", false, STMUXER);
	}

	// Segment configuration
	segment_t& segConfig = conf.target.segmentConfig;
	if (segConfig.type > 0) {
		const char* segType = "average";
		if(segConfig.type == 2) {
			segType = "normal";
		}
		prefs.SetStreamPref("extension.split.type", segType, STVIDEO);
		
		const char* unitType = "time";
		if(segConfig.unit == 0) {
			unitType = "size";
		}
		prefs.SetStreamPref("extension.split.unitType", unitType, STVIDEO);
		
		if(*segConfig.postfix) {
			prefs.SetStreamPref("extension.split.subfix", segConfig.postfix, STVIDEO);
		}

		if(segConfig.size > 0) {
			prefs.SetStreamPref("extension.split.segSize", segConfig.size, STVIDEO);
		}
		if(segConfig.lastSegPercent > 0) {
			prefs.SetStreamPref("extension.split.lastSegPercent", segConfig.lastSegPercent, STVIDEO);
		}
		if(segConfig.threshold > 0) {
			prefs.SetStreamPref("extension.split.threshold", segConfig.threshold, STVIDEO);
		}
	}

	// Playlist configuration
	playlist_t& plistConfig = conf.target.playlistConfig;
	if (*plistConfig.type) {
		prefs.SetStreamPref("extension.playlist.type", plistConfig.type, STVIDEO);
		if(*plistConfig.name) {
			prefs.SetStreamPref("extension.playlist.name", plistConfig.name, STVIDEO);
		}
		if(*plistConfig.postfix) {
			prefs.SetStreamPref("extension.playlist.postfix", plistConfig.postfix, STVIDEO);
		}
		prefs.SetStreamPref("extension.playlist.live", plistConfig.bLive, STVIDEO);
	}

	// Image tail
	if (*(conf.target.imageTail.imageFolder) || *(conf.target.imageTail.imagePath)) {
		if (*(conf.target.imageTail.imagePath)) {
			prefs.SetStreamPref("videofilter.imagetail.imagepath", conf.target.imageTail.imagePath, STVIDEO);
			prefs.SetStreamPref("videofilter.imagetail.duration", conf.target.imageTail.duration, STVIDEO);
		} else {
			prefs.SetStreamPref("videofilter.imagetail.imagefolder", conf.target.imageTail.imagePath, STVIDEO);
		}
		prefs.SetStreamPref("videofilter.imagetail.cropmode", conf.target.imageTail.cropMode, STVIDEO);
	}

	// Apple hls configuration
	hls_t& hlsConfig = conf.target.hlsConfig;
	if(hlsConfig.dur > 0) {
		prefs.SetStreamPref("muxer.hls.dur", hlsConfig.dur, STMUXER);
	}
	if(hlsConfig.listSize >= 0) {
		prefs.SetStreamPref("muxer.hls.listsize", hlsConfig.listSize, STMUXER);
	}
	if(hlsConfig.startIndex > 0) {
		prefs.SetStreamPref("muxer.hls.startIndex", hlsConfig.startIndex, STMUXER);
	}
	if(*hlsConfig.postfix) {
		prefs.SetStreamPref("muxer.hls.postfix", hlsConfig.postfix, STMUXER);
	}

	// Thumbnail 0
	const thumbnail_t& thumbNail = conf.target.thumbnails[0];
	if(thumbNail.count > 0 || thumbNail.interval > 0) {
		prefs.SetStreamPref("videofilter.thumb.enabled", true, STVIDEO);
		prefs.SetStreamPref("videofilter.thumb.align", 3, STVIDEO);	// User grid alignment
		prefs.SetStreamPref("videofilter.thumb.start", thumbNail.start, STVIDEO);
		prefs.SetStreamPref("videofilter.thumb.end", thumbNail.end, STVIDEO);
		prefs.SetStreamPref("videofilter.thumb.interval", thumbNail.interval, STVIDEO);
		prefs.SetStreamPref("videofilter.thumb.row", thumbNail.row, STVIDEO);
		prefs.SetStreamPref("videofilter.thumb.col", thumbNail.col, STVIDEO);
		prefs.SetStreamPref("videofilter.thumb.format", thumbNail.type, STVIDEO);
		prefs.SetStreamPref("videofilter.thumb.count", thumbNail.count, STVIDEO);
		prefs.SetStreamPref("videofilter.thumb.width", thumbNail.width, STVIDEO);
		prefs.SetStreamPref("videofilter.thumb.height", thumbNail.height, STVIDEO);
		prefs.SetStreamPref("videofilter.thumb.stitching", thumbNail.bStitch, STVIDEO);
		prefs.SetStreamPref("videofilter.thumb.pack", thumbNail.bPack, STVIDEO);
		prefs.SetStreamPref("videofilter.thumb.optimize", thumbNail.bOptimize, STVIDEO);
		prefs.SetStreamPref("videofilter.thumb.quality", thumbNail.quality, STVIDEO);
		if(*(thumbNail.postfix)) {
			prefs.SetStreamPref("videofilter.thumb.postfix", thumbNail.postfix, STVIDEO);
		}
	}

	// Thumbnail 1
	const thumbnail_t& thumbNail1 = conf.target.thumbnails[1];
	if(thumbNail1.count > 0 || thumbNail1.interval > 0) {
		prefs.SetStreamPref("videofilter.thumb1.enabled", true, STVIDEO);
		prefs.SetStreamPref("videofilter.thumb1.align", 3, STVIDEO);	// User grid alignment
		prefs.SetStreamPref("videofilter.thumb1.start", thumbNail1.start, STVIDEO);
		prefs.SetStreamPref("videofilter.thumb1.end", thumbNail1.end, STVIDEO);
		prefs.SetStreamPref("videofilter.thumb1.interval", thumbNail1.interval, STVIDEO);
		prefs.SetStreamPref("videofilter.thumb1.row", thumbNail1.row, STVIDEO);
		prefs.SetStreamPref("videofilter.thumb1.col", thumbNail1.col, STVIDEO);
		prefs.SetStreamPref("videofilter.thumb1.format", thumbNail1.type, STVIDEO);
		prefs.SetStreamPref("videofilter.thumb1.count", thumbNail1.count, STVIDEO);
		prefs.SetStreamPref("videofilter.thumb1.width", thumbNail1.width, STVIDEO);
		prefs.SetStreamPref("videofilter.thumb1.height", thumbNail1.height, STVIDEO);
		prefs.SetStreamPref("videofilter.thumb1.stitching", thumbNail1.bStitch, STVIDEO);
		prefs.SetStreamPref("videofilter.thumb1.pack", thumbNail1.bPack, STVIDEO);
		prefs.SetStreamPref("videofilter.thumb1.optimize", thumbNail1.bOptimize, STVIDEO);
		prefs.SetStreamPref("videofilter.thumb1.quality", thumbNail1.quality, STVIDEO);
		if(*(thumbNail1.postfix)) {
			prefs.SetStreamPref("videofilter.thumb1.postfix", thumbNail1.postfix, STVIDEO);
		}
	}

	// Ignore error index
	if(conf.target.ignoreErrIdx >= 0) {
		prefs.SetStreamPref("overall.task.ignoreError", conf.target.ignoreErrIdx, STVIDEO);
		prefs.SetStreamPref("overall.task.ignoreError", conf.target.ignoreErrIdx, STAUDIO);
	}

	if(conf.target.disable_insert_blank_audio) {
		prefs.SetStreamPref("overall.audio.insertBlank", false, STVIDEO);
	} else {
		prefs.SetStreamPref("overall.audio.insertBlank", true, STVIDEO);
	}
	if(conf.target.enable_insert_blank_video) {
		prefs.SetStreamPref("overall.video.insertBlank", true, STVIDEO);
	} else {
		prefs.SetStreamPref("overall.video.insertBlank", false, STVIDEO);
	}
	if(conf.target.disable_padding_avdata) {
		prefs.SetStreamPref("overall.task.alignAVData", false, STVIDEO);
	} else {
		prefs.SetStreamPref("overall.task.alignAVData", true, STVIDEO);
	}
	if(*conf.target.ignoreErrCodeStr) {
		prefs.SetStreamPref("overall.task.ignorecode", conf.target.ignoreErrCodeStr, STVIDEO);
	}

	m_outStdPrefs = prefs.DumpXml();
	return !m_outStdPrefs.empty();
}

std::string CCliHelperPPLive::GetMetaInfo(const char *mediaFile, const char *thumbnailFile)
{
	if (mediaFile == NULL || *mediaFile == '\0') {
		logger_err(LOGM_GLOBAL, "Invailed paramters in GetMetaInfo()\n");
		return "";
	}

#ifndef WIN32
	if (!FileExist(mediaFile)) {
		logger_err(LOGM_GLOBAL, "Media File %s is not existed\n", mediaFile);
		return "";
	}
#endif
	return "<?xml version=\"1.0\" encoding=\"UTF-8\"?><info><audio balance=\"80\" /></info>";
}
