#include <sstream>
#include <stdio.h>
#ifdef WIN32
#include <io.h>
#include <shlobj.h>
#endif

#include "DecoderMencoder.h"
#include "logger.h"
#include "MEvaluater.h"
#include "FileQueue.h"
#include "bitconfig.h"
#include "bit_osdep.h"

#include "util.h"
#include "yuvUtil.h"
#include "WorkManager.h"	// Get cpu core num

using namespace std;

CDecoderMencoder::CDecoderMencoder(void)
{
}

CDecoderMencoder::~CDecoderMencoder(void)
{
	Cleanup();
}

std::string CDecoderMencoder::GenAudioFilterOptions()
{
	std::ostringstream opts;
	int i;
	
	if (m_pAudioPref->GetBoolean("audiofilter.equalizer.enabled")) {
		opts << ",equalizer=";
		for (i = 0; i <= 9; i++) {
			char buf[32] = {0};
			sprintf(buf, "audiofilter.equalizer.band%d", i);
			opts << m_pAudioPref->GetInt(buf) << ':';
		}
	}

	int mixOption = m_pAudioPref->GetInt("audiofilter.channels.mix");
	if(mixOption > 0 && m_pAInfo->in_channels == 2) {
		switch(mixOption) {
		case 2:		// Left(mono)
			opts << ",pan=1:0.5:0.5"; break;
		default:	// Stereo
			opts << ",pan=2:0.5:0.5:0.5:0.5"; break;
		}
	} else if (m_pAudioPref->GetBoolean("audiofilter.channels.enabled")) {
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
	if (!FloatEqual(f, 0)) {
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

std::string CDecoderMencoder::GenVideoFilterOptions()
{
	std::stringstream opts;
	std::string subopts;
	int i;
	//float inFps = 0;
	//if(m_pVInfo->fps_in.den > 0) inFps = (float)m_pVInfo->fps_in.num / m_pVInfo->fps_in.den;

	// Add multiple delogo filter (with time and position)
	if (m_pVideoPref->GetBoolean("videofilter.delogo.enabled")) {
		int delogoThick = m_pVideoPref->GetInt("videofilter.delogo.thickness");
		if(delogoThick <= 0) delogoThick = 4;
		int delogoNum = m_pVideoPref->GetInt("videofilter.delogo.num");
		
		for (int i=0; i<delogoNum; ++i) {
			char posKey[50] = {0};
			sprintf(posKey, "videofilter.delogo.pos%d", i+1);
			const char* timePosStr = m_pVideoPref->GetString(posKey);
			std::vector<int> vctVal;
			float delogoStart= 0.f, delogoEnd = 0.f;
			int delogoX = 0, delogoY = 0, delogoW = 0, delogoH = 0;
			if(StrPro::StrHelper::parseStringToNumArray(vctVal, timePosStr)) {
				if(vctVal.size() != 6) continue;
				delogoStart = vctVal.at(0)/1000.f;	// ms to sec
				delogoEnd = vctVal.at(1)/1000.f;	// ms to sec
				delogoX = vctVal.at(2);
				delogoY = vctVal.at(3);
				delogoW = vctVal.at(4);
				delogoH = vctVal.at(5);
			}

			//int startFrame = (int)(delogoStart*inFps+0.5f);
			//if(startFrame == 0) startFrame = 1;
			//int endFrame = (int)(delogoEnd*inFps+0.5f);
			if(delogoStart > -0.1f && delogoEnd > -0.1f && delogoStart<delogoEnd) continue;
			opts << "delogo=" << delogoX << ':' 
				<< delogoY << ':' << delogoW << ':'
				<< delogoH << ':' << delogoThick << ':'
				<< delogoStart << ':' << delogoEnd << ',';
		}
	}

	if(m_pVInfo->vfType != VF_ENCODER) {
		bool needDeint = false;
		switch (m_pVideoPref->GetInt("videofilter.deint.mode")) {
		case 2:
			if (m_pVInfo->interlaced != VID_PROGRESSIVE) {
				needDeint = true;
			}
			break;
		case 1:
			needDeint = true; break;
		}

		if(m_pVideoPref->GetBoolean("overall.task.interlace")) {
			needDeint = false;
		}

		if (needDeint) {
			const char *p;
			switch (m_pVideoPref->GetInt("videofilter.deint.algorithm")) {
			case 0: opts << "pp=lb"; break;
			case 1: opts << "pp=li"; break;
			case 2: opts << "pp=ci"; break;
			case 3: opts << "pp=md"; break;
			case 4: opts << "pp=l5"; break;
			case 5: opts << "dint"; break;
			case 6:
				opts << "kerndeint";
				if ((p = m_pVideoPref->GetString("videofilter.deint.kerndeint")) && *p)
					opts << '=' << p;
				break;			
			case 7: 
				{
					int fieldOrder = m_pVideoPref->GetInt("videofilter.deint.fieldOrder");
					if(fieldOrder == 0) {
						if(m_pVInfo->interlaced == VID_INTERLACE_TFF) {
							fieldOrder = 1;
						} else if(m_pVInfo->interlaced == VID_INTERLACE_BFF) {
							fieldOrder = 0;
						}
					} else if(fieldOrder == 2) {
						fieldOrder = 0;
					}
					opts << "yadif=" << m_pVideoPref->GetInt("videofilter.deint.yadif");
					opts << ":" << fieldOrder;
				}
				break;
			case 8:
				opts << "mcdeint=" << m_pVideoPref->GetInt("videofilter.deint.mcdeint");
				break;
			}
			opts << ',';

			// Interlace is done by decoder, then encoder no need to do
			if(m_bLastPass) m_pVInfo->interlaced = 0;	
		}
	}

	if(m_bEnableVf || m_pVInfo->vfType != VF_ENCODER) {
		bool enableExpand = m_pVideoPref->GetBoolean("videofilter.expand.enabled");
		int cropMode = m_pVideoPref->GetInt("videofilter.crop.mode");
		if (cropMode == 1 || cropMode == 2 || cropMode == 4) {	// Manual or auto crop
			int x = m_pVideoPref->GetInt("videofilter.crop.left");
			int y = m_pVideoPref->GetInt("videofilter.crop.top");
			int w = m_pVideoPref->GetInt("videofilter.crop.width");
			int h = m_pVideoPref->GetInt("videofilter.crop.height");
			if (w || h) {
				opts << "crop=";
				if(w) opts << w;	
				opts << ':';
				if(h) opts << h;
				if(x >= 0) opts << ':' << x;
				if(y >= 0) opts << ':' << y;
				opts <<',';
			}
		}

		resolution_t resOut = m_pVInfo->res_out;
		resolution_t resIn = m_pVInfo->res_in;
		int scaleW = resOut.width, scaleH = resOut.height;
		if (scaleW > 0 && scaleH > 0) {
			if ((cropMode == 1 || cropMode == 3) && enableExpand) {		// scale and then expand
				fraction_t inDar = m_pVInfo->src_dar;
				fraction_t outDar = m_pVInfo->dest_dar;
				if(inDar.den > 0 && outDar.den > 0) {
					if(outDar.num/(float)outDar.den	> inDar.num/(float)inDar.den) {
						scaleW = scaleH*inDar.num/inDar.den;
						EnsureMultipleOfDivisor(scaleW, 2);
					} else {
						scaleH = scaleW*inDar.den/inDar.num;
						EnsureMultipleOfDivisor(scaleH, 2);
					}
				}
			} 
			opts << "scale=" << scaleW << ':' << scaleH;
			int interlaceScale = 0;
			int chromaSkip = m_pVideoPref->GetInt("videofilter.scale.chroma");
			if(chromaSkip >= 1 && chromaSkip <= 3) {
				opts << ':' << interlaceScale << ':' << chromaSkip;
			}
			opts << ',';
		}

		// Expand after scale
		if (enableExpand) {
			int x = m_pVideoPref->GetInt("videofilter.expand.x");
			int y = m_pVideoPref->GetInt("videofilter.expand.y");
			int w = m_pVideoPref->GetInt("videofilter.expand.width");
			int h = m_pVideoPref->GetInt("videofilter.expand.height");
			fraction_t dar = m_pVInfo->dest_dar;
			if(dar.den == 0 || dar.num == 0) {
				dar = m_pVInfo->src_dar;
			}
			if(dar.den != 0 && dar.num != 0) {
				if(w <= 0 && h > 0) {
					w = h*dar.num/dar.den;
					EnsureMultipleOfDivisor(w, 4);
				}
				if(h <= 0 && w > 0) {
					h = w*dar.den/dar.num;
					EnsureMultipleOfDivisor(h, 4);
				}
			} 

			if (w || h) {
				opts << "expand=";
				if(w) opts << w;
				opts << ':';
				if(h) opts << h;
				if(x >= 0) opts << ':' << x;
				if(y >= 0) opts << ':' << y;
				opts << ',';

				// Change video width and height
				m_pVInfo->res_in.width = w;
				m_pVInfo->res_in.height = h;
				m_pVInfo->res_out.width = w;
				m_pVInfo->res_out.height = h;
			}
		}
	} 

	if(m_bEnableVf) {
		switch (m_pVideoPref->GetInt("videofilter.itf.type")) {
		case 1: opts << "detc,"; break;
		case 2: opts << "pullup,"; break;
		}

		switch (m_pVideoPref->GetInt("videofilter.postproc.level")) {
		case 1: subopts += "/al"; break;
		case 2: subopts += "/al:f"; break;
		}
		if (m_pVideoPref->GetBoolean("videofilter.postproc.hdeblock")) subopts += "/ha";
		if (m_pVideoPref->GetBoolean("videofilter.postproc.vdeblock")) subopts += "/va";
		if (m_pVideoPref->GetBoolean("videofilter.postproc.dering")) subopts += "/dr";
		if (m_pVideoPref->GetInt("videofilter.denoise.mode") == 1) subopts += "/tn:1:2:3";
		if (!subopts.empty()) {
			opts << "pp=" << subopts.substr(1) << ',';
		}

		//if(m_pVInfo->fps_out.den > 0 && m_pVInfo->fps_in.den > 0) {
		//	if(inFps - m_pVInfo->fps_out.num/(float)m_pVInfo->fps_out.den > 1) {
		opts << "softskip,";
			//}
		//}
	
		/*
		if (m_pVideoPref->GetBoolean("videofilter.scale.dsize")) {
		int dw = m_pVideoPref->GetInt("videofilter.scale.dw");
		int dh = m_pVideoPref->GetInt("videofilter.scale.dh");
		if (dw && dh)
		opts << "dsize=" << dw << ':' << dh << ',';
		} else if (m_pVideoPref->GetBoolean("overall.video.dar")) {
		int darw = m_pVideoPref->GetInt("overall.video.darw");
		int darh = m_pVideoPref->GetInt("overall.video.darh");
		if (darw && darh) opts << ",dsize=" << darw << '/' << darh << ',';
		}

		if (m_pVideoPref->GetInt("videofilter.unsharp.mode") > 0 && (m_pVideoPref->GetBoolean("videofilter.unsharp.luma") || m_pVideoPref->GetBoolean("videofilter.unsharp.chroma"))) {
		float amount = (float) m_pVideoPref->GetInt("videofilter.unsharp.amount") * (m_pVideoPref->GetInt("videofilter.unsharp.mode") == 1 ? 1 : -1) / 100;
		opts << "unsharp=";
		if (m_pVideoPref->GetBoolean("videofilter.unsharp.luma")) {
		opts << "l" << m_pVideoPref->GetInt("videofilter.unsharp.width") << 'x' << m_pVideoPref->GetInt("videofilter.unsharp.height") << ':' << amount << ',';
		}
		if (m_pVideoPref->GetBoolean("videofilter.unsharp.chroma")) {
		opts << "c" << m_pVideoPref->GetInt("videofilter.unsharp.width") << 'x' << m_pVideoPref->GetInt("videofilter.unsharp.height") << ':' << amount << ',';
		}
		}

		if (m_pVideoPref->GetBoolean("videofilter.expand.enabled")) {
		opts << "expand=";
		if ((i = m_pVideoPref->GetInt("videofilter.expand.width")) != 0) opts << i;
		opts << ':';
		if ((i = m_pVideoPref->GetInt("videofilter.expand.height")) != 0) opts << i;
		opts << ':';
		if ((i = m_pVideoPref->GetInt("videofilter.expand.x")) != 0) opts << i;
		opts << ':';
		if ((i= m_pVideoPref->GetInt("videofilter.expand.y")) != 0) opts << i;
		if (m_pVideoPref->GetBoolean("videofilter.expand.osd")) opts << ":1";
		opts << ',';
		}*/

		if ((i = m_pVideoPref->GetInt("videofilter.denoise.mode")) > 1) {
			opts << (i == 2 ? "denoise3d" : "hqdn3d") << '='
				<< m_pVideoPref->GetInt("videofilter.denoise.luma") << ':'
				<< m_pVideoPref->GetInt("videofilter.denoise.chroma") << ':'
				<< m_pVideoPref->GetInt("videofilter.denoise.strength") << ',';
		}

		// Image equality
		int vContrast = m_pVideoPref->GetInt("videofilter.eq.contrast");
		int vBright = m_pVideoPref->GetInt("videofilter.eq.brightness");
		int vSaturation = m_pVideoPref->GetInt("videofilter.eq.saturation");
		float vGammaR = m_pVideoPref->GetFloat("videofilter.eq.gammaRed");
		float vGammaG = m_pVideoPref->GetFloat("videofilter.eq.gammaGreen");
		float vGammaB = m_pVideoPref->GetFloat("videofilter.eq.gammaBlue");
		if(vContrast != 50 || vBright != 0 || vSaturation != 100 || !FloatEqual(vGammaR, 1.f)
			|| !FloatEqual(vGammaG, 1.f) || !FloatEqual(vGammaB, 1.f)) {	// If not default value
			float gamma = vGammaR;
			if(!FloatEqual(gamma,vGammaG) || !FloatEqual(gamma,vGammaB)) {	// if gamma of r,g,b is not equal, then set it to 1
				gamma = 1.f;
			}
			
			char buf[32] = {0};
			sprintf(buf, "%.2f:%.2f:%.2f:%.2f", gamma, 
				(float)vContrast / 50, (float)vBright / 100, (float)vSaturation /100);
			opts << ",eq2=" << buf;
			if (FloatEqual(gamma, 1.f)) {	// gamma of r,g,b is not equal
				memset(buf, 0, 32);
				sprintf(buf, ":%.2f:%.2f:%.2f", vGammaR, vGammaG, vGammaB);
				opts << buf;
			}
			opts << ",";
		}

		if ((i = m_pVideoPref->GetInt("videofilter.eq.hue")) != 0)
			opts << "hue=" << i << ',';

		if (m_pVideoPref->GetBoolean("videofilter.rotate.enabled")) {
			int rotateMode = m_pVideoPref->GetInt("videofilter.rotate.mode");
			if(rotateMode >= 0 && rotateMode <= 3) {
				opts << "rotate=" << rotateMode << ',';
				swap(m_pVInfo->res_out.width, m_pVInfo->res_out.height);
				swap(m_pVInfo->src_dar.num, m_pVInfo->src_dar.den);
				swap(m_pVInfo->dest_dar.num, m_pVInfo->dest_dar.den);
			}
		}
		if (m_pVideoPref->GetBoolean("videofilter.rotate.flip")) {
			opts << "flip,";
		}
		if (m_pVideoPref->GetBoolean("videofilter.rotate.mirror")) {
			opts << "mirror,";
		}
	}

	return opts.str();
}

std::string CDecoderMencoder::GenSubTitleOptions(const char* srcFile, bool& useAss)
{
	std::stringstream opts;
	int sid = 0;
	if (srcFile && strstr(srcFile, "dvd://")) {		// DVD playback 
		std::vector<std::string> dvdParams;
		StrPro::StrHelper::splitString(dvdParams, srcFile);
		if(!dvdParams.empty() && !dvdParams[4].empty() && dvdParams[4] != "-1") {
			sid = atoi(dvdParams[4].c_str())-32;
		}
	} else {
		sid = m_pVideoPref->GetInt("overall.subtitle.sid");
	}
	
	const char* codePage = m_pVideoPref->GetString("overall.subtitle.codepage");
	if(codePage && *codePage) {
		opts << " -subcp " << codePage;
	} else {
		opts << " -subcp cp936,GBK,BIG-5,CP932,UTF-8,UTF-16";
	}
	const char* slang = m_pVideoPref->GetString("overall.subtitle.slang");
	
	if(slang && *slang) {
		opts << " -slang " << slang;
	} else {
		opts << " -slang zh";
	}
	
	bool bSubCC = m_pVideoPref->GetBoolean("overall.subtitle.subcc");
	int subEncode = m_pVideoPref->GetInt("overall.subtitle.encoding");
	int subPos = m_pVideoPref->GetInt("overall.subtitle.pos");
	float subdelay = m_pVideoPref->GetFloat("overall.subtitle.delay");
	int subBackAlpha = m_pVideoPref->GetInt("overall.subtitle.alpha");
	int subBackColor = m_pVideoPref->GetInt("overall.subtitle.color");
	float subScale = m_pVideoPref->GetFloat("overall.subtitle.scale");
	int subAutoScaleMode = m_pVideoPref->GetInt("overall.subtitle.autoScale");
	int subBlurRadius = m_pVideoPref->GetInt("overall.subtitle.blur");
	int fontOutLineThick = m_pVideoPref->GetInt("overall.subtitle.outline");
	int subMaxWidth = m_pVideoPref->GetInt("overall.subtitle.width");

	if(bSubCC) opts << " -subcc";
	switch (subEncode) {
		case 1: // UTF-8
			opts << " -utf8";
			break;
		case 2: // Unicode
			opts << " -unicode";
			break;
	}
	
	//if(subBlurRadius >= 0) opts << " -subfont-blur " << subBlurRadius;
	//if(fontOutLineThick >= 0) opts << " -subfont-outline " << fontOutLineThick;
	//if(subMaxWidth >= 10) opts << " -subwidth " << subMaxWidth;
	
	// Work around for MKV external subtitles problem
	float mkvExtraDelay = 0.f; 

	useAss = false;
	std::string subFile = GetSubFile(srcFile);
	if(!subFile.empty()) {
		std::string subExt;
		size_t lastDotIdx = subFile.find_last_of('.');
		if(lastDotIdx != std::string::npos) {
			subExt = subFile.substr(lastDotIdx);
		}
		if(!subExt.empty() && !_stricmp(subExt.c_str(), ".idx")) {	//Vobsub
			opts << " -vobsub \"" << subFile.substr(0,lastDotIdx) << "\"";
		} else {	// Other text sub
			if(subEncode <= 0) {	// auto detect charset
				const char* charType = StrPro::CCharset::DetectCharset(subFile.c_str());
				if(useAss && charType) {
					if(strstr(charType, "UTF-16")) {
						opts << " -unicode";
					} else if(strstr(charType, "UTF-8")) {
						opts << " -utf8";
					}
				} else {
					if(_stricmp(charType, "ANSI")) {
						opts << " -utf8";
					}
				}
			}
			opts << " -sub \"" << subFile << "\"";
			if(m_pVideoPref) {	// mplayer-ww support ass
				//useAss = m_pVideoPref->GetBoolean("videosrc.mencoder.useBackup");
			}
		}
		const char* ext = strrchr(srcFile, '.');
		if(ext && _stricmp(ext, ".mkv") == 0) {
			// Work around for mkv external subtitles problem
			mkvExtraDelay = 0.2f;		// 0.2s delay
		}
	} else {
		if(sid >= 0) opts << " -sid " << sid;
	}

	if(useAss) {
		opts << " -ass -ass-use-margins"; // -ass-font-scale 1.2
	} else {
		char  appPath[MAX_PATH]={0};
		GetAppDir(appPath, MAX_PATH);
		std::string subFontPath = appPath;
		subFontPath += "\\APPSUBFONTDIR\\msyh.ttf";
		if(FileExist(subFontPath.c_str()) /*&& !subFile.empty()*/) {		// some internal subs can't find fonts
			opts << " -subfont \"" << subFontPath << "\"";
		}
		if(subPos >= 0) opts << " -subpos " << subPos;

		//if(subBackAlpha >= 0) opts << " -sub-bg-alpha " << subBackAlpha;
		//if(subBackColor >= 0) opts << " -sub-bg-color " << subBackColor;
		if(subScale > 0) opts << " -subfont-text-scale " << subScale;
		if(subAutoScaleMode >= 0) opts << " -subfont-autoscale " << subAutoScaleMode;
	}

	subdelay += mkvExtraDelay;
	if(subdelay>0.0001f || subdelay<-0.0001f) opts << " -subdelay " << subdelay;

	return opts.str();
}

std::string CDecoderMencoder::GetCmdString(const char* mediaFile)
{
	ostringstream cmd;
	const char* mencoderVerStr = MENCODER;
	if(m_pVideoPref && m_pVideoPref->GetBoolean("videosrc.mencoder.useBackup")) {
		// To solve a/v sync problem of MKV and MOV, use mencoder-ww version
		mencoderVerStr = MENCODER;
	}
	if (strstr(mediaFile, "dvd://")) {			// DVD playback 
		std::string dvdCmd = GetDVDPlayCmd(mediaFile);
		cmd << mencoderVerStr << " " << dvdCmd << " " << NULL_FILE;
	} else if (strstr(mediaFile, "vcd://")) {	// VCD playback
		std::string vcdCmd = GetVCDPlayCmd(mediaFile);
		cmd << mencoderVerStr << " " << vcdCmd << " " << NULL_FILE;
	} else {									// Media file playback
		cmd << mencoderVerStr << " \"" << mediaFile << "\" "<< NULL_FILE;
	}

	if(strstr(mediaFile, "://")) {
		int cachesize = m_pVideoPref->GetInt("videosrc.mencoder.cache");
		if (cachesize > 0) {
			cmd << " -cache " << cachesize;
		}
	}

	if(m_bDecAudio) {		// Read PCM data and create audio encoders
#ifdef WIN32
		cmd << " -oac pcm -pcmopts pipe=$(fdAudioWrite)" ;
#else 
		cmd << " -oac pcm -pcmopts pipe=%d";
#endif
		if(m_pAInfo && m_pAInfo->aid >= 0) {
			cmd << " -aid " << m_pAInfo->aid;
		}

		int n = m_pAudioPref->GetInt("overall.audio.delay1");
		if (n != 0) {
			cmd << " -delay " << n/1000.f;
		} else if (m_pAInfo->delay != 0) {
			//cmd << " -delay " << (float)m_pAInfo->delay/1000.f;
		}

		// 5.1->2 Downmix is not correct in decoders
		int channelEnum = m_pAudioPref->GetInt("overall.audio.channels");
		int inchs = m_pAInfo->in_channels;
		//if(inchs > 6) inchs = 6;	// if channels > 6, ex. 8 chs
		if(m_pAInfo->in_channels == 6) {
			cmd << " -channels " << inchs;
		}
		/*
		if(channelEnum == 0 && m_pAInfo->in_channels > 2) {		// Use original channels
			cmd << " -channels " << inchs;
		} else if(m_pAInfo->srcformat == AC_DTS) {
			cmd << " -channels " << inchs;
		}*/

		std::string audioFilterStr = GenAudioFilterOptions();
		if(!audioFilterStr.empty() ){
			if(*audioFilterStr.begin() == ',') {
				audioFilterStr.erase(audioFilterStr.begin());
			}
			cmd << " -af " << audioFilterStr;
		}
	} else {
		cmd << " -nosound";
	}

	// Select demuxer
	std::string srcFileExt;
	StrPro::StrHelper::getFileExt(mediaFile, srcFileExt);
	
	bool useLavf = m_pVideoPref->GetBoolean("videosrc.mencoder.lavfdemux");
	
	// Cut movie
	CXMLPref* pPref = m_pAudioPref;
	if(!pPref) pPref = m_pVideoPref;
	if(m_pVideoPref) {
		int startpos = m_pVideoPref->GetInt("overall.decoding.startTime");
		int duration = m_pVideoPref->GetInt("overall.decoding.duration");
		if (startpos > 0) {
			cmd << " -ss " << startpos / 1000.f;
		} else if (!_stricmp(srcFileExt.c_str(), ".ts")) {	
			// Work around for ts file that first frame is not key frame
			if(!useLavf) {
				cmd << " -ss 0.01 -mc 1";	//
			}
		}
		if (duration > 0) cmd << " -endpos " << duration / 1000.f;
		if(useLavf) {
			cmd << " -demuxer lavf";
		}
	}
	
	if(m_bDecVideo) {		// Read YUV data and create video encoders
		if(m_pVInfo->res_out.width > 0 && m_pVInfo->res_out.height > 0) {
			int scaleAlgorithm = m_pVideoPref->GetInt("videofilter.scale.algorithm");
			switch (scaleAlgorithm) {
			case 0: case 1:
				cmd << " -sws 1"; break;
			default:
				cmd << " -sws " << scaleAlgorithm-1; break;
			}
		}  

		const char* forceVcStr = m_pVideoPref->GetString("videosrc.mencoder.forcevc");
		if(forceVcStr && *forceVcStr) {
			cmd << " -vc " << forceVcStr;
		}

		// Add Subtitle settings
		bool useAss = false;
		int subTitleMode = pPref->GetInt("overall.subtitle.mode");
		if(subTitleMode > 0) {
			cmd << GenSubTitleOptions(mediaFile, useAss);
		}

		cmd << " -vf format=i420,";
		if(m_pVInfo->raw_fromat == RAW_RGB24) {
			m_pVideoPref->SetBoolean("videofilter.rotate.flip", true);
		}
		if(m_pVInfo) {
			AutoCropOrAddBand(mediaFile);
			cmd << GenVideoFilterOptions();
		}

		if(m_pVInfo->raw_fromat == RAW_I420) {
			if(useAss) cmd << "ass,format=i420,";
		} else if(m_pVInfo->raw_fromat == RAW_RGB24){
			cmd << "format=BGR24,";
		}
		cmd << "harddup"; 

		fraction_t fpsIn = m_pVInfo->fps_in;
		fraction_t fpsOut = m_pVInfo->fps_out;
		if (fpsIn.num <= 0 || fpsIn.den <= 0) {
			fpsIn.num = 25;
			fpsIn.den = 1;
		}

		// Force wmv decode at fps='fpsIn'
		// || !_stricmp(srcFileExt.c_str(), ".WMV") || !_stricmp(srcFileExt.c_str(), ".ASF")
		bool bSyncCorrection = m_pVideoPref->GetBoolean("videofilter.extra.syncCorrection");
		if(!bSyncCorrection && !_stricmp(srcFileExt.c_str(), ".mkv")) {
			//cmd << " -fps " << fpsIn.num << '/' << fpsIn.den;
			cmd << " -mc 0 -noskip";
		}
		
		if(m_bEnableVf && fpsOut.num > 0) {
			cmd << " -ofps " << fpsOut.num << '/' << fpsOut.den;
		}

#ifdef WIN32
		cmd << " -of rawvideo -ovc raw -rawvidopts pipe=$(fdVideoWrite)";
#else
		m_proc.flags |= SF_USE_VIDEO_PIPE;
		m_proc.flags |= SF_INHERIT_WRITE;
		cmd << " -of rawvideo -ovc raw -rawvidopts pipe=%d";
#endif
	}

	int coreNum = CWorkManager::GetInstance()->GetCPUCoreNum();
	if(coreNum >= 16) {	// Threads num must <= 8
		coreNum = 8;
	} else if(coreNum >= 8){
		coreNum = 4;
	} else if(coreNum >= 4) {
		coreNum = 2;
	} else if(coreNum == 2) {
		coreNum = 1;
	}
    cmd << " -quiet -lavdopts threads=" << coreNum;  // Now mencoder should specify threads  
	return cmd.str();
}



