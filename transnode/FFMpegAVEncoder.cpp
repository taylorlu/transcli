#include <stdint.h>
#include <sstream>
#include "FFMpegAVEncoder.h"
#include "bit_osdep.h"
#include "MEvaluater.h"
#include "logger.h"
#include "util.h"
#include "yuvUtil.h"

CFFMpegAVEncoder::CFFMpegAVEncoder(const char* outFileName) : CAVEncoder(outFileName)
{
	m_logModuleType = LOGM_TS_AVENC_FFMPEG;
}

CFFMpegAVEncoder::CFFMpegAVEncoder(void)
{
}


CFFMpegAVEncoder::~CFFMpegAVEncoder(void)
{
}

bool CFFMpegAVEncoder::Initialize()
{
	InitWaterMark();
	std::string cmdLine = getCommandLine();
	m_proc.flags |= SF_LOWER_PRIORITY;

#ifdef DEBUG_EXTERNAL_CMD
	logger_warn(m_logModuleType, "%s\n", cmdLine.c_str());
#endif

	bool success = m_proc.Spawn(cmdLine.c_str()) && m_proc.Wait(200) == 0;

	return success;
}

bool CFFMpegAVEncoder::Stop()
{
	m_proc.Wait(200);
	m_proc.Cleanup();
	return true;
}

std::string CFFMpegAVEncoder::getCommandLine()
{
	std::ostringstream sstr;
	if(!m_bEnableAudio && !m_bEnableVideo) return "";

	sstr << "\"" << FFMPEG << "\" -i \""
		<< m_strSrcFile << "\"";
	if(!m_bEnableAudio) {
		sstr << " -an";
	}
	if(!m_bEnableVideo) {
		sstr << " -vn";
	}

	int startpos = m_pVideoPrefs->GetInt("overall.decoding.startTime");
	int duration = m_pVideoPrefs->GetInt("overall.decoding.duration");
	if (startpos) sstr << " -ss " << startpos / 1000.f;
	if (duration > 0) sstr << " -t " << duration / 1000.f;

	// Video output config
	if(m_bEnableVideo && m_pVideoPrefs) {
		// video filter
		autoCropOrAddBand();
		std::string vfStr = " -vf \"";
		std::stringstream vfOpts;
		int cropMode = m_pVideoPrefs->GetInt("videofilter.crop.mode");
		if (cropMode == 1 || cropMode == 2) {	// Manual or auto crop
			int x = m_pVideoPrefs->GetInt("videofilter.crop.left");
			int y = m_pVideoPrefs->GetInt("videofilter.crop.top");
			int w = m_pVideoPrefs->GetInt("videofilter.crop.width");
			int h = m_pVideoPrefs->GetInt("videofilter.crop.height");
			if (x || y || w || h) {
				vfOpts << "crop=";
				if(w) {
					vfOpts << w;
				} else {
					vfOpts << "iw";
				}
				vfOpts << ':';
				if(h) {
					vfOpts << h;
				} else {
					vfOpts << "ih";
				}
				vfOpts << ':' << x;
				vfOpts << ':' << y <<',';
			}
		}

		// Manual or auto expand before scale
		if (cropMode == 1 || cropMode == 3) {
			int x = m_pVideoPrefs->GetInt("videofilter.expand.x");
			int y = m_pVideoPrefs->GetInt("videofilter.expand.y");
			int w = m_pVideoPrefs->GetInt("videofilter.expand.width");
			int h = m_pVideoPrefs->GetInt("videofilter.expand.height");
			if (x || y || w || h) {
				vfOpts << "pad=" << w;
				vfOpts << ':' << h << ':' << x;
				vfOpts << ':' << y <<',';
				vfOpts <<":black,";
			}
		}

		resolution_t outRes = m_vInfo.res_out;
		if(outRes.width > 0 && outRes.height > 0) {
			vfOpts << "scale=" << outRes.width << ':' << outRes.height << ',';
		}
		std::string vfOptsStr = vfOpts.str();
		if(!vfOptsStr.empty()) {
			if(vfOptsStr.find_last_of(',') == vfOptsStr.length()-1) {
				vfOptsStr.erase(vfOptsStr.length()-1, 1);
			}
			vfStr += vfOptsStr;
			vfStr += "\"";
		} else {
			vfStr = "";
		}
		if(!vfStr.empty()) sstr << vfStr;

		const char *vfmt = GetFFMpegVideoCodecName(m_vInfo.format);
		if (vfmt) sstr << " -vcodec " << vfmt;
		// Video overall setting
		int bitrate = m_pVideoPrefs->GetInt("overall.video.bitrate");
		if(bitrate <= 0) bitrate = 800;
		if (m_pVideoPrefs->GetInt("overall.video.mode") != RC_MODE_VBR) {
			sstr << " -b " <<  bitrate* 1000;
		} else if (m_pVideoPrefs->GetInt("overall.video.format") == VC_H264) {
			sstr << " -cqp " << 31 * (101 - m_pVideoPrefs->GetInt("overall.video.quality")) / 100;
		} else {
			sstr << " -qscale " << 31 * (101 - m_pVideoPrefs->GetInt("overall.video.quality")) / 100;
		}

		if (m_vInfo.dest_dar.num > 0) {
			//sstr << " -aspect " << m_vInfo.dest_dar.num << ':' << m_vInfo.dest_dar.den;
		}

		sstr << " -r " << m_vInfo.fps_out.num << '/' << m_vInfo.fps_out.den;
			//<< " -s " << m_vInfo.res_out.width << 'x' << m_vInfo.res_out.height;

		int b = m_pVideoPrefs->GetInt("videoenc.ffmpeg.vrcMinRate");
		if (b > 0) sstr << " -minrate " << b * 1000;
		b = m_pVideoPrefs->GetInt("videoenc.ffmpeg.vrcMaxRate");
		if (b > 0) sstr << " -maxrate " << b * 1000;
		b = m_pVideoPrefs->GetInt("videoenc.ffmpeg.vrcBufSize");
		if (b > 0) sstr << " -bufsize " << b * 1000;
		sstr<< " -g " << m_pVideoPrefs->GetInt("videoenc.ffmpeg.keyint");

		if (m_pVideoPrefs->GetInt("overall.video.format") == VC_H264) {
			sstr << " -fpre ./codecs/ffpresets/libx264-baseline.ffpreset";
			sstr << " -threads 4";
			//sstr << " -vpre baseline";
		} else {
			sstr << " -me_method ";
			switch (m_pVideoPrefs->GetInt("videoenc.ffmpeg.me")) {
			case 0: sstr << "zero"; break;
			case 1: sstr << "phods"; break;
			case 2: sstr << "log"; break;
			case 3: sstr << "x1"; break;
			case 4: sstr << "hex"; break;
			case 5: sstr << "umh"; break;
			case 6: sstr << "epzs"; break;
			case 7: sstr << "full"; break;
			}
			sstr << " -me_range " << m_pVideoPrefs->GetInt("videoenc.ffmpeg.me_range")
				<< " -mbd " << m_pVideoPrefs->GetInt("videoenc.ffmpeg.mbd");
			int qcomp = m_pVideoPrefs->GetInt("videoenc.ffmpeg.qcomp");
			if(qcomp > 0) sstr<< " -qcomp " << qcomp;
		}
	}

	// Audio output config
	if(m_bEnableAudio && m_pAudioPrefs) {
		const char *afmt = GetFFMpegAudioCodecName(m_aInfo.format);
		if (afmt) sstr << " -acodec " << afmt;
		int bitrate = GetAudioBitrateForAVEnc(m_pAudioPrefs);
		sstr << " -ab " << bitrate*1000;
		if(m_aInfo.out_channels > 0) {
			sstr << " -ac " << m_aInfo.out_channels;
		}
		if(m_aInfo.out_srate > 0) {
			sstr << " -ar " << m_aInfo.out_srate;
		}
	}

	// Overall output config
	if(m_pMuxerPrefs) {
		const char *s = GetFFMpegFormatName(m_pMuxerPrefs->GetInt("overall.container.format"));
		if (s) {
			sstr << " -f " << s;
		}
		if ( (s = m_pMuxerPrefs->GetString("overall.video.fourcc")) && *s) {
			if(m_vInfo.format == VC_XVID) s = "XVID";
			sstr << " -vtag " << s;
		}
		sstr << " -y \"" << m_strOutFile << "\"";
	}

	return sstr.str();
}

