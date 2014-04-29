#include "mp4headInterface.h"
#include "GlobalDefine.h"

#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#define MOV_TIMESCALE 600
#define FFMAX(a,b) ((a) > (b) ? (a) : (b))

#define   TRACK_AUDIO_ID 1
#define   TRACK_VIDEO_ID 0

#define MKTAG(a,b,c,d) (a | (b << 8) | (c << 16) | (d << 24))

mp4headInterface::mp4headInterface(void)
{
	mov = new MOVMuxContextPPTV;
	memset(mov, 0, sizeof(MOVMuxContextPPTV));
}

mp4headInterface::~mp4headInterface(void)
{
#if 1
	if (mov->tracks[TRACK_VIDEO_ID].cluster) {
		delete [](mov->tracks[TRACK_VIDEO_ID].cluster);
		mov->tracks[TRACK_VIDEO_ID].cluster= NULL;
	}

	if (mov->tracks[TRACK_VIDEO_ID].vosData) {
		delete [](mov->tracks[TRACK_VIDEO_ID].vosData);
		mov->tracks[TRACK_VIDEO_ID].vosData = NULL;
	}

	if (mov->tracks[TRACK_VIDEO_ID].enc) {
		delete mov->tracks[TRACK_VIDEO_ID].enc;
		mov->tracks[TRACK_VIDEO_ID].enc = NULL;
	}

	if (mov->tracks[TRACK_AUDIO_ID].cluster) {
		delete [](mov->tracks[TRACK_AUDIO_ID].cluster);
		mov->tracks[TRACK_AUDIO_ID].cluster= NULL;
	}

	if (mov->tracks[TRACK_AUDIO_ID].vosData) {
		delete [](mov->tracks[TRACK_AUDIO_ID].vosData);
		mov->tracks[TRACK_AUDIO_ID].vosData= NULL;
	}

	if (mov->tracks[TRACK_AUDIO_ID].enc) {
		delete mov->tracks[TRACK_AUDIO_ID].enc;
		mov->tracks[TRACK_AUDIO_ID].enc = NULL;
	}	

	if (mov->tracks) {
		delete [](mov->tracks);
		mov->tracks = NULL;
	}	

	if (mov) {
		delete mov;
		mov = NULL;
	}
#endif
}

