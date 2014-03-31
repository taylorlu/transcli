#ifndef WIN32
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

#include "bit_osdep.h"

#include <stdint.h>

#include <string>
#include <sstream>

#include "MEvaluater.h"
#include "processwrapper.h"
#include "VideoEncode.h"
#include "AudioEncode.h"
#include "CLIEncoder.h"
#include "logger.h"
#include "util.h"
#include "bitconfig.h"

#define WAIT_CLI_DUR 60000		// Nero aac need long time to write temp file when stop
#define WAIT_ENCODER_START 500

using namespace std;

/***************************************************************************************
CCLIVideoEncoder
***************************************************************************************/
bool CCLIVideoEncoder::Initialize()
{
	InitWaterMark();
	
#ifndef COMMUNICATION_USE_FIFO
	m_proc.flags |= SF_REDIRECT_STDIN;	// Under windows, use std input to communicate with encoder
#else
	m_fdWrite = -1;
	if(makeVideoFifo() != 0) {	// Under linux, use fifo to communicate withe encoder
		return false;
	}
#endif
    m_proc.flags |= SF_LOWER_PRIORITY;

	string cmd = GetCommandLine();
#ifdef DEBUG_EXTERNAL_CMD
	logger_info(LOGM_TS_E_CLI, "%s\n", cmd.c_str());
#endif
	bool success = m_proc.Spawn(cmd.c_str()) && m_proc.Wait(WAIT_ENCODER_START) == 0;
	if(!success) {
		Stop();
		return false;
	}
#ifdef COMMUNICATION_USE_FIFO
	m_fdWrite = open(m_strFifo.c_str(), O_WRONLY);
	if(m_fdWrite < 0) {
		Stop();
		return false;
	}
#endif

	return success;
}

bool CCLIVideoEncoder::IsRunning() 
{
	return m_proc.IsProcessRunning();
}

#ifdef COMMUNICATION_USE_FIFO
int CCLIVideoEncoder::makeVideoFifo()
{
	__int64 curTime = GetTickCount();
	std::stringstream tmpStr;
	tmpStr<<"/tmp/"<<curTime<<"_"<<m_tid<<".yuv";
	m_strFifo = tmpStr.str();
	if(access(m_strFifo.c_str(), F_OK) == -1) {
		return mkfifo(m_strFifo.c_str(), 0777);
	}
	return 0;
}
#endif

int CCLIVideoEncoder::EncodeFrame(uint8_t* pFrameBuf, int bufSize, bool bLast)
{
	int offset = 0;
	if(!pFrameBuf) return VIDEO_ENC_END;				// End encoding

	while (offset < m_frameSizeAfterVf) {
#ifndef COMMUNICATION_USE_FIFO
		int bytes = m_proc.Write(pFrameBuf + offset, m_frameSizeAfterVf - offset);
#else
		int bytes = write(m_fdWrite, pFrameBuf + offset, m_frameSizeAfterVf - offset);
#endif
		if (bytes < 0) {
			return VIDEO_ENC_ERROR;
		}
		offset += bytes;
	}

	if(offset == m_frameSizeAfterVf) {
		m_frameCount++;
	}

	return m_frameCount;
}

bool CCLIVideoEncoder::Stop()
{
#ifndef COMMUNICATION_USE_FIFO
	m_proc.CloseStdin();
#else
	if(m_fdWrite > 0) {
		close(m_fdWrite);
		m_fdWrite = -1;
	}
	logger_info(LOGM_TS_E_CLI, "=====Encoding loop stopped. Close write handle======\n");
#endif

	if(m_proc.Wait(WAIT_CLI_DUR) != 1) {
		logger_err(LOGM_TS_E_CLI, "CLI Video encoder can't end automatically, should force to shut down.\n");
	}
	m_proc.Cleanup();
#ifdef COMMUNICATION_USE_FIFO
	unlink(m_strFifo.c_str());
#endif
	return CVideoEncoder::Stop();
}


/***************************************************************************************
CCLIAudioEncoder
***************************************************************************************/
bool CCLIAudioEncoder::Initialize()
{
#ifndef COMMUNICATION_USE_FIFO
	m_proc.flags |= SF_REDIRECT_STDIN;	// Under windows, use std input to communicate with encoder
#else
	m_fdWrite = -1;
	if(makeAudioFifo() != 0) {	// Under linux, use fifo to communicate withe encoder
		return false;
	}
#endif

	m_proc.flags |= SF_LOWER_PRIORITY;

	string cmd = GetCommandLine();
#ifdef DEBUG_EXTERNAL_CMD
	logger_info(LOGM_TS_E_CLI, "%s.\n", cmd.c_str());
#endif
	bool success = m_proc.Spawn(cmd.c_str()) && m_proc.Wait(500) == 0;
	if(!success) {
		Stop();
		return false;
	}
#ifdef COMMUNICATION_USE_FIFO
	m_fdWrite = open(m_strFifo.c_str(), O_WRONLY);
	if(m_fdWrite < 0) {
		Stop();
		return false;
	}
#endif
	m_bClosed = 0;
	return success;
}

