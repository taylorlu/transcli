#include <stdio.h>
#include <sstream>
#ifdef WIN32
#include <io.h>
#endif

#include "DecoderMplayer.h"
#include "logger.h"
#include "MEvaluater.h"
#include "util.h"
#include "bit_osdep.h"
#include "bitconfig.h"


using namespace std;

CDecoderMplayer::CDecoderMplayer(void)
{
}

CDecoderMplayer::~CDecoderMplayer(void)
{
}

std::string CDecoderMplayer::GetCmdString(const char* mediaFile)
{
	ostringstream cmd;
	cmd << "\"" << MPLAYER << "\" \"" << mediaFile << "\" -quiet -nomsgcolor -benchmark";

	// Cut movie
	CXMLPref* pPref = m_pAudioPref;
	if(!pPref) pPref = m_pVideoPref;
	if(pPref) {
		int startpos = pPref->GetInt("overall.decoding.startTime");
		int duration = pPref->GetInt("overall.decoding.duration");
		if (startpos > 0) cmd << " -ss " << startpos / 1000.f;
		if (duration > 0) cmd << " -endpos " << duration / 1000.f;
	}

	if(m_bDecAudio) {		// Read PCM data and create audio encoders
#ifdef WIN32
		cmd << " -ao pcm:waveheader:fast:pipe=$(fdAudioWrite)";		// holder of audio write handle
#else
		cmd << " -ao pcm:waveheader:fast:pipe=%d" ;
#endif
		// 5.1->2 Downmix is not correct in decoders
		int channelEnum = m_pAudioPref->GetInt("overall.audio.channels");
		int inchs = m_pAInfo->in_channels;
		//if(inchs > 6) inchs = 6;	// if channels > 6, ex. 8 chs
		if(m_pAInfo->in_channels == 6 ) {
			cmd << " -channels " << inchs;
		}
		/*if(channelEnum == 0 && m_pAInfo->in_channels > 2) {		// Use original channels
			cmd << " -channels " << m_pAInfo->in_channels;
		} else if(m_pAInfo->srcformat == AC_DTS) {
			cmd << " -channels " << m_pAInfo->in_channels;
		}*/

		// Audio filter
		std::string audioFilterStr = GenAudioFilterOptions();
		if(!audioFilterStr.empty() ){
			if(*audioFilterStr.begin() == ',') {
				audioFilterStr.erase(audioFilterStr.begin());
			}
			cmd << " -af " << audioFilterStr;
		}
		if(m_pAInfo && m_pAInfo->aid >= 0) {
			cmd << " -aid " << m_pAInfo->aid;
		}
	} else {
#ifdef WIN32
		cmd << " -nosound";
#endif
	}
	
	int cachesize = 0;
	if(m_pVideoPref && strstr(mediaFile, "://")) {
		cachesize = m_pVideoPref->GetInt("videosrc.mplayer.cache");
	}
	if (cachesize > 0) {
		cmd << " -cache " << cachesize;
	}

	if(m_bDecVideo) {
#ifdef WIN32
		cmd << " -vo yuvpipe:fd=$(fdVideoWrite)"; 
#else
		cmd << " -fd %d";
#endif
		if(m_bEnableVf && m_pVInfo) {
			resolution_t resOut = m_pVInfo->res_out;
			if(resOut.width > 0 && resOut.height > 0) {
				cmd << " -vf scale=" << resOut.width << ':' << resOut.height;
			}
		}
	} else {
		if(m_pAInfo->srcformat == AC_MP3 || m_pAInfo->srcformat == AC_MP2) {
			cmd << " -vc null -vo null";
		} else {	// Mpg(Mpeg2-Mp2) will fail with "-vc dummy"
			cmd << " -vc dummy -vo null";
		}
		
	}
	return cmd.str();
}