void mp4headInterface::Init(FILE *pf,vector<unsigned long long> vFrameOffset,vector<unsigned int> vKeyFrameIndex, vector<unsigned char> vFrameType,
							 const char *pAudioConfig,unsigned short sAudioConfigLen, const char *pVideoConfig,unsigned short sVideoConfigLen,unsigned short sWidth,unsigned short sHeight,unsigned int iAudioDuration,
							 unsigned int iVideoDuration,vector<unsigned short> vVideoDelta, unsigned int iAudioSampleRate, unsigned int iVideoSampleRate, unsigned int lastframelen)
{
	m_iAudioDuration = iAudioDuration;
	m_iVideoDuration = iVideoDuration;
	m_pf = pf;
	mov->nb_streams = 2;			//2 tracks

	vector<unsigned int>     vVideoOffset;
	vector<unsigned int>     vAudioOffset;
	vector<unsigned char>      vVideoKeyFrame;
	vector<unsigned char>      vAVFrameType;
	vector<unsigned int>     vAudioFrameSize;
	vector<unsigned int>     vVideoFrameSize;
	for (int i = 0; i<vFrameType.size(); i++)
	{
		vAVFrameType.push_back(vFrameType[i]);
	}
	for (int i = 0; i<vFrameOffset.size(); i++)
	{
		if (vAVFrameType[i] == TRACK_VIDEO_ID)
		{
			vVideoOffset.push_back(vFrameOffset[i]);
			if (i+1 == vFrameOffset.size())
			{
				vVideoFrameSize.push_back(lastframelen);
			}
			else
			{
				if (vAVFrameType[i+1] == TRACK_VIDEO_ID)
				{
					vVideoFrameSize.push_back(vFrameOffset[i+1] - vFrameOffset[i] - 20 );

				}
				else
				{
					vVideoFrameSize.push_back(vFrameOffset[i+1] - vFrameOffset[i] - 17);
				}
			}
			
			unsigned char bKeyFrame = 0;
			for (int j = 0; j<vKeyFrameIndex.size(); j++)
			{
				if (vVideoOffset.size() == vKeyFrameIndex[j])
				{
					bKeyFrame = 1;
					break;
				}
			}
			vVideoKeyFrame.push_back(bKeyFrame);
		}
		else
		{
			vAudioOffset.push_back(vFrameOffset[i]);
			if (i+1 == vFrameOffset.size())
			{
				vAudioFrameSize.push_back(lastframelen);
			}
			else
			{
				if (vAVFrameType[i+1] == TRACK_VIDEO_ID)
				{
					vAudioFrameSize.push_back(vFrameOffset[i+1] - vFrameOffset[i] - 20);
				}
				else
				{
					vAudioFrameSize.push_back(vFrameOffset[i+1] - vFrameOffset[i] - 17);
				}
			}
			
		}
	}

	//¹¹Ôìtrack
	mov->tracks = new MOVTrack[2];
	
	//m_duration = iDuration/1000;
	//for audio
	mov->tracks[TRACK_AUDIO_ID].language = 21956;
	mov->tracks[TRACK_AUDIO_ID].flags = 0;
	
	mov->tracks[TRACK_AUDIO_ID].time = time(0);
	mov->tracks[TRACK_AUDIO_ID].trackID = TRACK_AUDIO_ID+1;
	mov->tracks[TRACK_AUDIO_ID].entry = vAudioOffset.size();
	mov->tracks[TRACK_AUDIO_ID].sampleCount = vAudioOffset.size();
	mov->tracks[TRACK_AUDIO_ID].trackDuration = vAudioOffset.size()*1024;//(UInt64)iAudioDuration*48000/MOV_TIMESCALE;
	mov->tracks[TRACK_AUDIO_ID].timescale = iAudioSampleRate;
    //FIXME or can't play transfered mp4 video
	mov->tracks[TRACK_AUDIO_ID].audio_vbr = 1;

	mov->tracks[TRACK_AUDIO_ID].enc = new AVCodecContextPPTV;
	mov->tracks[TRACK_AUDIO_ID].enc->codec_type = AVMEDIA_TYPE_AUDIOPPTV;
	mov->tracks[TRACK_AUDIO_ID].vosLen = sAudioConfigLen;
	mov->tracks[TRACK_AUDIO_ID].vosData = new unsigned char[sAudioConfigLen];
	memcpy(mov->tracks[TRACK_AUDIO_ID].vosData,pAudioConfig,sAudioConfigLen);
	mov->tracks[TRACK_AUDIO_ID].cluster = new MOVIentryPPTV[vAudioOffset.size()];
	for (int i = 0; i<vAudioOffset.size(); i++)
	{
		mov->tracks[TRACK_AUDIO_ID].cluster[i].samplesInChunk = 1;
		mov->tracks[TRACK_AUDIO_ID].cluster[i].entries = 1;
		mov->tracks[TRACK_AUDIO_ID].cluster[i].size = vAudioFrameSize[i];
		mov->tracks[TRACK_AUDIO_ID].cluster[i].dts = i*1024;//(mov->tracks[TRACK_AUDIO_ID].trackDuration/vAudioOffset.size());
		mov->tracks[TRACK_AUDIO_ID].cluster[i].pos = vAudioOffset[i];
		mov->tracks[TRACK_AUDIO_ID].cluster[i].cts = 0/*iDuration/vAudioFrameSize.size()/2*/;
	}
	mov->tracks[TRACK_AUDIO_ID].tag = MKTAG('m','p','4','a');
	mov->tracks[TRACK_AUDIO_ID].enc->codec_id = CODEC_ID_AACPPTV;
	
	//FIXME Init bit_rate and rc_max_rate, rc_min_rate 
	mov->tracks[TRACK_AUDIO_ID].enc->bit_rate = 0;
	mov->tracks[TRACK_AUDIO_ID].enc->rc_max_rate = 0;
	mov->tracks[TRACK_AUDIO_ID].enc->rc_min_rate = 0;

	//for video
	mov->tracks[TRACK_VIDEO_ID].language = 21956;
	mov->tracks[TRACK_VIDEO_ID].flags = 0;
	mov->tracks[TRACK_VIDEO_ID].flags |= MOV_TRACK_CTTS;
	mov->tracks[TRACK_VIDEO_ID].time = time(0);
	mov->tracks[TRACK_VIDEO_ID].trackID = TRACK_VIDEO_ID+1;
	mov->tracks[TRACK_VIDEO_ID].entry = vVideoOffset.size();
    //XXX FIXME
	mov->tracks[TRACK_VIDEO_ID].hasKeyframes = 1; //XXX
	mov->tracks[TRACK_VIDEO_ID].sampleCount = vVideoOffset.size();
	mov->tracks[TRACK_VIDEO_ID].trackDuration = vVideoOffset.size()*1000;//(UInt64)iVideoDuration*30000/MOV_TIMESCALE;
	mov->tracks[TRACK_VIDEO_ID].timescale = iVideoSampleRate;
	mov->tracks[TRACK_VIDEO_ID].enc = new AVCodecContextPPTV;
	mov->tracks[TRACK_VIDEO_ID].enc->codec_type = AVMEDIA_TYPE_VIDEOPPTV;
	mov->tracks[TRACK_VIDEO_ID].enc->width = sWidth;
	mov->tracks[TRACK_VIDEO_ID].enc->height = sHeight;
    //FIXME
	mov->tracks[TRACK_VIDEO_ID].enc->rc_buffer_size = vKeyFrameIndex.size();

	mov->tracks[TRACK_VIDEO_ID].height = sHeight;
	mov->tracks[TRACK_VIDEO_ID].vosLen = sVideoConfigLen;
	mov->tracks[TRACK_VIDEO_ID].vosData = new unsigned char[sVideoConfigLen];
	memcpy(mov->tracks[TRACK_VIDEO_ID].vosData,pVideoConfig,sVideoConfigLen);
	mov->tracks[TRACK_VIDEO_ID].cluster = new MOVIentryPPTV[vVideoOffset.size()];
	for (int i = 0; i<vVideoOffset.size(); i++)
	{
		mov->tracks[TRACK_VIDEO_ID].cluster[i].samplesInChunk = 1;
		mov->tracks[TRACK_VIDEO_ID].cluster[i].entries = 1;
		mov->tracks[TRACK_VIDEO_ID].cluster[i].size = vVideoFrameSize[i];
		mov->tracks[TRACK_VIDEO_ID].cluster[i].dts = i*1000;
		if (vVideoKeyFrame[i])
		{
			mov->tracks[TRACK_VIDEO_ID].cluster[i].flags = MOV_SYNC_SAMPLE;
		}
		else
		{
			mov->tracks[TRACK_VIDEO_ID].cluster[i].flags = MOV_PARTIAL_SYNC_SAMPLE;
		}
		mov->tracks[TRACK_VIDEO_ID].cluster[i].pos = vVideoOffset[i];
		mov->tracks[TRACK_VIDEO_ID].cluster[i].cts = (unsigned long long)vVideoDelta[i]/(1000/(mov->tracks[TRACK_VIDEO_ID].timescale/1000))*1000;
		//mov->tracks[TRACK_VIDEO_ID].cluster[i].cts = 40;
	}
	mov->tracks[TRACK_VIDEO_ID].tag = MKTAG('a','v','c','1');
	mov->tracks[TRACK_VIDEO_ID].enc->codec_id = CODEC_ID_H264PPTV;
	
	//FIXME Init bit_rate and rc_max_rate, rc_min_rate
	mov->tracks[TRACK_VIDEO_ID].enc->bit_rate = 0;
	mov->tracks[TRACK_VIDEO_ID].enc->rc_max_rate = 0;
	mov->tracks[TRACK_VIDEO_ID].enc->rc_min_rate = 0;

	if (vVideoOffset.size()) {
//		printf("%s, %d\n", __func__, __LINE__);
		vVideoOffset.clear();
	}

	if (vAudioOffset.size()) {
//		printf("%s, %d\n", __func__, __LINE__);
		vAudioOffset.clear();
	}

	if (vVideoKeyFrame.size()) {
//		printf("%s, %d\n", __func__, __LINE__);
		vVideoKeyFrame.clear();
	}

	if (vAVFrameType.size()) {
//		printf("%s, %d\n", __func__, __LINE__);
		vAVFrameType.clear();
	}

	if (vAudioFrameSize.size()) {
//		printf("%s, %d\n", __func__, __LINE__);
		vAudioFrameSize.clear();
	}

	if (vVideoFrameSize.size()) {
//		printf("%s, %d\n", __func__, __LINE__);
		vVideoFrameSize.clear();
	}

}