#ifdef COMMUNICATION_USE_FIFO
int CCLIAudioEncoder::makeAudioFifo()
{
	__int64 curTime = GetTickCount();
	std::stringstream tmpStr;
	tmpStr<<"/tmp/"<<curTime<<"_"<<m_tid<<".wav";
	m_strFifo = tmpStr.str();
	if(access(m_strFifo.c_str(), F_OK) == -1) {
		return mkfifo(m_strFifo.c_str(), 0777);
	}
	return 0;
}
#endif

bool CCLIAudioEncoder::IsRunning() 
{
	return m_proc.IsProcessRunning();
}

int64_t CCLIAudioEncoder::EncodeBuffer(uint8_t* pAudioBuf, int bufLen)
{
	int offset = 0;
	while (offset < bufLen && m_proc.IsProcessRunning()) {
#ifndef COMMUNICATION_USE_FIFO
		int bytes = m_proc.Write(pAudioBuf + offset, bufLen - offset);
#else
		int bytes = write(m_fdWrite, pAudioBuf + offset, bufLen - offset);
#endif
		if (bytes < 0) {
			return -1;
		}
		offset += bytes;
	}
	m_encodedBytes += offset;
	return m_encodedBytes;
}

bool CCLIAudioEncoder::Stop()
{
#ifndef COMMUNICATION_USE_FIFO
	m_proc.CloseStdin();
#else
	if(m_fdWrite > 0) {
		close(m_fdWrite);
		m_fdWrite = -1;
	}
#endif
	if(m_proc.Wait(WAIT_CLI_DUR) != 1) {
		logger_err(LOGM_TS_E_CLI, "CLI Audio encoder can't end automatically, should force to shut down.\n");
	}
	
	m_proc.Cleanup();
#ifdef COMMUNICATION_USE_FIFO
	unlink(m_strFifo.c_str());
#endif
	return CAudioEncoder::Stop();
}

/***************************************************************************************
CFFmpegEncoder
***************************************************************************************/

std::string CFFmpegVideoEncoder::GetCommandLine()
{
	std::ostringstream sstr;
	sstr << "\"" << FFMPEG << "\""
		<< " -r " << m_vInfo.fps_out.num << '/' << m_vInfo.fps_out.den
		<< " -s " << m_vInfo.res_out.width << 'x' << m_vInfo.res_out.height
#ifdef WIN32
		<< " -f rawvideo -i pipe:" << m_fdExeRead;
#else
		<< " -f rawvideo -i pipe:%d";
#endif
	sstr << " -v fatal";
	const char *vfmt = GetFFMpegVideoCodecName(m_vInfo.format);
	if (vfmt) sstr << " -c:v " << vfmt;
	m_vInfo.bitrate = m_pXmlPrefs->GetInt("overall.video.bitrate");
	if(m_vInfo.bitrate <= 0) m_vInfo.bitrate = 800;
	if (m_pXmlPrefs->GetInt("overall.video.mode") != RC_MODE_VBR) {
		sstr << " -b:v " <<  m_vInfo.bitrate* 1000;
	} else if (m_pXmlPrefs->GetInt("overall.video.format") == VC_H264) {
		sstr << " -cqp " << 31 * (101 - m_pXmlPrefs->GetInt("overall.video.quality")) / 100;
	} else {
		sstr << " -q:v " << 31 * (101 - m_pXmlPrefs->GetInt("overall.video.quality")) / 100;
	}

	if (m_vInfo.dest_dar.num > 0) {
		sstr << " -aspect " << m_vInfo.dest_dar.num << ':' << m_vInfo.dest_dar.den;
	}
	
	sstr << " -me_method ";
	switch (m_pXmlPrefs->GetInt("videoenc.ffmpeg.me")) {
	case 0:
		sstr << "zero";
		break;
	case 1:
		sstr << "phods";
		break;
	case 2:
		sstr << "log";
		break;
	case 3:
		sstr << "x1";
		break;
	case 4:
		sstr << "hex";
		break;
	case 5:
		sstr << "umh";
		break;
	case 6:
		sstr << "epzs";
		break;
	case 7:
		sstr << "full";
		break;
	}
	int keyint = m_pXmlPrefs->GetInt("videoenc.ffmpeg.keyint");
	if(keyint <= 0) {
		keyint = 10*m_vInfo.fps_out.num/m_vInfo.fps_out.den;
	} else if(keyint < 10) {
		keyint = keyint*m_vInfo.fps_out.num/m_vInfo.fps_out.den;
	}

	sstr << " -me_range " << m_pXmlPrefs->GetInt("videoenc.ffmpeg.me_range")
		<< " -mbd " << m_pXmlPrefs->GetInt("videoenc.ffmpeg.mbd")
		<< " -g " << keyint
		<< " -qcomp " << m_pXmlPrefs->GetFloat("videoenc.ffmpeg.qcomp");

	int bframeNum = m_pXmlPrefs->GetInt("videoenc.ffmpeg.bframes");
	if(bframeNum > 0 && bframeNum <= 4) {
		sstr << " -bf " << bframeNum;
	}

	int subQ = m_pXmlPrefs->GetInt("videoenc.ffmpeg.subq");
	if(subQ > 0 && subQ <= 8) {
		sstr << " -subq " << subQ;
	}

	if(m_pXmlPrefs->GetBoolean("overall.task.interlace")) {
		sstr << " -flags +ilme+ildct";
		if(m_vInfo.interlaced == VID_INTERLACE_TFF) {
			sstr << " -top 1";
		} else if(m_vInfo.interlaced == VID_INTERLACE_BFF) {
			sstr << " -top 0";
		} else {
			sstr << " -top -1";
		}
	}

	int b = m_pXmlPrefs->GetInt("videoenc.ffmpeg.vrcMinRate");
	if (b > 0) sstr << " -minrate " << b * 1000;
	b = m_pXmlPrefs->GetInt("videoenc.ffmpeg.vrcMaxRate");
	if (b > 0) sstr << " -maxrate " << b * 1000;
	b = m_pXmlPrefs->GetInt("videoenc.ffmpeg.vrcBufSize");
	if (b > 0) sstr << " -bufsize " << b * 1000;

	std::string outFormat("rawvideo");
	switch(m_vInfo.format) {
	case VC_H263:
	case VC_H263P:
		outFormat = "3gp";
		break;
	/*case VC_WMV8: 
		outFormat = "asf";
		break;
	case VC_MPEG1:
		outFormat = "mpeg1video";
		break;
	case VC_MPEG2:
		outFormat = "mpeg2video";
		break;
	case VC_MPEG4:
		outFormat = "m4v";
		break;
	case VC_DV:
		outFormat = "dv";
		break;*/
	}

	int containerFormat = m_pXmlPrefs->GetInt("overall.container.format");
	switch (containerFormat) {
	case CF_RTP:
		outFormat= "rtp";
		break;
	case CF_UDP:
		outFormat= "udp";
		break;
	}

	sstr << " -f " << outFormat << " -an \"" << m_strOutFile << "\"";
	return sstr.str();
}