std::string CDecoderMplayer::GenAudioFilterOptions()
{
	ostringstream opts;
	int i;
	
	if (m_pAudioPref->GetBoolean("audiofilter.equalizer.enabled")) {
		opts << ",equalizer=";
		for (i = 0; i <= 9; i++) {
			char buf[32] = {0};
			sprintf(buf, "audiofilter.equalizer.band%d", i);
			opts << m_pAudioPref->GetInt(buf) << ':';
		}
	}
	if (m_pAudioPref->GetBoolean("audiofilter.channels.enabled")) {
		int nr = m_pAudioPref->GetInt("audiofilter.channels.routes");
		opts << ",channels=" << m_pAudioPref->GetInt("audiofilter.channels.output") << ':' << nr;
		for (i=0; i < nr; i++) {
			char buf[48] = {0};
			_snprintf(buf, sizeof(buf), "audiofilter.channels.channel%d", i + 1);
			opts << ':' << (m_pAudioPref->GetInt(buf) - 1) << ':' << i;
		}
	} else {
		int channelEnum = m_pAudioPref->GetInt("overall.audio.channels");
		if(channelEnum == 0) {		// original channel processing
			if(m_pAInfo->out_channels == 1) {
				channelEnum = 1;
			} else if (m_pAInfo->out_channels == 2) {
				channelEnum = 3;
			} else if(m_pAInfo->out_channels > 0) {
				channelEnum = m_pAInfo->out_channels;
			}
		} else if(channelEnum > 2 && m_pAInfo->out_channels == 1) {
			// If output is set to more than 2 Ch, but audio source is only 1 Ch
			channelEnum = 1;
		}
		switch (channelEnum) {
		case 1:
			opts << ",channels=1:1:0:0";
			break;
		case 2:
			opts << ",channels=1:1:1:0";
			break;
		case 3:
			if(/*m_pAInfo->srcformat == AC_DTS && */m_pAInfo->in_channels == 6) {
				opts << ",pan=2:0.5:0:0:0.5:0.5:0:0:0.5:0.9:0.9:0.3:0.3";
			} else {
				opts << ",channels=2";
			}
			break;
		case 4:
			opts << ",channels=4";
			break;
		case 5:
			opts << ",channels=5";
			break;
		case 6:
			opts << ",channels=6";
			break;
		}
	}

	if(m_bEnableAf) {
		int srate = m_pAInfo->out_srate;
		if (srate > 0 /*&& (srate != m_pAInfo->in_srate || m_bForceResample)*/) {
			if (m_pAudioPref->GetBoolean("audiofilter.resample.lavcResampler")) {
				opts << ",lavcresample=" << srate;
			} else {
				opts << ",resample=" << srate << ':' << (m_pAudioPref->GetInt("audiofilter.resample.precious") ? 0 : 1) << ':' << m_pAudioPref->GetInt("audiofilter.resample.method");
			}
		}
	}

	float f = m_pAudioPref->GetFloat("audiofilter.surround.delay");
	if (f) {
		opts << ",surround=" << f;
	}
	if (m_pAudioPref->GetBoolean("audiofilter.delay.enabled")) {
		opts << ",delay=";
		for (i = 0; i < 6; i++) {
			char buf[32] = {0};
			sprintf(buf, "audiofilter.delay.channel%d", i + 1);
			opts << m_pAudioPref->GetInt(buf) << ':';
		}
	}
	if (m_pAudioPref->GetBoolean("audiofilter.extraStereo.enabled")) {
		opts << ",extrastereo=" << m_pAudioPref->GetFloat("audiofilter.extraStereo.coeff");
	}

	int n;
	if ((n = m_pAudioPref->GetInt("audiofilter.volume.normalization"))) {
		opts << ",volnorm=" << n << ':' << m_pAudioPref->GetFloat("audiofilter.volume.targetAmplitude");
	} 

	float volGain = m_pAudioPref->GetFloat("audiofilter.volume.gain");
	volGain += m_pAInfo->volGain;
	if (volGain > 0.9f || volGain < -0.9f) {
		int clipping = m_pAudioPref->GetBoolean("audiofilter.volume.clipping") ? 1:0;
		opts << ",volume=" << volGain << ':' << clipping;
	}

	if (m_pAudioPref->GetBoolean("audiofilter.compressor.enabled")) {
		opts << ",comp";
	}

	return opts.str();
}