int mp4headInterface::UnInit()
{
	if (mov->tracks[TRACK_VIDEO_ID].cluster) {
		delete [](mov->tracks[TRACK_VIDEO_ID].cluster);
//		printf("%s, %d\n", __func__, __LINE__);
		mov->tracks[TRACK_VIDEO_ID].cluster= NULL;
	}

	if (mov->tracks[TRACK_VIDEO_ID].vosData) {
		delete [](mov->tracks[TRACK_VIDEO_ID].vosData);
//		printf("%s, %d\n", __func__, __LINE__);
		mov->tracks[TRACK_VIDEO_ID].vosData = NULL;
	}

	if (mov->tracks[TRACK_VIDEO_ID].enc) {
//		printf("%s, %d\n", __func__, __LINE__);
		delete mov->tracks[TRACK_VIDEO_ID].enc;
		mov->tracks[TRACK_VIDEO_ID].enc = NULL;
	}

	if (mov->tracks[TRACK_AUDIO_ID].cluster) {
//		printf("%s, %d\n", __func__, __LINE__);
		delete [](mov->tracks[TRACK_AUDIO_ID].cluster);
		mov->tracks[TRACK_AUDIO_ID].cluster= NULL;
	}

	if (mov->tracks[TRACK_AUDIO_ID].vosData) {
//		printf("%s, %d\n", __func__, __LINE__);
		delete [](mov->tracks[TRACK_AUDIO_ID].vosData);
		mov->tracks[TRACK_AUDIO_ID].vosData= NULL;
	}

	if (mov->tracks[TRACK_AUDIO_ID].enc) {
//		printf("%s, %d\n", __func__, __LINE__);
		delete mov->tracks[TRACK_AUDIO_ID].enc;
		mov->tracks[TRACK_AUDIO_ID].enc = NULL;
	}	

	if (mov->tracks) {
		delete [](mov->tracks);
		mov->tracks = NULL;
	}
	return 0;
}

int   mp4headInterface::MovWriteMp4Header()
{
	mov_write_ftyp_tag();
	mov_write_moov_tag();
	mov_write_mdat_tag();
	return 0;
}


int   mp4headInterface::mov_write_ftyp_tag()
{
	//int minor = 0x200;
	int minor = 1;
	//unsigned long long pos = (*m_outfile).tellp();//RWtell(m_infile);
	unsigned long long pos = ftell(m_pf);
	Write32(m_pf,0);
	put_tag("ftyp");
	put_tag("isom");

	Write32(m_pf, minor);
	put_tag("isom");
	//put_tag("iso2");
	put_tag("avc1");
	//put_tag("mp41");
	return updateSize(pos);
}

int mp4headInterface::mov_write_moov_tag()
{
	int i;
	//unsigned long long pos = (*m_outfile).tellp();//RWtell(m_infile);
	unsigned long long pos = ftell(m_pf);
	Write32( m_pf, 0); /* size placeholder*/
	put_tag("moov");
	mov_write_mvhd_tag();
	for (i=0; i<mov->nb_streams; i++) {
		if(mov->tracks[i].entry > 0) {
			mov_write_trak_tag(&(mov->tracks[i]));
		}
	}

	return updateSize(pos);
}

int    mp4headInterface::mov_write_mvhd_tag()
{
	int maxTrackID = 1, i;
	unsigned long long maxTrackLenTemp, maxTrackLen = 0;
	int version;

#if 0
	for (i=0; i<mov->nb_streams; i++) {
		if(mov->tracks[i].entry > 0) {
			maxTrackLenTemp = av_rescale_rnd(mov->tracks[i].trackDuration,
				MOV_TIMESCALE,
				mov->tracks[i].timescale,
				AV_ROUND_UP);
			if(maxTrackLen < maxTrackLenTemp)
				maxTrackLen = maxTrackLenTemp;
			if(maxTrackID < mov->tracks[i].trackID)
				maxTrackID = mov->tracks[i].trackID;
		}
	}
#endif

	maxTrackLen = m_iAudioDuration>m_iVideoDuration?m_iAudioDuration/1000*600:m_iVideoDuration/1000*600;


	version = maxTrackLen < UINT32_MAX ? 0 : 1;
	(version == 1) ? Write32(m_pf, 120) : Write32(m_pf, 108); /* size */
	put_tag("mvhd");
	Write8(m_pf, version);
	Write24(m_pf, 0); /* flags */

	Write32(m_pf, time(0)); /* creation time */
	Write32(m_pf, time(0)); /* modification time */
	
	Write32(m_pf, MOV_TIMESCALE);
	(version == 1) ? Write64(m_pf, maxTrackLen) : Write32(m_pf, maxTrackLen); /* duration of longest track */

	Write32(m_pf, 0x00010000); /* reserved (preferred rate) 1.0 = normal */
	Write16(m_pf, 0x0100); /* reserved (preferred volume) 1.0 = normal */
	Write16(m_pf, 0); /* reserved */
	Write32(m_pf, 0); /* reserved */
	Write32(m_pf, 0); /* reserved */

	/* Matrix structure */
	Write32(m_pf, 0x00010000); /* reserved */
	Write32(m_pf, 0x0); /* reserved */
	Write32(m_pf, 0x0); /* reserved */
	Write32(m_pf, 0x0); /* reserved */
	Write32(m_pf, 0x00010000); /* reserved */
	Write32(m_pf, 0x0); /* reserved */
	Write32(m_pf, 0x0); /* reserved */
	Write32(m_pf, 0x0); /* reserved */
	Write32(m_pf, 0x40000000); /* reserved */

	Write32(m_pf, 0); /* reserved (preview time) */
	Write32(m_pf, 0); /* reserved (preview duration) */
	Write32(m_pf, 0); /* reserved (poster time) */
	Write32(m_pf, 0); /* reserved (selection time) */
	Write32(m_pf, 0); /* reserved (selection duration) */
	Write32(m_pf, 0); /* reserved (current time) */
	maxTrackID = 2;
	Write32(m_pf, maxTrackID+1); /* Next track id */
	return 0x6c;
}