bool CFFmpegVideoEncoder::Initialize()
{
	InitWaterMark();
#ifdef WIN32
	CProcessWrapper::MakePipe(m_fdExeRead, m_fdWriteForExe, m_frameSizeAfterVf * 5, true, false);
#else 
	m_proc.flags |= SF_USE_VIDEO_PIPE;
	m_proc.flags |= SF_INHERIT_READ;
#endif
	
	string cmd = this->GetCommandLine();

	m_proc.flags |=  SF_LOWER_PRIORITY;
#ifdef DEBUG_EXTERNAL_CMD
	logger_info(LOGM_TS_E_CLI, "%s\n", cmd.c_str());
#endif

	bool success = m_proc.Spawn(cmd.c_str()) && m_proc.Wait(500) == 0;

#ifdef WIN32
	_close(m_fdExeRead);
	m_fdExeRead = -1;
#else
	m_fdWriteForExe = m_proc.fdWriteVideo;
#endif
	if (!success) {
		_close(m_fdWriteForExe);
		m_fdWriteForExe = -1;
	}
	return success;
}

int CFFmpegVideoEncoder::EncodeFrame(uint8_t* pFrameBuf, int bufSize, bool bLast)
{
	int writeByts = 0;
	if(!pFrameBuf) return VIDEO_ENC_END;				// End encoding

	if((writeByts = SafeWrite(m_fdWriteForExe, pFrameBuf, m_frameSizeAfterVf)) < 0) {
		return VIDEO_ENC_ERROR;
	}
	
	if(writeByts > 0) {
		m_frameCount++;
	}

	return m_frameCount;
}

bool CFFmpegVideoEncoder::Stop()
{
	if(m_fdWriteForExe >= 0) {
		_close(m_fdWriteForExe);
		m_fdWriteForExe = -1;
	}
	
	if(m_proc.Wait(WAIT_CLI_DUR) != 1) {
		logger_err(LOGM_TS_E_CLI, "FFMpeg Video encoder can't end automatically, should force to shut down.\n");
	}
	m_proc.Cleanup();
	
#ifdef COMMUNICATION_USE_FIFO
	unlink(m_strFifo.c_str());
#endif
	return CVideoEncoder::Stop();
}