void CFFMpegAVEncoder::autoCropOrAddBand()
{
	int cropMode = m_pVideoPrefs->GetInt("videofilter.crop.mode");
	CYuvUtil::FrameRect rect = {0, 0, 0, 0};
	if(m_vInfo.dest_dar.num<=0 || m_vInfo.dest_dar.den<=0) return;
	if(m_vInfo.src_dar.num<=0 || m_vInfo.src_dar.den<=0) return;
	float destDar = (float)m_vInfo.dest_dar.num/m_vInfo.dest_dar.den;
	float srcDar = (float)m_vInfo.src_dar.num/m_vInfo.src_dar.den;

	switch(cropMode) {
	case 2:			// Crop to fit
		rect = CYuvUtil::CalcAutoCrop(srcDar, destDar, m_vInfo.res_in.width, m_vInfo.res_in.height);
		m_pVideoPrefs->SetInt("videofilter.crop.left", rect.x);
		m_pVideoPrefs->SetInt("videofilter.crop.top", rect.y);
		m_pVideoPrefs->SetInt("videofilter.crop.width", rect.w);
		m_pVideoPrefs->SetInt("videofilter.crop.height", rect.h);
		break;
	case 3:			// Expand to fit
		rect = CYuvUtil::CalcAutoExpand(srcDar, destDar, m_vInfo.res_in.width, m_vInfo.res_in.height);
		m_pVideoPrefs->SetBoolean("videofilter.expand.enabled", true);
		m_pVideoPrefs->SetInt("videofilter.expand.x", rect.x);
		m_pVideoPrefs->SetInt("videofilter.expand.y", rect.y);
		m_pVideoPrefs->SetInt("videofilter.expand.width", rect.w);
		m_pVideoPrefs->SetInt("videofilter.expand.height", rect.h);
		break;
	}
}