int mp4headInterface::mov_write_trak_tag(MOVTrack *track)
{
	//unsigned long long pos = (*m_outfile).tellp();//RWtell(m_infile);
	unsigned long long pos = ftell(m_pf);
	Write32(m_pf, 0); /* size */
	put_tag("trak");
	mov_write_tkhd_tag(track);
	mov_write_mdia_tag(track);
	return updateSize(pos);
}

int mp4headInterface::mov_write_tkhd_tag(MOVTrack *track)
{
#if 0
	unsigned long long duration = av_rescale_rnd(track->trackDuration, MOV_TIMESCALE,
		track->timescale, AV_ROUND_UP);
	int version = duration < INT32_MAX ? 0 : 1;
#endif
	int version = track->trackDuration < INT32_MAX?0:1;


	(version == 1) ? Write32(m_pf, 104) : Write32(m_pf, 92); /* size */
	put_tag("tkhd");
	Write8(m_pf, version);
	Write24(m_pf, 0x000001); /* flags (track enabled) */
	if (version == 1) {
		Write64(m_pf, track->time);
		Write64(m_pf, track->time);
	} else {
		Write32(m_pf, track->time); /* creation time */
		Write32(m_pf, track->time); /* modification time */
	}
	Write32(m_pf, track->trackID); /* track-id */
	Write32(m_pf, 0); /* reserved */
#if 0
(version == 1) ? Write64(m_rwops, duration) : Write32(m_rwops, duration);
#endif

	unsigned long long duration = 0;
	if(track->enc->codec_type == AVMEDIA_TYPE_AUDIOPPTV)
	{
		 duration = m_iAudioDuration/1000*600;
	}
	else
	{
		 duration = m_iVideoDuration/1000*600;
	}
	(version == 1) ? Write64(m_pf, duration) : Write32(m_pf, duration);

	Write32(m_pf, 0); /* reserved */
	Write32(m_pf, 0); /* reserved */
	Write32(m_pf, 0x0); /* reserved (Layer & Alternate group) */
	/* Volume, only for audio */
	if(track->enc->codec_type == AVMEDIA_TYPE_AUDIOPPTV)
		Write16(m_pf, 0x0100);
	else
		Write16(m_pf, 0);
	Write16(m_pf, 0); /* reserved */

	/* Matrix structure */
	Write32(m_pf, 0x00010000); /* reserved */
	Write32(m_pf, 0x0); /* reserved */
	Write32(m_pf, 0x0); /* reserved */
	Write32(m_pf, 0x0); /* reserved */
	Write32(m_pf, 0x00010000); /* reserved */
	Write32(m_pf, 0x0); /* reserved */
	Write32(m_pf, 0x0); /* reserved */
	Write32(m_pf, 0x0); /* reserved */
	Write32(m_pf, 0x40000000); /* reserved */

	/* Track width and height, for visual only */
	if((track->enc->codec_type == AVMEDIA_TYPE_VIDEOPPTV ||
		track->enc->codec_type == AVMEDIA_TYPE_SUBTITLEPPTV)) {
			double sample_aspect_ratio = 0;
			if(!sample_aspect_ratio || track->height != track->enc->height)
				sample_aspect_ratio = 1;
			Write32(m_pf, sample_aspect_ratio * track->enc->width*0x10000);
			Write32(m_pf, track->height*0x10000);
	}
	else {
		Write32(m_pf, 0);
		Write32(m_pf, 0);
	}
	return 0x5c;
}

int mp4headInterface::mov_write_mdia_tag(MOVTrack *track)
{
	//unsigned long long pos = (*m_outfile).tellp();//RWtell(m_infile);
	unsigned long long pos = ftell(m_pf);
	Write32(m_pf, 0); /* size */
	put_tag("mdia");
	mov_write_mdhd_tag(track);
	mov_write_hdlr_tag(track);
	mov_write_minf_tag(track);
	return updateSize(pos);
}

int mp4headInterface::mov_write_mdhd_tag(MOVTrack *track)
{
	int version = track->trackDuration < INT32_MAX ? 0 : 1;

	(version == 1) ? Write32(m_pf, 44) : Write32(m_pf, 32); /* size */
	put_tag( "mdhd");
	Write8(m_pf, version);
	Write24(m_pf, 0); /* flags */
	if (version == 1) {
		Write64(m_pf, track->time);
		Write64(m_pf, track->time);
	} else {
		Write32(m_pf, track->time); /* creation time */
		Write32(m_pf, track->time); /* modification time */
	}
	Write32(m_pf,track->timescale); /* time scale (sample rate for audio) */
	(version == 1) ? Write64(m_pf, track->trackDuration) : Write32(m_pf, track->trackDuration); /* duration */
	Write16(m_pf, track->language); /* language */
	Write16(m_pf, 0); /* reserved (quality) */
	return 32;
}

