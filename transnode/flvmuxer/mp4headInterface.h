#ifndef __MP4_HEAD_INTERFACE
#define __MP4_HEAD_INTERFACE
#include <iostream>
#include <fstream>
#include <vector>
using namespace std;

#ifndef INT32_MAX
#define INT32_MAX       0x7fffffff
#endif

#ifndef UINT32_MAX
#define UINT32_MAX      0xffffffff
#endif

#ifndef INT64_MIN
#define INT64_MIN       (-0x7fffffffffffffffLL - 1)
#endif

enum AVRoundingPPTV {
	AV_ROUND_ZEROPPTV     = 0, ///< Round toward zero.
	AV_ROUND_INFPPTV      = 1, ///< Round away from zero.
	AV_ROUND_DOWNPPTV     = 2, ///< Round toward -infinity.
	AV_ROUND_UPPPTV       = 3, ///< Round toward +infinity.
	AV_ROUND_NEAR_INFPPTV = 5, ///< Round to nearest and halfway cases away from zero.
};

enum AVMediaTypePPTV {
	AVMEDIA_TYPE_UNKNOWNPPTV = -1,
	AVMEDIA_TYPE_VIDEOPPTV,
	AVMEDIA_TYPE_AUDIOPPTV,
	AVMEDIA_TYPE_DATAPPTV,
	AVMEDIA_TYPE_SUBTITLEPPTV,
	AVMEDIA_TYPE_ATTACHMENTPPTV,
	AVMEDIA_TYPE_NBPPTV
};

enum CodecIDPPTV {
	CODEC_ID_NONEPPTV,