/***************************************************************************************
CX264CLIEncoder
***************************************************************************************/
std::string CX264CLIEncoder::GetCommandLine()
{
	std::ostringstream sstr;
	sstr << "\"" << X264_BIN << "\"";

	int n = m_pXmlPrefs->GetInt("videoenc.x264.vbv_maxrate");
	m_vInfo.bitrate = m_pXmlPrefs->GetInt("overall.video.bitrate");
	if(m_vInfo.bitrate <= 0) m_vInfo.bitrate = 800;

	switch (m_pXmlPrefs->GetInt("overall.video.mode")) {
	case RC_MODE_ABR:
		sstr << " --bitrate " << m_vInfo.bitrate;
		if (n > 0) sstr << " --vbv-maxrate " << n;
		break;
	case RC_MODE_VBR:
		sstr << " --crf " << (100 - m_pXmlPrefs->GetInt("overall.video.quality")) / 2;
		if (n > 0) sstr << " --vbv-maxrate " << n;
		break;
	case RC_MODE_CBR:
		sstr << " --bitrate " << m_vInfo.bitrate << " --vbv-maxrate " << m_vInfo.bitrate;
		break;
	case RC_MODE_2PASS:
	case RC_MODE_3PASS:
		sstr << " --bitrate " << m_vInfo.bitrate;
		if (n > 0) sstr << " --vbv-maxrate " << n;
		break;
	}

	static const char* presets[] = { "ultrafast", "superfast", "veryfast", "faster", "fast", "medium", "slow", "slower", "veryslow", "placebo", 0 };
	static const char* tunes[] = { "film", "animation", "grain", "stillimage", "psnr", "ssim", "fastdecode", "zerolatency", 0 };
	int preset = m_pXmlPrefs->GetInt("videoenc.x264.preset");
	if (preset > 0 && presets[preset-1]) {
		sstr << " --preset " << presets[preset-1];
	}
	int tune = m_pXmlPrefs->GetInt("videoenc.x264.tune");
	if (tune > 0 && tunes[tune-1]) {
		sstr << " --tune " << tunes[tune-1];
	}

	n = m_pXmlPrefs->GetInt("videoenc.x264.vbv_bufsize");
	if (n > 0) sstr << " --vbv-bufsize " << n;

	float ratetol = m_pXmlPrefs->GetFloat("videoenc.x264.ratetol");
	if(ratetol > 0.001f) {
		sstr << " --ratetol " << ratetol;
	} else {
		sstr << " --ratetol inf";
	}

	float ipratio = m_pXmlPrefs->GetFloat("videoenc.x264.ipratio");
	if(ipratio > 0) {
		sstr << " --ipratio " << ipratio;
	}
	float pbratio = m_pXmlPrefs->GetFloat("videoenc.x264.pbratio");
	if(pbratio > 0) {
		sstr << " --pbratio " << pbratio;
	}

	int bframes = m_pXmlPrefs->GetInt("videoenc.x264.bframes");
	bool cabac = m_pXmlPrefs->GetBoolean("videoenc.x264.cabac");
	
	int x264Profile = m_pXmlPrefs->GetInt("videoenc.x264.profile");
	switch (x264Profile) {
	case H264_BASE_LINE:
		sstr << " --profile baseline";
		bframes = 0;
		cabac = false;
		break;
	case H264_MAIN:
		sstr << " --profile main";
		break;
	case H264_HIGH:
		sstr << " --profile high";
		break;
	}
	int level = m_pXmlPrefs->GetInt("videoenc.x264.level");
	if (level > 0) sstr << " --level " << level;
	if (!cabac) sstr << " --no-cabac";

	int ifDeblock = m_pXmlPrefs->GetBoolean("videoenc.x264.deblock");
	if(ifDeblock) {
		int deblockAlpha = m_pXmlPrefs->GetInt("videoenc.x264.deblockAlpha");
		int deblockBeta = m_pXmlPrefs->GetInt("videoenc.x264.deblockBeta");
		if(deblockBeta > -7 && deblockBeta < 7 && deblockAlpha > -7 && deblockAlpha < 7) {
			// -6~6
			sstr << " --deblock " << deblockAlpha << ":" << deblockBeta;
		}
	} else {
		sstr << " --no-deblock";
	}
	int threadNum   = m_pXmlPrefs->GetInt("videoenc.x264.threads");
	if(threadNum > 0) sstr << " --threads " << threadNum;
	
	int keyInt = m_pXmlPrefs->GetInt("videoenc.x264.keyint");
	if(keyInt > 0) sstr << " --keyint " << keyInt;
	int keyIntMin = m_pXmlPrefs->GetInt("videoenc.x264.keyint_min");
	if(keyIntMin > 0) sstr << " --min-keyint " << keyIntMin;

    if(preset == 0) {	// Custom setting
		int refFrames = m_pXmlPrefs->GetInt("videoenc.x264.frameref");
		if(refFrames <= 0) refFrames = 1;
		sstr << " --ref " << refFrames;
		if (bframes > 0 && bframes <= 16) sstr << " --bframes " << bframes;
		int meRange = m_pXmlPrefs->GetInt("videoenc.x264.me_range");
		if(meRange >=4 && meRange <= 64) {
			sstr << " --merange " << meRange;
		}
		int scenecutNum = m_pXmlPrefs->GetInt("videoenc.x264.scenecut");
		if(scenecutNum > 0) {
			sstr << " --scenecut " << scenecutNum;
		}
		int subme = m_pXmlPrefs->GetInt("videoenc.x264.subme");
		if(subme >=0 && subme <= 10) {
			sstr << " --subme " << subme;
		}
		int me = m_pXmlPrefs->GetInt("videoenc.x264.me");
		if(me >= 0 && me <= 4) {
			const char* strMe[] = {"dia","hex","umh","esa","tesa"};
			sstr << " --me " << strMe[me];
		}
	}

	if (m_pXmlPrefs->GetBoolean("videoenc.x264.aud"))
		sstr << " --aud";

	const char* s = NULL;
	s = m_pXmlPrefs->GetString("videoenc.x264.colorprim");
	if (s && *s) sstr << " --colorprim " << s;
	s = m_pXmlPrefs->GetString("videoenc.x264.transfer");
	if (s && *s) sstr << " --transfer " << s;
	s = m_pXmlPrefs->GetString("videoenc.x264.colormatrix");
	if (s && *s) sstr << " --colormatrix " << s;

	int slices = m_pXmlPrefs->GetInt("videoenc.x264.slices");
	if (slices > 0) sstr << " --slices " << slices;

	int b_adapt = m_pXmlPrefs->GetInt("videoenc.x264.b_adapt");
	if(b_adapt >= 0 && b_adapt <= 2) {
		sstr << " --b-adapt " << b_adapt;
	}

	int b_bias = m_pXmlPrefs->GetInt("videoenc.x264.b_bias");
	if(b_bias >= 0) {
		sstr << " --b-bias " << b_bias;
	}

	int qpmax = m_pXmlPrefs->GetInt("videoenc.x264.qpmax");
	if(qpmax >=1 && qpmax <= 51) {
		sstr << " --qpmax " << qpmax;
	}
	int qpmin = m_pXmlPrefs->GetInt("videoenc.x264.qpmin");
	if(qpmin >=1 && qpmin <= 51) {
		sstr << " --qpmin " << qpmin;
	}
	int qpstep = m_pXmlPrefs->GetInt("videoenc.x264.qpstep");
	if(qpstep >=0 && qpstep <= 50) {
		sstr << " --qpstep " << qpstep;
	}
	int qcomp = m_pXmlPrefs->GetInt("videoenc.x264.qcomp");
	if(qcomp >= 0 && qcomp <= 100) {
		float fqcomp = qcomp / 100.f;
		sstr << " --qcomp " << fqcomp;
	}

	int directPred = m_pXmlPrefs->GetInt("videoenc.x264.direct_pred");
	if(directPred >= 0 && directPred <= 3) {
		const char* strDirect[] = {"none", "spatial", "temporal", "auto"};
		sstr << " --direct " << strDirect[directPred];
	}

	bool weight_b = m_pXmlPrefs->GetBoolean("videoenc.x264.weight_b");
	if(!weight_b) sstr << " --no-weightb";

	int weight_p = m_pXmlPrefs->GetInt("videoenc.x264.weight_p");
	if(weight_p >= 0 && weight_p <= 2) {
		sstr << " --weightp " << weight_p;
	}
	switch (m_pXmlPrefs->GetInt("videoenc.x264.b_pyramid")) {
	case 0:
		sstr << " --b-pyramid none";
		break;
	case 1:
		sstr << " --b-pyramid strict";
		break;
	case 2:
		sstr << " --b-pyramid normal";
		break;
	}

	switch (m_pXmlPrefs->GetInt("videoenc.x264.nalhrd")) {
	case 1:
		sstr << " --nal-hrd vbr"; break;
	case 2:
		sstr << " --nal-hrd cbr"; break;
	}

	int partitions = m_pXmlPrefs->GetInt("videoenc.x264.partitions");
	if(partitions >= 0 && partitions <= 3) {
		const char* strPartition[] = {"none", "p8x8,i4x4,b8x8", "p8x8,i8x8,i4x4,b8x8", "all"};
		sstr << " --partitions " << strPartition[partitions];
	}

	bool p8x8dct = m_pXmlPrefs->GetBoolean("videoenc.x264.p8x8dct");
	if(!p8x8dct && x264Profile != H264_HIGH) {
		sstr << " --no-8x8dct";
	}

	bool noChromaMe = m_pXmlPrefs->GetBoolean("videoenc.x264.no_choma_me");
	if(noChromaMe) {
		sstr << " --no-chroma-me";
	}

	bool mixRef = m_pXmlPrefs->GetBoolean("videoenc.x264.mixed_refs");
	if(!mixRef) {
		sstr << " --no-mixed-refs";
	}

	bool fastPSkip = m_pXmlPrefs->GetBoolean("videoenc.x264.fast_pskip");
	if(!fastPSkip) {
		sstr << " --no-fast-pskip";
	}

	bool dctDecimate = m_pXmlPrefs->GetBoolean("videoenc.x264.dct_decimate");
	if(!dctDecimate) {
		sstr << " --no-dct-decimate";
	}

	int nr = m_pXmlPrefs->GetInt("videoenc.x264.nr");
	if(nr >= 100 && nr <= 100000) {
		sstr << " --nr " << nr;
	}

	if(cabac) {
		int trellis = m_pXmlPrefs->GetInt("videoenc.x264.trellis");
		if(trellis >= 0 && trellis <= 2) {
			sstr << " --trellis " << trellis;
		}
	}

	bool bPsy = m_pXmlPrefs->GetBoolean("videoenc.x264.psy");
	if(bPsy) {
		float psyRd = m_pXmlPrefs->GetFloat("videoenc.x264.psy_rd");
		float psyTrellis = m_pXmlPrefs->GetFloat("videoenc.x264.psy_trellis");
		sstr << " --psy-rd " << psyRd << ':' << psyTrellis;
	} else {
		sstr << " --no-psy";
	}

	int rcLookAhead = m_pXmlPrefs->GetInt("videoenc.x264.rc_lookahead");
	if(rcLookAhead > 0) {
		sstr << " --rc-lookahead " << rcLookAhead;
	}

	std::string outputTarget = m_strOutFile;
	if(m_encodePass > 1) {
#ifdef WIN32
		outputTarget = "nul";
#else 
		outputTarget = "/dev/null";
#endif
	} else {	// Add "" for file path
		outputTarget.insert(outputTarget.begin(), 1, '"');
		outputTarget.append(1, '"');
	}
	if(m_bMultiPass && m_pPassLogFile) {
		sstr << " --stats \"" << m_pPassLogFile << "\"";
		if(m_encodePass > 1) {
			sstr << " --pass 1";
		}
		else sstr << " --pass 2";
	}

	sstr << " --fps " << m_vInfo.fps_out.num << '/' << m_vInfo.fps_out.den
		<< " --sar " << m_vInfo.dest_par.num << ':' << m_vInfo.dest_par.den
#ifndef COMMUNICATION_USE_FIFO
		<< " -o " << outputTarget << " - "
#else
		<< " -o " << outputTarget << " \"" << m_strFifo << "\" "
#endif
#ifdef USE_NEW_X264
		<< " --input-csp i420  --input-res " 
#endif
		<< m_vInfo.res_out.width << 'x' << m_vInfo.res_out.height;
	
	return sstr.str();
}