int mp4headInterface::mov_write_hdlr_tag(MOVTrack *track)
{
	const char *hdlr, *descr = NULL, *hdlr_type = NULL;
	//unsigned long long pos = (*m_outfile).tellp();//RWtell(m_infile);
	unsigned long long pos = ftell(m_pf);
	if (!track) { /* no media --> data handler */
		hdlr = "dhlr";
		hdlr_type = "url ";
		descr = "DataHandler";
	} else {
		hdlr =  "\0\0\0\0";
		if (track->enc->codec_type == AVMEDIA_TYPE_VIDEOPPTV) {
			hdlr_type = "vide";
			descr = "VideoHandler";
		} else if (track->enc->codec_type == AVMEDIA_TYPE_AUDIOPPTV) {
			hdlr_type = "soun";
			descr = "SoundHandler";
		} 
	}

	Write32(m_pf, 0); /* size */
	put_tag("hdlr");
	Write32(m_pf, 0); /* Version & flags */
	WriteBytes(m_pf, CharToString(hdlr,4), 4); /* handler */
	put_tag(hdlr_type); /* handler type */
	Write32(m_pf ,0); /* reserved */
	Write32(m_pf ,0); /* reserved */
	Write32(m_pf ,0); /* reserved */
	WriteBytes(m_pf, CharToString(descr,strlen(descr)), strlen(descr)); /* handler description */
	if (track)
		Write8(m_pf, 0); /* c string */
	return updateSize(pos);
}

int mp4headInterface::mov_write_minf_tag(MOVTrack *track)
{
	//unsigned long long pos = (*m_outfile).tellp();//RWtell(m_infile);
	unsigned long long pos = ftell(m_pf);
	Write32(m_pf, 0); /* size */
	put_tag("minf");
	if(track->enc->codec_type == AVMEDIA_TYPE_VIDEOPPTV)
		mov_write_vmhd_tag();
	else if (track->enc->codec_type == AVMEDIA_TYPE_AUDIOPPTV)
		mov_write_smhd_tag();

	mov_write_dinf_tag();
	mov_write_stbl_tag(track);
	return updateSize(pos);
}

int mp4headInterface::mov_write_vmhd_tag()
{
	Write32(m_pf, 0x14); /* size (always 0x14) */
	put_tag("vmhd");
	Write32(m_pf, 0x01); /* version & flags */
	Write64(m_pf, 0); /* reserved (graphics mode = copy) */
	return 0x14;
}

int mp4headInterface::mov_write_smhd_tag()
{
	Write32(m_pf, 16); /* size */
	put_tag("smhd");
	Write32(m_pf, 0); /* version & flags */
	Write16(m_pf, 0); /* reserved (balance, normally = 0) */
	Write16(m_pf, 0); /* reserved */
	return 16;
}


int mp4headInterface::mov_write_dinf_tag()
{
	//unsigned long long pos = (*m_outfile).tellp();//RWtell(m_infile);
	unsigned long long pos = ftell(m_pf);
	Write32(m_pf, 0); /* size */
	put_tag("dinf");
	mov_write_dref_tag();
	return updateSize(pos);
}

int mp4headInterface::mov_write_dref_tag()
{
	Write32(m_pf, 28); /* size */
	put_tag("dref");
	Write32(m_pf, 0); /* version & flags */
	Write32(m_pf, 1); /* entry count */

	Write32(m_pf, 0xc); /* size */
	put_tag("url ");
	Write32(m_pf, 1); /* version & flags */

	return 28;
}

int mp4headInterface::mov_write_stbl_tag(MOVTrack *track)
{
	//unsigned long long pos = (*m_outfile).tellp();//RWtell(m_infile);
	unsigned long long pos = ftell(m_pf);
	Write32(m_pf, 0); /* size */
	put_tag("stbl");
	mov_write_stsd_tag(track);
	mov_write_stts_tag(track);
   // unsigned long long tmp_pos = (*m_outfile).tellp();//RWtell(m_infile);
    unsigned long long tmp_pos = ftell(m_pf);
	//printf("111111 = %llu\n", tmp_pos);
    //printf("track->hasKeyframes = %d, track->entry = %d\n", track->hasKeyframes, track->entry);
	if (track->enc->codec_type == AVMEDIA_TYPE_VIDEOPPTV &&
		track->hasKeyframes && (track->hasKeyframes < track->entry)) {
    //	FIXME track->hasKeyframes = 0 in linux!!!
	//if (track->enc->codec_type == AVMEDIA_TYPE_VIDEO &&
	//	(track->hasKeyframes < track->entry)) {
		mov_write_stss_tag(track, MOV_SYNC_SAMPLE);
    }
	if (track->enc->codec_type == AVMEDIA_TYPE_VIDEOPPTV &&
		track->flags & MOV_TRACK_CTTS)
    {
		mov_write_ctts_tag(track);
    }
	mov_write_stsc_tag(track);
	mov_write_stsz_tag(track);
	mov_write_stco_tag(track);
	return updateSize(pos);
}




int mp4headInterface::mov_write_stsd_tag(MOVTrack *track)
{
	//unsigned long long pos = (*m_outfile).tellp();//RWtell(m_infile);
	unsigned long long pos = ftell(m_pf);
	Write32(m_pf, 0); /* size */
	put_tag("stsd");
	Write32(m_pf, 0); /* version & flags */
	Write32(m_pf, 1); /* entry count */
	if (track->enc->codec_type == AVMEDIA_TYPE_VIDEOPPTV)
		mov_write_video_tag(track);
	else if (track->enc->codec_type == AVMEDIA_TYPE_AUDIOPPTV)
		mov_write_audio_tag(track);
	return updateSize(pos);
}