	/* video codecs */
	CODEC_ID_MPEG1VIDEOPPTV,
	CODEC_ID_MPEG2VIDEOPPTV, ///< preferred ID for MPEG-1/2 video decoding
	CODEC_ID_MPEG2VIDEO_XVMCPPTV,
	CODEC_ID_H261PPTV,
	CODEC_ID_H263PPTV,
	CODEC_ID_RV10PPTV,
	CODEC_ID_RV20PPTV,
	CODEC_ID_MJPEGPPTV,
	CODEC_ID_MJPEGBPPTV,
	CODEC_ID_LJPEGPPTV,
	CODEC_ID_SP5XPPTV,
	CODEC_ID_JPEGLSPPTV,
	CODEC_ID_MPEG4PPTV,
	CODEC_ID_RAWVIDEOPPTV,
	CODEC_ID_MSMPEG4V1PPTV,
	CODEC_ID_MSMPEG4V2PPTV,
	CODEC_ID_MSMPEG4V3PPTV,
	CODEC_ID_WMV1PPTV,
	CODEC_ID_WMV2PPTV,
	CODEC_ID_H263PPPTV,
	CODEC_ID_H263IPPTV,
	CODEC_ID_FLV1PPTV,
	CODEC_ID_SVQ1PPTV,
	CODEC_ID_SVQ3PPTV,
	CODEC_ID_DVVIDEOPPTV,
	CODEC_ID_HUFFYUVPPTV,
	CODEC_ID_CYUVPPTV,
	CODEC_ID_H264PPTV,
	CODEC_ID_INDEO3PPTV,
	CODEC_ID_VP3PPTV,
	CODEC_ID_THEORAPPTV,
	CODEC_ID_ASV1PPTV,
	CODEC_ID_ASV2PPTV,
	CODEC_ID_FFV1PPTV,
	CODEC_ID_4XMPPTV,
	CODEC_ID_VCR1PPTV,
	CODEC_ID_CLJRPPTV,
	CODEC_ID_MDECPPTV,
	CODEC_ID_ROQPPTV,
	CODEC_ID_INTERPLAY_VIDEOPPTV,
	CODEC_ID_XAN_WC3PPTV,
	CODEC_ID_XAN_WC4PPTV,
	CODEC_ID_RPZAPPTV,
	CODEC_ID_CINEPAKPPTV,
	CODEC_ID_WS_VQAPPTV,
	CODEC_ID_MSRLEPPTV,
	CODEC_ID_MSVIDEO1PPTV,
	CODEC_ID_IDCINPPTV,
	CODEC_ID_8BPSPPTV,
	CODEC_ID_SMCPPTV,
	CODEC_ID_FLICPPTV,
	CODEC_ID_TRUEMOTION1PPTV,
	CODEC_ID_VMDVIDEOPPTV,
	CODEC_ID_MSZHPPTV,
	CODEC_ID_ZLIBPPTV,
	CODEC_ID_QTRLEPPTV,
	CODEC_ID_SNOWPPTV,
	CODEC_ID_TSCCPPTV,
	CODEC_ID_ULTIPPTV,
	CODEC_ID_QDRAWPPTV,
	CODEC_ID_VIXLPPTV,
	CODEC_ID_QPEGPPTV,
#if LIBAVCODEC_VERSION_MAJOR < 53
	CODEC_ID_XVIDPPTV,
#endif
	CODEC_ID_PNGPPTV,
	CODEC_ID_PPMPPTV,
	CODEC_ID_PBMPPTV,
	CODEC_ID_PGMPPTV,
	CODEC_ID_PGMYUVPPTV,
	CODEC_ID_PAMPPTV,
	CODEC_ID_FFVHUFFPPTV,
	CODEC_ID_RV30PPTV,
	CODEC_ID_RV40PPTV,
	CODEC_ID_VC1PPTV,
	CODEC_ID_WMV3PPTV,
	CODEC_ID_LOCOPPTV,
	CODEC_ID_WNV1PPTV,
	CODEC_ID_AASCPPTV,
	CODEC_ID_INDEO2PPTV,
	CODEC_ID_FRAPSPPTV,
	CODEC_ID_TRUEMOTION2PPTV,
	CODEC_ID_BMPPPTV,
	CODEC_ID_CSCDPPTV,
	CODEC_ID_MMVIDEOPPTV,
	CODEC_ID_ZMBVPPTV,
	CODEC_ID_AVSPPTV,
	CODEC_ID_SMACKVIDEOPPTV,
	CODEC_ID_NUVPPTV,
	CODEC_ID_KMVCPPTV,
	CODEC_ID_FLASHSVPPTV,
	CODEC_ID_CAVSPPTV,
	CODEC_ID_JPEG2000PPTV,
	CODEC_ID_VMNCPPTV,
	CODEC_ID_VP5PPTV,
	CODEC_ID_VP6PPTV,
	CODEC_ID_VP6FPPTV,
	CODEC_ID_TARGAPPTV,
	CODEC_ID_DSICINVIDEOPPTV,
	CODEC_ID_TIERTEXSEQVIDEOPPTV,
	CODEC_ID_TIFFPPTV,
	CODEC_ID_GIFPPTV,
	CODEC_ID_FFH264PPTV,
	CODEC_ID_DXAPPTV,
	CODEC_ID_DNXHDPPTV,
	CODEC_ID_THPPPTV,
	CODEC_ID_SGIPPTV,
	CODEC_ID_C93PPTV,
	CODEC_ID_BETHSOFTVIDPPTV,
	CODEC_ID_PTXPPTV,
	CODEC_ID_TXDPPTV,
	CODEC_ID_VP6APPTV,
	CODEC_ID_AMVPPTV,
	CODEC_ID_VBPPTV,
	CODEC_ID_PCXPPTV,
	CODEC_ID_SUNRASTPPTV,
	CODEC_ID_INDEO4PPTV,
	CODEC_ID_INDEO5PPTV,
	CODEC_ID_MIMICPPTV,
	CODEC_ID_RL2PPTV,
	CODEC_ID_8SVX_EXPPPTV,
	CODEC_ID_8SVX_FIBPPTV,
	CODEC_ID_ESCAPE124PPTV,
	CODEC_ID_DIRACPPTV,
	CODEC_ID_BFIPPTV,
	CODEC_ID_CMVPPTV,
	CODEC_ID_MOTIONPIXELSPPTV,
	CODEC_ID_TGVPPTV,
	CODEC_ID_TGQPPTV,
	CODEC_ID_TQIPPTV,
	CODEC_ID_AURAPPTV,
	CODEC_ID_AURA2PPTV,
	CODEC_ID_V210XPPTV,
	CODEC_ID_TMVPPTV,
	CODEC_ID_V210PPTV,
	CODEC_ID_DPXPPTV,
	CODEC_ID_MADPPTV,
	CODEC_ID_FRWUPPTV,
	CODEC_ID_FLASHSV2PPTV,
	CODEC_ID_CDGRAPHICSPPTV,
	CODEC_ID_R210PPTV,
	CODEC_ID_ANMPPTV,
	CODEC_ID_BINKVIDEOPPTV,
	CODEC_ID_IFF_ILBMPPTV,
	CODEC_ID_IFF_BYTERUN1PPTV,
	CODEC_ID_KGV1PPTV,
	CODEC_ID_YOPPPTV,
	CODEC_ID_VP8PPTV,