/***************************************************************************************
CCudaCLIEncoder
***************************************************************************************/
std::string CCudaCLIEncoder::GetCommandLine()
{
	std::ostringstream sstr;
	sstr << "\"" << CUDAENC_BIN << "\""
		<< " -profile " << m_pXmlPrefs->GetString("videoenc.cuda264.profile")
		<< " -level " << m_pXmlPrefs->GetString("videoenc.cuda264.level")
		<< " -idrp " << m_pXmlPrefs->GetInt("videoenc.cuda264.idr_period")
		<< " -qp " << m_pXmlPrefs->GetInt("videoenc.cuda264.qp")
		<< " -qpp " << m_pXmlPrefs->GetInt("videoenc.cuda264.qpp")
		<< " -qpb " << m_pXmlPrefs->GetInt("videoenc.cuda264.qpb")
		<< " -gop " << m_pXmlPrefs->GetInt("videoenc.cuda264.gop")
		<< " -slicecount " << m_pXmlPrefs->GetInt("videoenc.cuda264.slice")
		<< " -nalframe " << m_pXmlPrefs->GetInt("videoenc.cuda264.nal")
		<< " -pinterval " << (m_pXmlPrefs->GetInt("videoenc.cuda264.bframes") + 1);

	if (m_pXmlPrefs->GetBoolean("videoenc.cuda264.deblock")) sstr << " -deblock";
	if (m_pXmlPrefs->GetBoolean("videoenc.cuda264.intra")) sstr << " -forceintra";
	if (m_pXmlPrefs->GetBoolean("videoenc.cuda264.idr")) sstr << " -forceidr";
	if (m_pXmlPrefs->GetBoolean("videoenc.cuda264.deint")) sstr << " -enabledi";
	if (!m_pXmlPrefs->GetBoolean("videoenc.cuda264.sps")) sstr << " -disablesps";
	if (!m_pXmlPrefs->GetBoolean("videoenc.cuda264.cabac")) sstr << " -disablecabac";
	if(m_pXmlPrefs->GetBoolean("videoenc.cuda264.disableMulti")) sstr << " -disablemulti";
	if(m_pXmlPrefs->GetBoolean("videoenc.cuda264.useGpuMem")) sstr << " -devicemem";
	int offload = m_pXmlPrefs->GetInt("videoenc.cuda264.ol");
	switch(offload) {
	case 0:		// OFFLOAD_DEFAULT
		sstr << " -ol -1"; break;
	case 1:		// OFFLOAD_ESTIMATORS
		sstr << " -ol 8"; break;
	case 2:		// OFFLOAD_ALL
		sstr << " -ol 16"; break;
	}
	int gpuId = m_pXmlPrefs->GetInt("videoenc.cuda264.gpuid");
	if(gpuId >= 0) sstr << " -gpuid " << gpuId;

	m_vInfo.bitrate = m_pXmlPrefs->GetInt("overall.video.bitrate");
	if(m_vInfo.bitrate <= 0) m_vInfo.bitrate = 800;

	switch (m_pXmlPrefs->GetInt("overall.video.mode")) {
	case RC_MODE_ABR:
		sstr << " -rc 0 -abit " << m_vInfo.bitrate << " -pbit " << m_pXmlPrefs->GetInt("videoenc.cuda264.peak");
		break;
	case RC_MODE_VBR:
		sstr << " -rc 1 -pbit " << m_pXmlPrefs->GetInt("videoenc.cuda264.peak");
		break;
	case RC_MODE_CBR:
		sstr << " -rc 2 -abit " << m_vInfo.bitrate << " -pbit " << m_pXmlPrefs->GetInt("videoenc.cuda264.peak");
		break;
	}

	sstr << " -i - -o \"" << m_strOutFile << "\""
		<< " -fpsnum " << m_vInfo.fps_out.num << " -fpsden " << m_vInfo.fps_out.den
		<< " -iw " << m_vInfo.res_in.width << " -ih " << m_vInfo.res_in.height
		<< " -darw " << m_vInfo.dest_dar.num << " -darh " << m_vInfo.dest_dar.den;

	if (m_vInfo.vfType == VF_ENCODER) {
		sstr << " -ow " << m_vInfo.res_out.width
			<< " -oh " << m_vInfo.res_out.height;
		if (m_vInfo.interlaced > 0) sstr << " -enabledi";
	}

	//while (GetTickCount() % 2000 > 1500) Sleep(25);
	//int verifyCode = (int)(GetTickCount()/2000 * 17);
	//sstr << " -verify " << verifyCode;
	return sstr.str();
}