int mp4headInterface::mov_write_video_tag(MOVTrack *track)
{
	//unsigned long long pos = (*m_outfile).tellp();//RWtell(m_infile);
	unsigned long long pos = ftell(m_pf);
	char compressor_name[32];

	Write32(m_pf, 0); /* size */
	WriteL32(m_pf, track->tag); // store it byteswapped
	Write32(m_pf, 0); /* Reserved */
	Write16(m_pf, 0); /* Reserved */
	Write16(m_pf, 1); /* Data-reference index */

	Write16(m_pf, 0); /* Codec stream version */
	Write16(m_pf, 0); /* Codec stream revision (=0) */

	Write32(m_pf, 0); /* Reserved */
	Write32(m_pf, 0); /* Reserved */
	Write32(m_pf, 0); /* Reserved */

	Write16(m_pf, track->enc->width); /* Video width */
	Write16(m_pf, track->height); /* Video height */
	Write32(m_pf, 0x00480000); /* Horizontal resolution 72dpi */
	Write32(m_pf, 0x00480000); /* Vertical resolution 72dpi */
	Write32(m_pf, 0); /* Data size (= 0) */
	Write16(m_pf, 1); /* Frame count (= 1) */

	memset(compressor_name,0,32);

	Write8(m_pf, strlen(compressor_name));
	WriteBytes(m_pf, CharToString(compressor_name,31), 31);

	Write16(m_pf, 0x18); /* Reserved */
	Write16(m_pf, 0xffff); /* Reserved */

	mov_write_avcc_tag(track);

	return updateSize( pos);
}

int mp4headInterface::mov_write_avcc_tag(MOVTrack *track)
{
	//unsigned long long pos = (*m_outfile).tellp();//RWtell(m_infile);
	unsigned long long pos = ftell(m_pf);
	Write32(m_pf, 0);
	put_tag("avcC");
	ff_isom_write_avcc(track->vosData, track->vosLen);
	return updateSize(pos);
}

int mp4headInterface::mov_write_audio_tag(MOVTrack *track)
{
	//unsigned long long pos = (*m_outfile).tellp();//RWtell(m_infile);
	unsigned long long pos = ftell(m_pf);
	int version = 0;
	unsigned int tag = track->tag;


	Write32(m_pf, 0); /* size */
	WriteL32(m_pf, tag); // store it byteswapped
	Write32(m_pf, 0); /* Reserved */
	Write16(m_pf, 0); /* Reserved */
	Write16(m_pf, 1); /* Data-reference index, XXX  == 1 */

	/* SoundDescription */
	Write16(m_pf, version); /* Version */
	Write16(m_pf, 0); /* Revision level */
	Write32(m_pf, 0); /* Reserved */

	
		
	Write16(m_pf, 2);
	Write16(m_pf, 16);
	Write16(m_pf, 0);
		

	Write16(m_pf, 0); /* packet size (= 0) */
	Write16(m_pf, track->timescale); /* Time scale */
	Write16(m_pf, 0); /* Reserved */

	mov_write_esds_tag(track);

	return updateSize(pos);
}

int mp4headInterface::mov_write_esds_tag(MOVTrack *track) // Basic
{
	//unsigned long long pos = (*m_outfile).tellp();//RWtell(m_infile);
	unsigned long long pos = ftell(m_pf);
	int decoderSpecificInfoLen = track->vosLen ? descrLength(track->vosLen):0;

	Write32(m_pf, 0); // size
	put_tag("esds");
	Write32(m_pf, 0); // Version

	// ES descriptor
	putDescr( 0x03, 3 + descrLength(13 + decoderSpecificInfoLen) +
		descrLength(1));
	Write16(m_pf, track->trackID);
	Write8(m_pf, 0x00); // flags (= no flags)

	// DecoderConfig descriptor
	putDescr(0x04, 13 + decoderSpecificInfoLen);

	// Object type indication
	Write8(m_pf, ff_codec_get_tag(ff_mp4_obj_type, track->enc->codec_id));

	//Write8(m_rwops,40);

	// the following fields is made of 6 bits to identify the streamtype (4 for video, 5 for audio)
	// plus 1 bit to indicate upstream and 1 bit set to 1 (reserved)
	if(track->enc->codec_type == AVMEDIA_TYPE_AUDIOPPTV)
		Write8(m_pf, 0x15); // flags (= Audiostream)
	else
		Write8(m_pf, 0x11); // flags (= Visualstream)

    //unsigned long long tmp_pos = (*m_outfile).tellp();//RWtell(m_infile);
	unsigned long long tmp_pos = ftell(m_pf);
    //printf("88888888 tmp_pos = %llu, track->enc->rc_buffer_size = %d\n", tmp_pos, track->enc->rc_buffer_size);
	Write8(m_pf,  track->enc->rc_buffer_size>>(3+16));    // Buffersize DB (24 bits)
	Write16(m_pf, (track->enc->rc_buffer_size>>3)&0xFFFF); // Buffersize DB

	Write32(m_pf, FFMAX(track->enc->bit_rate, track->enc->rc_max_rate)); // maxbitrate (FIXME should be max rate in any 1 sec window)
	if(track->enc->rc_max_rate != track->enc->rc_min_rate || track->enc->rc_min_rate==0)
		Write32(m_pf, 0); // vbr
	else
		Write32(m_pf, track->enc->rc_max_rate); // avg bitrate

	if (track->vosLen) {
		// DecoderSpecific info descriptor
		putDescr(0x05, track->vosLen);
		WriteBytes(m_pf, CharToString((char*)track->vosData,track->vosLen), track->vosLen);
	}

	// SL descriptor
	putDescr(0x06, 1);
	Write8(m_pf,0x02);
	return updateSize(pos);
}