	/* various PCM "codecs" */
	CODEC_ID_PCM_S16LEPPTV= 0x10000,
	CODEC_ID_PCM_S16BEPPTV,
	CODEC_ID_PCM_U16LEPPTV,
	CODEC_ID_PCM_U16BEPPTV,
	CODEC_ID_PCM_S8PPTV,
	CODEC_ID_PCM_U8PPTV,
	CODEC_ID_PCM_MULAWPPTV,
	CODEC_ID_PCM_ALAWPPTV,
	CODEC_ID_PCM_S32LEPPTV,
	CODEC_ID_PCM_S32BEPPTV,
	CODEC_ID_PCM_U32LEPPTV,
	CODEC_ID_PCM_U32BEPPTV,
	CODEC_ID_PCM_S24LEPPTV,
	CODEC_ID_PCM_S24BEPPTV,
	CODEC_ID_PCM_U24LEPPTV,
	CODEC_ID_PCM_U24BEPPTV,
	CODEC_ID_PCM_S24DAUDPPTV,
	CODEC_ID_PCM_ZORKPPTV,
	CODEC_ID_PCM_S16LE_PLANARPPTV,
	CODEC_ID_PCM_DVDPPTV,
	CODEC_ID_PCM_F32BEPPTV,
	CODEC_ID_PCM_F32LEPPTV,
	CODEC_ID_PCM_F64BEPPTV,
	CODEC_ID_PCM_F64LEPPTV,
	CODEC_ID_PCM_BLURAYPPTV,

	/* various ADPCM codecs */
	CODEC_ID_ADPCM_IMA_QTPPTV= 0x11000,
	CODEC_ID_ADPCM_IMA_WAVPPTV,
	CODEC_ID_ADPCM_IMA_DK3PPTV,
	CODEC_ID_ADPCM_IMA_DK4PPTV,
	CODEC_ID_ADPCM_IMA_WSPPTV,
	CODEC_ID_ADPCM_IMA_SMJPEGPPTV,
	CODEC_ID_ADPCM_MSPPTV,
	CODEC_ID_ADPCM_4XMPPTV,
	CODEC_ID_ADPCM_XAPPTV,
	CODEC_ID_ADPCM_ADXPPTV,
	CODEC_ID_ADPCM_EAPPTV,
	CODEC_ID_ADPCM_G726PPTV,
	CODEC_ID_ADPCM_CTPPTV,
	CODEC_ID_ADPCM_SWFPPTV,
	CODEC_ID_ADPCM_YAMAHAPPTV,
	CODEC_ID_ADPCM_SBPRO_4PPTV,
	CODEC_ID_ADPCM_SBPRO_3PPTV,
	CODEC_ID_ADPCM_SBPRO_2PPTV,
	CODEC_ID_ADPCM_THPPPTV,
	CODEC_ID_ADPCM_IMA_AMVPPTV,
	CODEC_ID_ADPCM_EA_R1PPTV,
	CODEC_ID_ADPCM_EA_R3PPTV,
	CODEC_ID_ADPCM_EA_R2PPTV,
	CODEC_ID_ADPCM_IMA_EA_SEADPPTV,
	CODEC_ID_ADPCM_IMA_EA_EACSPPTV,
	CODEC_ID_ADPCM_EA_XASPPTV,
	CODEC_ID_ADPCM_EA_MAXIS_XAPPTV,
	CODEC_ID_ADPCM_IMA_ISSPPTV,

	/* AMR */
	CODEC_ID_AMR_NBPPTV= 0x12000,
	CODEC_ID_AMR_WBPPTV,

	/* RealAudio codecs*/
	CODEC_ID_RA_144PPTV= 0x13000,
	CODEC_ID_RA_288PPTV,

	/* various DPCM codecs */
	CODEC_ID_ROQ_DPCMPPTV= 0x14000,
	CODEC_ID_INTERPLAY_DPCMPPTV,
	CODEC_ID_XAN_DPCMPPTV,
	CODEC_ID_SOL_DPCMPPTV,