#if defined(HAVE_LINUX)
std::string CMindCLIEncoder::GetCommandLine()
{
	std::ostringstream sstr;
	sstr << "\"" << MINDER_BIN << "\"";

	std::string outputTarget = m_strOutFile;
	if(m_encodePass > 1) {
		outputTarget = "/dev/null";
	} else {	// Add "" for file path
		outputTarget.insert(outputTarget.begin(), 1, '"');
		outputTarget.append(1, '"');
	}

	m_bitrate = m_pXmlPrefs->GetInt("overall.video.bitrate");
	if(m_bitrate <= 0) m_bitrate = 800;
	sstr << " --bitrate " << m_bitrate;
	if(m_bMultiPass) {
		sstr << " --stats \"" << m_passLogFile << "\"";
		if(m_encodePass > 1) sstr << " --pass 1";
		else sstr << " --pass 2";
	}
	sstr << " --fps " << m_vInfo.fps_out.num << '/' << m_vInfo.fps_out.den
		<< " --sar " << m_vInfo.dest_par.num << ':' << m_vInfo.dest_par.den
		<< " -o " << outputTarget << " " << m_strFifo << " "
		<< m_vInfo.res_out.width << 'x' << m_vInfo.res_out.height;
	return sstr.str();
}
#endif

/***************************************************************************************
CFFmpegAudioEncoder
***************************************************************************************/
std::string CFFmpegAudioEncoder::GetCommandLine()
{
	std::ostringstream sstr;
	sstr << "\"" << FFMPEG << "\" -f s16le -ac " << m_aInfo.out_channels
		<< " -ar " << m_aInfo.out_srate
		<< " -i - -v fatal";

	const char *afmt = GetFFMpegAudioCodecName(m_aInfo.format);
	if (afmt) sstr << " -c:a " << afmt;

	switch (m_pXmlPrefs->GetInt("overall.container.format")) {
	case CF_RTP:
		sstr << " -f rtp";
		break;
	case CF_UDP:
		sstr << " -f udp";
		break;
	}

	m_bitrate = m_pXmlPrefs->GetInt("audioenc.ffmpeg.bitrate");
	if(m_bitrate <= 0) m_bitrate = 224;
	
	if(m_aInfo.format == AC_AMR) {
		int amrBitrate = 10200;
		switch(m_bitrate) {
		case 4:
			amrBitrate = 4750;
			break;
		case 5:
			amrBitrate = 5900;
			break;
		case 6:
			amrBitrate = 6700;
			break;
		case 7:
			amrBitrate = 7400;
			break;
		case 10:
			amrBitrate = 10200;
			break;
		case 12:
			amrBitrate = 12200;
			break;
		}
		sstr << " -ab " << amrBitrate;
	} else {
		sstr << " -ab " << m_bitrate*1000;
	}
	
	audio_format_t aacFormat = m_aInfo.format;
	int singleChBitrate = m_bitrate/m_aInfo.out_channels;
	if(singleChBitrate >= 48) {
		aacFormat = AC_AAC_LC;
	}
	if(singleChBitrate <18) {
		aacFormat = AC_AAC_HEV2;
	}

	if(m_aInfo.format == AC_AAC_HE) {
		sstr << " -profile:a aac_he";
	} else if(m_aInfo.format == AC_AAC_HEV2){
		sstr << " -profile:a aac_he_v2";
	}

	sstr << " \"" << m_strOutFile << "\"";	
	return sstr.str();
}