int mp4headInterface::mov_write_stts_tag(MOVTrack *track)
{
	MOVSttsPPTV *stts_entries;
	unsigned int entries = -1;
	unsigned int atom_size;
	int i;
#ifdef TEST
	entries = 1;
	stts_entries = (MOVStts *)malloc(sizeof(*stts_entries));
	stts_entries[0].count = track->sampleCount;
	stts_entries[0].duration = 1000;
#else
    //FIXME audio_vbr is no inited!
	if (track->enc->codec_type == AVMEDIA_TYPE_AUDIOPPTV && !track->audio_vbr) {
		stts_entries = (MOVSttsPPTV *)malloc(sizeof(*stts_entries)); /* one entry */
		stts_entries[0].count = track->sampleCount;
		stts_entries[0].duration = 1024;
		entries = 1;
	} else {
		stts_entries = (MOVSttsPPTV *)malloc(track->entry * sizeof(*stts_entries)); /* worst case */
		for (i=0; i<track->entry; i++) {
			unsigned long long duration = i + 1 == track->entry ?
				track->trackDuration - track->cluster[i].dts + track->cluster[0].dts : /* readjusting */
			track->cluster[i+1].dts - track->cluster[i].dts;
			if (i && (int)duration == stts_entries[entries].duration) {
				stts_entries[entries].count++; /* compress */
			} else {
				entries++;
				stts_entries[entries].duration = duration;
				stts_entries[entries].count = 1;
			}
		}
		entries++; /* last one */
	}
#endif


	atom_size = 16 + (entries * 8);
	Write32(m_pf,atom_size); /* size */
	put_tag("stts");
	Write32(m_pf,0); /* version & flags */
	Write32(m_pf,entries); /* entry count */
	for (i=0; i<entries; i++) {
		Write32(m_pf, stts_entries[i].count);
		Write32(m_pf, stts_entries[i].duration);
        //printf("222222 entries = %d, stts_entries[i].count = %d, stts_entries[i].duration = %d\n", entries, stts_entries[i].count, stts_entries[i].duration);
	}
	free(stts_entries);
	return atom_size;
}

/* Sync sample atom */
int mp4headInterface::mov_write_stss_tag(MOVTrack *track, unsigned int flag)
{
	unsigned long long curpos, entryPos;
	int i, index = 0;
	//unsigned long long pos = (*m_outfile).tellp();//RWtell(m_infile);
	unsigned long long pos = ftell(m_pf);
	Write32(m_pf, 0); // size
	put_tag(flag == MOV_SYNC_SAMPLE ? "stss" : "stps");
	Write32(m_pf, 0); // version & flags
	//entryPos = (*m_outfile).tellp();//RWtell(m_infile);
	entryPos = ftell(m_pf);
	Write32(m_pf, track->entry); // entry count
	for (i=0; i<track->entry; i++) {
		if (track->cluster[i].flags & flag) {
			Write32(m_pf, i+1);
			index++;
		}
	}
	//curpos = (*m_outfile).tellp();//RWtell(m_infile);
	curpos = ftell(m_pf);
	//m_outfile->seekp(entryPos);//RWseek(m_outfile, entryPos, SEEK_SET);
	fseek(m_pf, entryPos, SEEK_SET);
	Write32(m_pf, index); // rewrite size
	//m_outfile->seekp(curpos);//RWseek(m_outfile, curpos, SEEK_SET);
	fseek(m_pf, curpos, SEEK_SET);
	return updateSize(pos);
}

int mp4headInterface::mov_write_ctts_tag(MOVTrack *track)
{
	MOVSttsPPTV *ctts_entries;
	unsigned int entries = 0;
	unsigned int atom_size;
	int i;

	ctts_entries = (MOVSttsPPTV *)malloc((track->entry + 1) * sizeof(*ctts_entries)); /* worst case */
	ctts_entries[0].count = 1;
	ctts_entries[0].duration = track->cluster[0].cts;
	for (i=1; i<track->entry; i++) {
		if (track->cluster[i].cts == ctts_entries[entries].duration) {
			ctts_entries[entries].count++; /* compress */
		} else {
			entries++;
			ctts_entries[entries].duration = track->cluster[i].cts;
			ctts_entries[entries].count = 1;
		}
	}
	entries++; /* last one */
	atom_size = 16 + (entries * 8);
	Write32(m_pf, atom_size); /* size */
	put_tag("ctts");
	Write32(m_pf, 0); /* version & flags */
	Write32(m_pf, entries); /* entry count */
	for (i=0; i<entries; i++) {
		Write32(m_pf, ctts_entries[i].count);
		Write32(m_pf, ctts_entries[i].duration);
	}
	free(ctts_entries);
	return atom_size;
}


/* Sample to chunk atom */
int mp4headInterface::mov_write_stsc_tag(MOVTrack *track)
{
	int index = 0, oldval = -1, i;
	unsigned long long entryPos, curpos;

	//unsigned long long pos = (*m_outfile).tellp();//RWtell(m_infile);
	unsigned long long pos = ftell(m_pf);
	Write32(m_pf, 0); /* size */
	put_tag("stsc");
	Write32(m_pf, 0); // version & flags
	//entryPos = (*m_outfile).tellp();//RWtell(m_infile);
	entryPos = ftell(m_pf);
	Write32(m_pf, track->entry); // entry count
	for (i=0; i<track->entry; i++) {
		if(oldval != track->cluster[i].samplesInChunk)
		{
			Write32(m_pf, i+1); // first chunk
			Write32(m_pf, track->cluster[i].samplesInChunk); // samples per chunk
			Write32(m_pf, 0x1); // sample description index
			oldval = track->cluster[i].samplesInChunk;
			index++;
		}
	}
//	curpos = (*m_outfile).tellp();//RWtell(m_infile);
	curpos = ftell(m_pf);
	//m_outfile->seekp(entryPos);//RWseek(m_outfile, entryPos, SEEK_SET);
	fseek(m_pf, entryPos, SEEK_SET);
	Write32(m_pf, index); // rewrite size
	//m_outfile->seekp(curpos);//RWseek(m_outfile, curpos, SEEK_SET);
	fseek(m_pf, curpos, SEEK_SET);
	return updateSize(pos);
}