	/* audio codecs */
	CODEC_ID_MP2PPTV= 0x15000,
	CODEC_ID_MP3PPTV, ///< preferred ID for decoding MPEG audio layer 1, 2 or 3
	CODEC_ID_AACPPTV,
	CODEC_ID_AC3PPTV,
	CODEC_ID_DTSPPTV,
	CODEC_ID_VORBISPPTV,
	CODEC_ID_DVAUDIOPPTV,
	CODEC_ID_WMAV1PPTV,
	CODEC_ID_WMAV2PPTV,
	CODEC_ID_MACE3PPTV,
	CODEC_ID_MACE6PPTV,
	CODEC_ID_VMDAUDIOPPTV,
	CODEC_ID_SONICPPTV,
	CODEC_ID_SONIC_LSPPTV,
	CODEC_ID_FLACPPTV,
	CODEC_ID_MP3ADUPPTV,
	CODEC_ID_MP3ON4PPTV,
	CODEC_ID_SHORTENPPTV,
	CODEC_ID_ALACPPTV,
	CODEC_ID_WESTWOOD_SND1PPTV,
	CODEC_ID_GSMPPTV, ///< as in Berlin toast format
	CODEC_ID_QDM2PPTV,
	CODEC_ID_COOKPPTV,
	CODEC_ID_TRUESPEECHPPTV,
	CODEC_ID_TTAPPTV,
	CODEC_ID_SMACKAUDIOPPTV,
	CODEC_ID_QCELPPPTV,
	CODEC_ID_WAVPACKPPTV,
	CODEC_ID_DSICINAUDIOPPTV,
	CODEC_ID_IMCPPTV,
	CODEC_ID_MUSEPACK7PPTV,
	CODEC_ID_MLPPPTV,
	CODEC_ID_GSM_MSPPTV, /* as found in WAV */
	CODEC_ID_ATRAC3PPTV,
	CODEC_ID_VOXWAREPPTV,
	CODEC_ID_APEPPTV,
	CODEC_ID_NELLYMOSERPPTV,
	CODEC_ID_MUSEPACK8PPTV,
	CODEC_ID_SPEEXPPTV,
	CODEC_ID_WMAVOICEPPTV,
	CODEC_ID_WMAPROPPTV,
	CODEC_ID_WMALOSSLESSPPTV,
	CODEC_ID_ATRAC3PPPTV,
	CODEC_ID_EAC3PPTV,
	CODEC_ID_SIPRPPTV,
	CODEC_ID_MP1PPTV,
	CODEC_ID_TWINVQPPTV,
	CODEC_ID_TRUEHDPPTV,
	CODEC_ID_MP4ALSPPTV,
	CODEC_ID_ATRAC1PPTV,
	CODEC_ID_BINKAUDIO_RDFTPPTV,
	CODEC_ID_BINKAUDIO_DCTPPTV,

	/* subtitle codecs */
	CODEC_ID_DVD_SUBTITLEPPTV= 0x17000,
	CODEC_ID_DVB_SUBTITLEPPTV,
	CODEC_ID_TEXTPPTV,  ///< raw UTF-8 text
	CODEC_ID_XSUBPPTV,
	CODEC_ID_SSAPPTV,
	CODEC_ID_MOV_TEXTPPTV,
	CODEC_ID_HDMV_PGS_SUBTITLEPPTV,
	CODEC_ID_DVB_TELETEXTPPTV,

	/* other specific kind of codecs (generally used for attachments) */
	CODEC_ID_TTFPPTV= 0x18000,

	CODEC_ID_PROBEPPTV= 0x19000, ///< codec_id is not known (like CODEC_ID_NONE) but lavf should attempt to identify it

	CODEC_ID_MPEG2TSPPTV= 0x20000, /**< _FAKE_ codec to indicate a raw MPEG-2 TS
							   * stream (only used by libavformat) */
};

typedef struct AVCodecTagPPTV {
	enum CodecIDPPTV id;
	unsigned int tag;
} AVCodecTagPPTV;