/***************************************************************************************
CFaac
***************************************************************************************/
std::string CFaacCLI::GetCommandLine()
{
	std::ostringstream sstr;
	sstr << "\"" << FAAC_BIN << "\" -P -X -R " << m_aInfo.out_srate
		<< " -B 16 -C " << m_aInfo.out_channels;

	if (m_pXmlPrefs->GetInt("audioenc.faac.mode") == 0) {
		sstr << " -q " << m_pXmlPrefs->GetInt("audioenc.faac.quality");
	} else {
		m_bitrate = m_pXmlPrefs->GetInt("audioenc.faac.bitrate");
		if (m_bitrate <= 0) m_bitrate = 64;
		
		sstr << " -b " << m_bitrate;
	}

	int aacVer = m_pXmlPrefs->GetInt("audioenc.faac.version") == 0 ? 2 : 4;
    int objectType = m_pXmlPrefs->GetInt("audioenc.faac.object");
	switch (objectType) {
	case 0:				// Main
		sstr << " --obj-type Main";
		break;
	case 1:				// Low (LC)
		sstr << " --obj-type LC";
		break;
	case 3:				// LTP
		sstr << " --obj-type LTP";
		aacVer = 4;
		break;
	default:
		sstr << " --obj-type LC";
	}
	sstr << " --mpeg-vers " << aacVer;
	if(m_pXmlPrefs->GetBoolean("audioenc.faac.tns")) {
		sstr << " --tns";
	}
	if(m_pXmlPrefs->GetBoolean("audioenc.faac.noMidSide")) {
		sstr << " --no-midside";
	}

	int blockType = m_pXmlPrefs->GetInt("audioenc.faac.blockType");
	if(blockType != 0 && blockType != 1 && blockType != 2) {
		blockType = 0;
	}
	sstr << " --shortctl " << blockType;
	
	if(m_pXmlPrefs->GetBoolean("audioenc.faac.raw")) {
		sstr << " -r";
	}

	if(m_pXmlPrefs->GetInt("audioenc.faac.container") == 1) {	// Mp4
		sstr << " -w";
	}
#ifndef COMMUNICATION_USE_FIFO
		sstr << " -o \"" << m_strOutFile << "\" -";
#else
		sstr << " -o \"" << m_strOutFile << "\" " << m_strFifo;
#endif

	return sstr.str();
}