/* Sample size atom */
int mp4headInterface::mov_write_stsz_tag(MOVTrack *track)
{
	int equalChunks = 1;
	int i, j, entries = 0, tst = -1, oldtst = -1;

	//unsigned long long pos = (*m_outfile).tellp();//RWtell(m_infile);
	unsigned long long pos = ftell(m_pf);
	Write32(m_pf, 0); /* size */
	put_tag("stsz");
	Write32(m_pf, 0); /* version & flags */

	for (i=0; i<track->entry; i++) {
		tst = track->cluster[i].size/track->cluster[i].entries;
		if(oldtst != -1 && tst != oldtst) {
			equalChunks = 0;
		}
		oldtst = tst;
		entries += track->cluster[i].entries;
	}
	if (equalChunks) {
		int sSize = track->cluster[0].size/track->cluster[0].entries;
		Write32(m_pf, sSize); // sample size
		Write32(m_pf, entries); // sample count
	}
	else {
		Write32(m_pf, 0); // sample size
		Write32(m_pf, entries); // sample count
		for (i=0; i<track->entry; i++) {
			for (j=0; j<track->cluster[i].entries; j++) {
				Write32(m_pf, track->cluster[i].size /
					track->cluster[i].entries);
			}
		}
	}
	return updateSize(pos);
}

/* Chunk offset atom */
int mp4headInterface::mov_write_stco_tag(MOVTrack *track)
{
	int i;
	int mode64 = 0; //   use 32 bit size variant if possible
	//unsigned long long pos = (*m_outfile).tellp();//RWtell(m_infile);
	unsigned long long pos = ftell(m_pf);
	Write32(m_pf, 0); /* size */
	if (pos > UINT32_MAX) {
		mode64 = 1;
		put_tag("co64");
	} else
		put_tag("stco");
	Write32(m_pf, 0); /* version & flags */
	Write32(m_pf, track->entry); /* entry count */
	for (i=0; i<track->entry; i++) {
		if(mode64 == 1)
			Write64(m_pf, track->cluster[i].pos);
		else
			Write32(m_pf, track->cluster[i].pos);
	}
	return updateSize(pos);
}

int mp4headInterface::mov_write_mdat_tag()
{
	Write32(m_pf, 8);    // placeholder for extended size field (64 bit)
	put_tag("free");

//	mov->mdat_pos = m_infile.tellp();//RWtell(m_rwops);
	//unsigned long long pos = (*m_outfile).tellp();//RWtell(m_infile);
	unsigned long long pos = ftell(m_pf);
	Write32(m_pf, 0); /* size placeholder*/
	put_tag("mdat");
	//RWseek(m_rwops,0,SEEK_END);
	updateSize(pos);
	return 0;
}

unsigned int mp4headInterface::ff_codec_get_tag(const AVCodecTagPPTV *tags, int id)
{
	while (tags->id != CODEC_ID_NONEPPTV) {
		if (tags->id == id)
			return tags->tag;
		tags++;
	}
	return 0;
}

int mp4headInterface::ff_isom_write_avcc(unsigned char* pData, unsigned int iLen)
{
	WriteBytes(m_pf,CharToString((char*)pData,iLen),iLen);
	return iLen;
}

unsigned int mp4headInterface::descrLength(unsigned int len)
{
	int i;
	for(i=1; len>>(7*i); i++);
	return len + 1 + i;
}

void mp4headInterface::putDescr(int tag, unsigned int size)
{
	int i= descrLength(size) - size - 2;
	Write8(m_pf,tag);
	for(; i>0; i--)
		Write8(m_pf,(size>>(7*i)) | 0x80);
	Write8(m_pf,size & 0x7F);
}


void   mp4headInterface::put_tag(const char *pData)
{
	while (*pData) {
		Write8(m_pf, *pData++);
	}
}


unsigned long long mp4headInterface::updateSize(unsigned long long pos)
{
//	unsigned long long curpos = (*m_outfile).tellp();//RWtell(m_infile);
	unsigned long long curpos = ftell(m_pf);
	//m_outfile->seekp(pos);//RWseek(m_outfile,pos,SEEK_SET);
	fseek(m_pf, pos, SEEK_SET);
	Write32(m_pf, curpos - pos); /* rewrite size */
	//m_outfile->seekp(curpos);//RWseek(m_outfile, curpos, SEEK_SET);
	fseek(m_pf, curpos, SEEK_SET);
	return curpos - pos;
}

unsigned long long mp4headInterface::av_rescale_rnd(unsigned long long a, unsigned long long b, unsigned long long c, enum AVRoundingPPTV rnd){
	unsigned long long r=0;
	/*assert(c > 0);
	assert(b >=0);
	assert(rnd >=0 && rnd<=5 && rnd!=4);*/

	if(a<0 && a != INT64_MIN) return -av_rescale_rnd(-a, b, c, (AVRoundingPPTV)(rnd ^ ((rnd>>1)&1)));

	if(rnd==AV_ROUND_NEAR_INFPPTV) r= c/2;
	else if(rnd&1)             r= c-1;

	if(b<=INT_MAX && c<=INT_MAX){
		if(a<=INT_MAX)
			return (a * b + r)/c;
		else
			return a/c*b + (a%c*b + r)/c;
	}else{
#if 1
		unsigned long long a0= a&0xFFFFFFFF;
		unsigned long long a1= a>>32;
		unsigned long long b0= b&0xFFFFFFFF;
		unsigned long long b1= b>>32;
		unsigned long long t1= a0*b1 + a1*b0;
		unsigned long long t1a= t1<<32;
		int i;

		a0 = a0*b0 + t1a;
		a1 = a1*b1 + (t1>>32) + (a0<t1a);
		a0 += r;
		a1 += a0<r;

		for(i=63; i>=0; i--){
			//            int o= a1 & 0x8000000000000000ULL;
			a1+= a1 + ((a0>>i)&1);
			t1+=t1;
			if(/*o || */c <= a1){
				a1 -= c;
				t1++;
			}
		}
		return t1;
	}
#else
		AVInteger ai;
		ai= av_mul_i(av_int2i(a), av_int2i(b));
		ai= av_add_i(ai, av_int2i(r));

		return av_i2int(av_div_i(ai, av_int2i(c)));
	}
#endif
}


double mp4headInterface::av_q2d(AVRationalPPTV a){
	return a.num / (double) a.den;
}