const AVCodecTagPPTV ff_mp4_obj_type[] = {
	{ CODEC_ID_MOV_TEXTPPTV  , 0x08 },
	{ CODEC_ID_MPEG4PPTV     , 0x20 },
	{ CODEC_ID_H264PPTV      , 0x21 },
	{ CODEC_ID_AACPPTV       , 0x40 },
	{ CODEC_ID_MP4ALSPPTV    , 0x40 }, /* 14496-3 ALS */
	{ CODEC_ID_MPEG2VIDEOPPTV, 0x61 }, /* MPEG2 Main */
	{ CODEC_ID_MPEG2VIDEOPPTV, 0x60 }, /* MPEG2 Simple */
	{ CODEC_ID_MPEG2VIDEOPPTV, 0x62 }, /* MPEG2 SNR */
	{ CODEC_ID_MPEG2VIDEOPPTV, 0x63 }, /* MPEG2 Spatial */
	{ CODEC_ID_MPEG2VIDEOPPTV, 0x64 }, /* MPEG2 High */
	{ CODEC_ID_MPEG2VIDEOPPTV, 0x65 }, /* MPEG2 422 */
	{ CODEC_ID_AACPPTV       , 0x66 }, /* MPEG2 AAC Main */
	{ CODEC_ID_AACPPTV       , 0x67 }, /* MPEG2 AAC Low */
	{ CODEC_ID_AACPPTV       , 0x68 }, /* MPEG2 AAC SSR */
	{ CODEC_ID_MP3PPTV       , 0x69 }, /* 13818-3 */
	{ CODEC_ID_MP2PPTV       , 0x69 }, /* 11172-3 */
	{ CODEC_ID_MPEG1VIDEOPPTV, 0x6A }, /* 11172-2 */
	{ CODEC_ID_MP3PPTV       , 0x6B }, /* 11172-3 */
	{ CODEC_ID_MJPEGPPTV     , 0x6C }, /* 10918-1 */
	{ CODEC_ID_PNGPPTV       , 0x6D },
	{ CODEC_ID_JPEG2000PPTV  , 0x6E }, /* 15444-1 */
	{ CODEC_ID_VC1PPTV       , 0xA3 },
	{ CODEC_ID_DIRACPPTV     , 0xA4 },
	{ CODEC_ID_AC3PPTV       , 0xA5 },
	{ CODEC_ID_VORBISPPTV    , 0xDD }, /* non standard, gpac uses it */
	{ CODEC_ID_DVD_SUBTITLEPPTV, 0xE0 }, /* non standard, see unsupported-embedded-subs-2.mp4 */
	{ CODEC_ID_QCELPPPTV     , 0xE1 },
	{ CODEC_ID_NONEPPTV      ,    0 },
};


typedef struct {
	int count;
	int duration;
} MOVSttsPPTV;

typedef struct AVCodecContextPPTV
{
	enum AVMediaTypePPTV codec_type; /* see AVMEDIA_TYPE_xxx */
	enum CodecIDPPTV codec_id; /* see CODEC_ID_xxx */
	int  width;
	int  height;
	int  rc_buffer_size;
	int  bit_rate;
	int  rc_max_rate;
	int  rc_min_rate;
};

typedef struct AVRationalPPTV{
	int num; ///< numerator
	int den; ///< denominator
} AVRationalPPTV;
typedef struct MOVIentryPPTV {
	unsigned int	   size;
	unsigned long long     pos;
	unsigned int     samplesInChunk;
	unsigned int     entries;
	int        cts;
	unsigned long long     dts;
#define MOV_SYNC_SAMPLE         0x0001
#define MOV_PARTIAL_SYNC_SAMPLE 0x0002
	unsigned int     flags;
} MOVIentryPPTV;


typedef struct MOVIndexPPTV {
	int         entry;
	unsigned    timescale;
	unsigned long long      time;
	unsigned long long      trackDuration;
	long        sampleCount;
	long        sampleSize;
	int         hasKeyframes;
#define MOV_TRACK_CTTS         0x0001
#define MOV_TRACK_STPS         0x0002
	unsigned int    flags;
	int         language;
	int         trackID;
	int         tag; ///< stsd fourcc
	AVCodecContextPPTV *enc;

	int         vosLen;
	unsigned char      *vosData;
	MOVIentryPPTV   *cluster;
	int         audio_vbr;
	int         height; ///< active picture (w/o VBI) height for D-10/IMX
	unsigned int      tref_tag;
	int         tref_id; ///< trackID of the referenced track

	int         hint_track;   ///< the track that hints this track, -1 if no hint track is set
	int         src_track;    ///< the track that this hint track describes
	unsigned int      prev_rtp_ts;
	unsigned long long      cur_rtp_ts_unwrapped;
	unsigned int      max_packet_size;
} MOVTrack;