/***************************************************************************************
CNeroAudioEncoder
***************************************************************************************/
std::string CNeroAudioEncoder::GetCommandLine()
{
	std::ostringstream sstr;

	sstr << NEROAAC_BIN;
	const char* profileStr = " -lc";
	switch (m_aInfo.format) {
	case AC_AAC_HE:
		profileStr = " -he";
		break;
	case AC_AAC_HEV2:
		profileStr = " -hev2";
		break;
	}

	m_bitrate = m_pXmlPrefs->GetInt("audioenc.nero.bitrate");
	if(m_bitrate <= 0) m_bitrate = 128;
	int bitrateEachChannel = m_bitrate/m_aInfo.out_channels;
	if(bitrateEachChannel >= 48) {
		profileStr = " -lc";
	} else if(bitrateEachChannel >= 12 || m_aInfo.out_channels == 1) {
		profileStr = " -he";
	} else {
		// If bitrate of each channel is less than 5, maybe fail to encoding frame.
		if(bitrateEachChannel <= 4) {
			m_bitrate = 5*m_aInfo.out_channels;		
		}
		profileStr = " -hev2";
	}
	sstr << profileStr;
	switch (m_pXmlPrefs->GetInt("audioenc.nero.mode")) {
	case 0:
		sstr << " -q " << ((float)m_pXmlPrefs->GetInt("audioenc.nero.quality") / 100);
		break;
	case 1:
		sstr << " -br " << m_bitrate*1000;
		break;
	case 2:
		sstr << " -cbr " << m_bitrate*1000;
		break;
	}

#ifndef COMMUNICATION_USE_FIFO
	sstr << " -ignorelength -if - -of \"" << m_strOutFile << "\"";
#else
	sstr << " -ignorelength -if " << m_strFifo << " -of \"" << m_strOutFile << "\"";
#endif
	
	return sstr.str();
}

bool CNeroAudioEncoder::Initialize()
{
	bool success = CCLIAudioEncoder::Initialize();
	if (success) {
		wav_info_t wavhdr;
		wavhdr.riff = 'FFIR';
		wavhdr.file_length = 0x7ffff024;
		wavhdr.wave = 'EVAW';
		wavhdr.fmt = ' tmf';
		wavhdr.fmt_length = 16;
		wavhdr.fmt_tag = 1;
		wavhdr.channels = m_aInfo.out_channels;
		wavhdr.sample_rate = m_aInfo.out_srate;
		wavhdr.block_align = m_aInfo.out_channels * 2;
		wavhdr.bytes_per_second = wavhdr.channels * wavhdr.sample_rate * 2;
		wavhdr.bits = 16;
		wavhdr.data = 'atad';
		wavhdr.data_length = 0x7ffff000;
#ifndef COMMUNICATION_USE_FIFO
		m_proc.Write(&wavhdr, sizeof(wavhdr));
#else
		write(m_fdWrite, &wavhdr, sizeof(wavhdr));
#endif
	}
	return success;
}