typedef struct MOVMuxContextPPTV {
	int     nb_streams;
	//int     chapter_track; ///< qt chapter track number
	//unsigned long long mdat_pos;
	//unsigned long long mdat_size;
	MOVTrack *tracks;
} MOVMuxContextPPTV;

class mp4headInterface
{
public:
	mp4headInterface(void);
	~mp4headInterface(void);

public:
	/*void	Init(ofstream &infile,vector<unsigned int> vFrameOffset,vector<unsigned int> vKeyFrameIndex,
				vector<unsigned char> vFrameType,const char* pAudioConfig,unsigned short sAudioConfigLen,
				const char *pVideoConfig,unsigned short sVideoConfigLen,unsigned short sWidth,unsigned short sHeight,unsigned int iAudioDuration,
				unsigned int iVideoDuration,vector<unsigned short> vVideoDelta, unsigned int iAudioSampleRate, unsigned int iVideoSampleRate, unsigned int lastframelen = 0);*/
	void	Init(FILE *pf,vector<unsigned long long> vFrameOffset,vector<unsigned int> vKeyFrameIndex,
		vector<unsigned char> vFrameType,const char* pAudioConfig,unsigned short sAudioConfigLen,
		const char *pVideoConfig,unsigned short sVideoConfigLen,unsigned short sWidth,unsigned short sHeight,unsigned int iAudioDuration,
		unsigned int iVideoDuration,vector<unsigned short> vVideoDelta, unsigned int iAudioSampleRate, unsigned int iVideoSampleRate, unsigned int lastframelen = 0);
	int		UnInit();
	int		MovWriteMp4Header();

private:
	int    mov_write_ftyp_tag();
	int	   mov_write_moov_tag();
	int	   mov_write_mvhd_tag();
	int	   mov_write_trak_tag(MOVTrack *track);
	int    mov_write_tkhd_tag(MOVTrack *track);
	int    mov_write_mdia_tag(MOVTrack *track);
	int    mov_write_mdhd_tag(MOVTrack *track);
	int    mov_write_hdlr_tag(MOVTrack *track);
	int    mov_write_minf_tag(MOVTrack *track);
	int    mov_write_vmhd_tag();
	int    mov_write_smhd_tag();
	int    mov_write_dinf_tag();
	int	   mov_write_dref_tag();
	int	   mov_write_stbl_tag(MOVTrack *track);
	int    mov_write_stsd_tag(MOVTrack *track);
	int    mov_write_video_tag(MOVTrack *track);
	int    mov_write_avcc_tag(MOVTrack *track);
	int    ff_isom_write_avcc(unsigned char* pData, unsigned int iDataLen);
	int    mov_write_audio_tag(MOVTrack *track);
	int    mov_write_esds_tag(MOVTrack *track); // Basic
	int    mov_write_stts_tag(MOVTrack *track);
	/* Sync sample atom */
	int    mov_write_stss_tag(MOVTrack *track, unsigned int flag);
	int    mov_write_ctts_tag(MOVTrack *track);
	/* Sample to chunk atom */
	int    mov_write_stsc_tag(MOVTrack *track);
	/* Sample size atom */
	int    mov_write_stsz_tag(MOVTrack *track);
	/* Chunk offset atom */
	int    mov_write_stco_tag(MOVTrack *track);

	int    mov_write_mdat_tag();


private:
	void				put_tag(const char *pData);
	unsigned long long	updateSize(unsigned long long pos);
	unsigned long long	av_rescale_rnd(unsigned long long a, unsigned long long b, unsigned long long c, enum AVRoundingPPTV rnd);
	double				av_q2d(AVRationalPPTV a);
	unsigned int		descrLength(unsigned int len);
	void				putDescr(int tag, unsigned int size);
	unsigned int		ff_codec_get_tag(const AVCodecTagPPTV *tags, int id);


private:
	/*ofstream				*m_outfile;*/
	FILE                    *m_pf;
	unsigned int			m_duration;
	MOVMuxContextPPTV		*mov;

	unsigned int			m_iAudioDuration;
	unsigned int			m_iVideoDuration;
};

#endif
