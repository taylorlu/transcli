#include <string.h>
#include <sstream>
#include <vector>
#include <string>
#include "bit_osdep.h"
#include "DecoderFFMpeg.h"
#include "MEvaluater.h"
#include "util.h"
//#include "WorkManager.h"	// Get cpu core num
#include "logger.h"

CDecoderFFMpeg::CDecoderFFMpeg(void)
{
}

CDecoderFFMpeg::~CDecoderFFMpeg(void)
{
	for(size_t i=0; i<tmpPathVct.size(); ++i) {
		RemoveFile(tmpPathVct[i].c_str());
	}
	tmpPathVct.clear();
}

bool CDecoderFFMpeg::ReadAudioHeader()
{
	// FFMpeg wav format hasn't been parsed, output s16le format directly
	// Fill wave info struct with decoding params
	m_wavinfo.bits = 16;
	m_wavinfo.block_align = 2*m_wavinfo.channels;
	m_wavinfo.bytes_per_second = m_wavinfo.sample_rate*2*m_wavinfo.channels;
	return true;
	// FFmpeg output wav format is common, contain many info(tag info:Application version and so on)
	// ==============Riff header=================
	/*struct riff_header_t {
		char riffId[5];
		uint32_t fileLen;
		char wavId[5];
	};
	riff_header_t rheader = {0};
	if(_read(m_fdReadAudio, rheader.riffId, 4) != 4) return false;
	if(_read(m_fdReadAudio, &rheader.fileLen, 4) != 4) return false;
	if(_read(m_fdReadAudio, rheader.wavId, 4) != 4) return false;
	//==============Wave format chunk============
	struct wav_format_property {
		uint16_t fmt_tag;
		uint16_t channels;
		uint32_t sample_rate;
		uint32_t bytes_per_second;
		uint16_t block_align;
		uint16_t bits;
		uint16_t extension_size;
	};
	char fmtId[5] = {0};
	uint32_t fmtChunkSize = 0;
	if(_read(m_fdReadAudio, fmtId, 4) != 4) return false;
	if(_read(m_fdReadAudio, &fmtChunkSize, 4) != 4) return false;
	wav_format_property fmtConfig = {0};
	size_t fmtsize = sizeof(fmtConfig);
	if(_read(m_fdReadAudio, &fmtConfig, fmtsize) != fmtsize) return false;
	if(fmtConfig.fmt_tag == WAVE_FORMAT_PCM) {
	} else if(fmtConfig.fmt_tag == 0xFFFE) {
		int extensible = 0;
	}
	if(fmtConfig.extension_size > 0) {
		char tmpData[24] = {0};
		_read(m_fdReadAudio, tmpData, fmtConfig.extension_size);
	}

	// =============== Next chunk=============
	do {
		char chnkId[5] = {0};
		uint32_t chunkSize = 0;
		if(_read(m_fdReadAudio, chnkId, 4) != 4) return false;
		if(_read(m_fdReadAudio, &chunkSize, 4) != 4) return false;
		int datasize = 10+9;
		datasize += 8;
		if(chnkId[0]=='d' && chnkId[1]=='a' && chnkId[2]=='t' &&chnkId[3]=='a') break;
	}while(true);
	
	int readSize = SafeRead(m_fdReadAudio, (uint8_t*)&m_wavinfo, sizeof(m_wavinfo));
	int readTryCount = 0;
	while(readSize <= 0 && readTryCount < 8) {
		Sleep(200);
		readSize = SafeRead(m_fdReadAudio, (uint8_t*)&m_wavinfo, sizeof(m_wavinfo));
		readTryCount++;
	}
	if(readSize <= 0) {
		logger_err(LOGM_TS_VD, "Read audio header failed, return value:%d.\n", readSize);
		return false;
	}*/
}

/*
if(pPref->GetBoolean("overall.task.decoderTrim")) {
			globalTrimStr = " -global_trim ";
			int clipsCount = pPref->GetInt("overall.clips.count");
			for(int i=0; i<clipsCount; ++i) {
				char clipKey[32] = {0};
				sprintf(clipKey, "overall.clips.start%d", i+1);
				int startMs = pPref->GetInt(clipKey);	
				double trimStart = startMs/1000.0;
				memset(clipKey, 0, 32);
				sprintf(clipKey, "overall.clips.end%d", i+1);
				int endMs = pPref->GetInt(clipKey);		
				double trimEnd = endMs/1000.0;
				if(trimEnd > 0 && trimEnd > trimStart) {
					char trimStr[36] = {0};
					const char* clipFormat = "%1.2f,%1.2f,";
					if(i == clipsCount-1) {
						clipFormat = "%1.2f,%1.2f";
					}
					sprintf(trimStr, clipFormat, trimStart, trimEnd);  
					globalTrimStr += trimStr;
				}
			}
		} else {
*/

std::string CDecoderFFMpeg::GetCmdString(const char* mediaFile)
{
	CXMLPref* pPref = m_pAudioPref;
	if(!pPref) pPref = m_pVideoPref;
	int startpos = 0;
	int duration = 0;
	if(pPref) {
		startpos = pPref->GetInt("overall.decoding.startTime");
		duration = pPref->GetInt("overall.decoding.duration");
	}
	
	const int accurateSeekSecs = 20000;		// 20s
	std::ostringstream cmd;
	cmd << FFMPEG;
	if(startpos > accurateSeekSecs) {
		cmd << " -ss " << (startpos - accurateSeekSecs)/1000.f;
		startpos = accurateSeekSecs;
	}
	
	bool bSpecifyFps = m_pVInfo && (m_pVInfo->is_vfr ||
			m_pVInfo->fps_out.num != m_pVInfo->fps_in.num || 
			m_pVInfo->fps_out.den != m_pVInfo->fps_in.den);
	const char* audioCompensate = NULL;	// soft compenstion
	
	if(pPref->GetBoolean("overall.audio.insertBlank")) {
		m_bDecAudio = false;
	}
	if(pPref->GetBoolean("overall.video.insertBlank")) {
		m_bDecVideo = false;
	}

	if(m_bDecVideo && m_bDecAudio) {
		if(m_pVInfo->src_container != CF_MPEG2 && m_pVInfo->src_container != CF_AVI &&
			m_pVInfo->src_container != CF_MP4 && m_pVInfo->src_container != CF_MOV ) {
			//cmd << " -discard_first_not_key -dts_error_threshold 3600";	// discard corrupt frames for input   
		} 
		audioCompensate = "aresample=async=1:first_pts=0:min_comp=0.05:min_hard_comp=0.15,";
	}
	
	// -analyzeduration 500000000 to solve files that audio timestamp is leading video for about 20s
    cmd << " -analyzeduration 500000000 -probesize 50000000 -i \"" << mediaFile << "\"";
	// If there is no audio, insert blank audio track
	//if(pPref->GetBoolean("overall.audio.insertBlank")) {	
	//	cmd << " -f lavfi -i aevalsrc=0:d=1";	//:s=44100:c=2:d=10
	//}
	cmd << " -v error";	//verbose
	//if(m_bDecVideo && !bSpecifyFps && m_pVInfo->is_video_passthrough ) {
	//	cmd << " -vsync passthrough";
	//}

	if(m_bDecAudio) {
		if(!m_bDecVideo) cmd << " -vn";
		std::string audioFilterStr = GenAudioFilterOptions();
		if(startpos > 0) cmd << " -ss " << startpos/1000.f;
		if(duration > 0) cmd << " -t " << duration/1000.f;
		cmd << " -map 0:a:" << m_pAInfo->index;
		cmd << " -c:a pcm_s16le -f s16le";

		if((audioFilterStr.empty() || audioFilterStr.find("pan=") == std::string::npos)
			&& m_pAInfo->out_channels) {
			cmd << " -ac " << m_pAInfo->out_channels;
			m_wavinfo.channels = m_pAInfo->out_channels;
		}

		if(m_pAInfo->out_srate > 0) {
			cmd << " -ar " << m_pAInfo->out_srate;
			m_wavinfo.sample_rate = m_pAInfo->out_srate;
		} else {
			m_wavinfo.sample_rate = m_pAInfo->in_srate;
		}

		if(audioCompensate) {
			audioFilterStr.insert(0, audioCompensate);
		} 
		if(!audioFilterStr.empty()) {
            if(*audioFilterStr.rbegin() == ',') {
                audioFilterStr.erase(audioFilterStr.end()-1);
			}
			cmd << " -filter:a " << audioFilterStr;
		}
		
		cmd << " pipe:$(fdAudioWrite)";
	} 

	int subOption = 0;	// 0:disable, 1:external text sub(srt/ass), 2:internal bitmap sub(PGS/DVDSub)
	if(m_bDecVideo) {
		if(!m_bDecAudio) cmd << " -an";
		if(startpos > 0) cmd << " -ss " << startpos/1000.f;
		if(duration > 0) cmd << " -t " << duration/1000.f;
		if(m_pVInfo->index < 0) m_pVInfo->index = 0;
		cmd << " -map $(vtag)";
		
		cmd << " -c:v rawvideo -f rawvideo -pix_fmt yuv420p";	// -sws_flags bilinear
		const char* scaleMethodName[] = {"", "fast_bilinear", "bilinear", "bicubic", "experimental",
			"neighbor", "area", "bicublin", "gauss", "sinc", "lanczos", "spline"};
		int scaleAlgorithm = m_pVideoPref->GetInt("videofilter.scale.algorithm");
		if(scaleAlgorithm >= 0 && scaleAlgorithm <= 11) {
			const char* selectMethod = scaleMethodName[scaleAlgorithm];
			if(m_pVInfo->res_out.width > m_pVInfo->res_in.width) {	// If scale up, use lanczos
				selectMethod = "lanczos";
			}
			if(selectMethod && *selectMethod) {
				cmd << " -sws_flags " << selectMethod;
			}
		}

		if(m_pVInfo) {
 			AutoCropOrAddBand(mediaFile);
 		}

		if(bSpecifyFps) {
			cmd << " -r " << m_pVInfo->fps_out.num << '/' << m_pVInfo->fps_out.den;
		}

		// ========Judge subtitle type========
		int subTitleMode = m_pVideoPref->GetInt("overall.subtitle.mode");
		std::string externSub;
		if(subTitleMode > 0) {
			externSub = GetSubFile(mediaFile);
			if(!externSub.empty()) {
				subOption = 1;
			} else {
				int subIndex = m_pVideoPref->GetInt("overall.subtitle.sid");
				const char* embedSubType = m_pVideoPref->GetString("overall.subtitle.embedType");
				if(subIndex >= 0 && embedSubType) {
					if(!_stricmp(embedSubType, "pgssub") || !_stricmp(embedSubType, "dvdsub")) {
						subOption = 2;
					} else if(!_stricmp(embedSubType, "subrip") || !_stricmp(embedSubType, "ssa")) {
						// Extract embed text subtitle to external subtitle
						ExtractTextSub(mediaFile, subIndex, externSub, embedSubType);
						subOption = 1;
					}
				}
			}
		}

		std::string videoFilterStr = GenVideoFilterOptions(subOption);
		if(!videoFilterStr.empty()){
			if(subOption == 1) {	// External text sub
				videoFilterStr += GenTextSubOptions(mediaFile, externSub);
			}
            if(*videoFilterStr.rbegin() == ',') {
                videoFilterStr.erase(videoFilterStr.end()-1);
			}
			if(subOption == 2) {
				cmd << " -filter_complex \"" << videoFilterStr << "\""; // -fix_sub_duration";
			} else {
				cmd << " -filter:v " << videoFilterStr;
			}
		}

		/*int coreNum = CWorkManager::GetInstance()->GetCPUCoreNum();
		if(coreNum >= 16) {	// Threads num must <= 8
			coreNum = 8;
		} else if(coreNum >= 8){
			coreNum = 4;
		} else if(coreNum >= 4) {
			coreNum = 2;
		} else if(coreNum == 2) {
			coreNum = 1;
		}
		cmd << " -threads:v " << coreNum;*/

		int threadsNum = pPref->GetInt("overall.decoding.threads");
		if(threadsNum > 0) {
			cmd << " -threads:v " << threadsNum;
		}
		cmd << " pipe:$(fdVideoWrite)";
	} 
	std::string ffmpegCmd = cmd.str();
	if(subOption == 2) {
		ReplaceSubString(ffmpegCmd, "$(vtag)", "[v]");
	} else {
	    char vstream[8] = {0};
	    if(m_pVInfo && m_pVInfo->index > 0) {
		    sprintf(vstream, "0:v:%d", m_pVInfo->index);
		} else {
			strcpy(vstream, "0:v:0");
		}
		ReplaceSubString(ffmpegCmd, "$(vtag)", vstream);
	}
	return ffmpegCmd;
}

std::string CDecoderFFMpeg::GenAudioFilterOptions()
{
	std::ostringstream opts;
	int i;
	
	int mixOption = m_pAudioPref->GetInt("audiofilter.channels.mix");
	if(mixOption > 0 && m_pAInfo->in_channels == 2) {
		switch(mixOption) {
		case 2:		// Left(mono)
			opts << "pan=1:c0=0.5*c0+0.5*c1,"; 
			m_wavinfo.channels = 1;
			break;
		default:	// Stereo
			opts << "pan=stereo:c0=0.5*c0+0.5*c1:c1=0.5*c0+0.5*c1,";
			m_wavinfo.channels = 2;
			break;
		}
	} else if (m_pAudioPref->GetBoolean("audiofilter.channels.enabled")) {
		int nr = m_pAudioPref->GetInt("audiofilter.channels.routes");
		m_wavinfo.channels = m_pAudioPref->GetInt("audiofilter.channels.output");
		opts << "pan=" << m_wavinfo.channels;
		for (i=0; i < nr; i++) {
			char buf[48] = {0};
			_snprintf(buf, sizeof(buf), "audiofilter.channels.channel%d", i + 1);
			int sourcChIdx = m_pAudioPref->GetInt(buf) - 1;
			opts << ":c" << i << "=c" << sourcChIdx;
		}
		opts << ',';
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
			opts << "pan=1:c0=c0,";
			m_wavinfo.channels = 1;
			break;
		case 2:
			opts << "pan=1:c0=c1,";
			m_wavinfo.channels = 1;
			break;
		case 3:		// Stereo
			break;
		}
	}

	float volGain = m_pAudioPref->GetFloat("audiofilter.volume.gain");
	volGain += m_pAInfo->volGain;
	if (volGain > 0.9f || volGain < -0.9f) {
		opts << "volume=" << volGain << "dB,";
	}

	return opts.str();
}

std::string CDecoderFFMpeg::GenVideoFilterOptions(int subType)
{
	std::ostringstream forePart;
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
			const char* tinterlace = m_pVideoPref->GetString("videosrc.ffmpeg.tinterlace");
			if(tinterlace && *tinterlace) {
				forePart << "tinterlace=" << tinterlace << ',';
			}

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
			forePart << "yadif=" << m_pVideoPref->GetInt("videofilter.deint.yadif");
			forePart << ":" << fieldOrder << ',';
		
			// Interlace is done by decoder, then encoder no need to do
			if(m_bLastPass) m_pVInfo->interlaced = 0;	
		}
	}

	if((m_pVInfo->is_vfr && m_pVInfo->fps_out.num > 0 && m_pVInfo->fps_out.den > 0) ||
			m_pVInfo->fps_out.num != m_pVInfo->fps_in.num || 
			m_pVInfo->fps_out.den != m_pVInfo->fps_in.den) {
		forePart << "fps,";
	}

	// Add multiple delogo filter (with time and position)
	if (m_pVideoPref->GetBoolean("videofilter.delogo.enabled")) {
		int delogoThick = m_pVideoPref->GetInt("videofilter.delogo.thickness");
		if(delogoThick <= 0) delogoThick = 2;
		int delogoNum = m_pVideoPref->GetInt("videofilter.delogo.num");
		if(delogoNum > 0) {
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

				if(delogoStart > -0.1f && delogoEnd > -0.1f && delogoStart>delogoEnd) continue;
				forePart << "delogo=" << delogoX << ':' 
					<< delogoY << ':' << delogoW << ':'
					<< delogoH << ':' << delogoThick << ":0:"	// 0 is value of 'show'	
					<< delogoStart << ':' << delogoEnd << ',';
			}
		} else {	// MediacoderNT delogo
			int delogoX = m_pVideoPref->GetInt("videofilter.delogo.x");
			int delogoY = m_pVideoPref->GetInt("videofilter.delogo.y");
			int delogoW = m_pVideoPref->GetInt("videofilter.delogo.w");
			int delogoH = m_pVideoPref->GetInt("videofilter.delogo.h");
			int delogoThick = m_pVideoPref->GetInt("videofilter.delogo.thickness");
			forePart << "delogo=" << delogoX << ':' 
					<< delogoY << ':' << delogoW << ':'
					<< delogoH << ':' << delogoThick << ":0:"	// 0 is value of 'show'	
					<< "-1:-1,";
		}
	}

	std::ostringstream backPart;
	if(m_bEnableVf || m_pVInfo->vfType != VF_ENCODER) {
		bool enableExpand = m_pVideoPref->GetBoolean("videofilter.expand.enabled");
		int cropMode = m_pVideoPref->GetInt("videofilter.crop.mode");
		if (cropMode == 1 || cropMode == 2 || cropMode == 4) {	// Manual or auto crop
			int x = m_pVideoPref->GetInt("videofilter.crop.left");
			int y = m_pVideoPref->GetInt("videofilter.crop.top");
			int w = m_pVideoPref->GetInt("videofilter.crop.width");
			int h = m_pVideoPref->GetInt("videofilter.crop.height");
			int inW = m_pVInfo->res_in.width;
			int inH = m_pVInfo->res_in.height;
			if ((w > 0 || h > 0) && (w < inW || h < inH)) {
				backPart << "crop=";
				if(w) backPart << w;	
				backPart << ':';
				if(h) backPart << h;
				if(x >= 0) backPart << ':' << x;
				if(y >= 0) backPart << ':' << y;
				backPart <<',';
			}
		}

		// When video is rotated by PI/2 or 3*PI/2 in the first pass, then restore width/height and dar/sar 
		if(m_bLastPass) {
			if(m_pVInfo->rotate == 90 || m_pVInfo->rotate == 270) {
				std::swap(m_pVInfo->res_out.width, m_pVInfo->res_out.height);
				std::swap(m_pVInfo->dest_par.num, m_pVInfo->dest_par.den);
				std::swap(m_pVInfo->dest_dar.num, m_pVInfo->dest_dar.den);
				std::swap(m_pVInfo->src_dar.num, m_pVInfo->src_dar.den);
			}
		}

		resolution_t resOut = m_pVInfo->res_out;
		//resolution_t resIn = m_pVInfo->res_in;
		int scaleW = resOut.width, scaleH = resOut.height;
		if (scaleW > 0 && scaleH > 0) {
			if ((cropMode == 1 || cropMode == 3) && enableExpand) {		// scale and then expand
				fraction_t inDar = m_pVInfo->src_dar;
				fraction_t outDar = m_pVInfo->dest_dar;
				if(inDar.den > 0 && outDar.den > 0) {
					if(outDar.num/(float)outDar.den	> inDar.num/(float)inDar.den) {
						scaleW = scaleH*inDar.num/inDar.den;
						EnsureMultipleOfDivisor(scaleW, 2);
						if(scaleW > resOut.width) scaleW = resOut.width;
					} else {
						scaleH = scaleW*inDar.den/inDar.num;
						EnsureMultipleOfDivisor(scaleH, 2);
						if(scaleH > resOut.height) scaleH = resOut.height;
					}
				}
			} 
			backPart << "scale=" << scaleW << ':' << scaleH;
			
			/*int interlaceScale = 0;
			int chromaSkip = m_pVideoPref->GetInt("videofilter.scale.chroma");
			if(chromaSkip >= 1 && chromaSkip <= 3) {
				opts << ':' << interlaceScale << ':' << chromaSkip;
			}*/
			backPart << ',';
		}

		if (m_pVideoPref->GetInt("videofilter.denoise.mode") > 1) {
			backPart << "hqdn3d="
				<< m_pVideoPref->GetInt("videofilter.denoise.luma") << ':'
				<< m_pVideoPref->GetInt("videofilter.denoise.chroma") << ',';
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

			if (w && h && (w != scaleW || h != scaleH)) {
				backPart << "pad=";
				if(w) backPart << w;
				backPart << ':';
				if(h) backPart << h;
				if(x < 0) {
					x = (w - scaleW)/2;
					EnsureMultipleOfDivisor(x, 2);
				}

				if(y < 0) {
					y = (h - scaleH)/2;
					EnsureMultipleOfDivisor(y, 2);
				}

				backPart << ':' << x;
				backPart << ':' << y;
				backPart << ',';

				// Change video width and height
				//m_pVInfo->res_in.width = w;
				//m_pVInfo->res_in.height = h;
				m_pVInfo->res_out.width = w;
				m_pVInfo->res_out.height = h;
				//printf("Pad:x=%d,y=%d,w=%d,h=%d\n", x, y, w, h);
			}
		}

		if(m_pVInfo->rotate > 0 && m_pVInfo->rotate < 360) {
			const char* rotateStr = "PI/2";
			if(m_pVInfo->rotate == 270) {
				rotateStr = "3*PI/2";
			} else if(m_pVInfo->rotate == 180) {
				rotateStr = "PI";
			}
			backPart << "rotate=" << rotateStr ;
			if(m_pVInfo->rotate == 90 || m_pVInfo->rotate == 270) {
				std::swap(m_pVInfo->res_out.width, m_pVInfo->res_out.height);
				std::swap(m_pVInfo->dest_par.num, m_pVInfo->dest_par.den);
				std::swap(m_pVInfo->dest_dar.num, m_pVInfo->dest_dar.den);
				std::swap(m_pVInfo->src_dar.num, m_pVInfo->src_dar.den);
				backPart << ":ow=ih:oh=iw,";
			} else {
				backPart << ",";
			}
		}
	} 

	std::string vfilterStr;
	std::string foreCmd = forePart.str();
	std::string backCmd = backPart.str();
	if(subType == 2) {	// Overlay image subtitle
		const char* subIdStr = m_pVideoPref->GetString("overall.subtitle.sid");
        if(*foreCmd.rbegin() == ',') {
            foreCmd.erase(foreCmd.end()-1);
		}
        if(*backCmd.rbegin() == ',') {
            backCmd.erase(backCmd.end()-1);
		}

		char vstream[10] = {0};
	    if(m_pVInfo && m_pVInfo->index > 0) {
		    sprintf(vstream, "[0:v:%d]", m_pVInfo->index);
		} else {
			strcpy(vstream, "[0:v:0]");
		}

		if(foreCmd.empty() && backCmd.empty()) {	// All part empty
			vfilterStr = vstream;
			vfilterStr += "[0:s:";
			vfilterStr += subIdStr;
			vfilterStr += "]overlay[v]";
		} else if (foreCmd.empty()) {	
			vfilterStr = vstream;
			vfilterStr += "[0:s:";
			vfilterStr += subIdStr;
			vfilterStr += "]overlay,";
			vfilterStr += backCmd;
			vfilterStr += "[v]";
		} else if (backCmd.empty()) {
			vfilterStr = vstream;
			vfilterStr += foreCmd;
			vfilterStr += "[v];[v][0:s:";
			vfilterStr += subIdStr;
			vfilterStr += "]overlay[v]";
		} else {	// All part not empty
			vfilterStr = vstream;
			vfilterStr += foreCmd;
			vfilterStr += "[v];[v][0:s:";
			vfilterStr += subIdStr;
			vfilterStr += "]overlay,";
			vfilterStr += backCmd;
			vfilterStr += "[v]";
		}
	} else {
		vfilterStr = foreCmd;
		vfilterStr += backCmd;
	}
	return vfilterStr;
}

std::string CDecoderFFMpeg::GenTextSubOptions(const char* mediaFile, std::string& subFile)
{
	std::string subDir, subExt, subName;
	if(StrPro::StrHelper::splitFileName(subFile.c_str(),subDir,subName,subExt) &&
		(!_stricmp(subExt.c_str(), ".ass") || !_stricmp(subExt.c_str(), ".ssa") ||
		!_stricmp(subExt.c_str(), ".srt")) ) {
		// Transform subName to MD5 String(as vf_ass in ffmpeg can't read Chinese file name)
		subName = GetMd5(subName);
		std::string dstAssSub = subName + ".ass";
		std::string tmpSub = subName + subExt;
		if(!FileExist(dstAssSub.c_str())) {
			// Detect srcfile encoding
			const char* dstCode = "UTF-8";
			const char* srcCode = StrPro::CCharset::DetectCharset(subFile.c_str());
			// If src encoding equal to dst encoding, just copy file
			if(srcCode && dstCode && !strcmp(srcCode, dstCode)) {	
                TsCopyFile(subFile.c_str(), tmpSub.c_str());
			} else {
				if(srcCode && !strcmp(srcCode, "ANSI")) {
					srcCode = "\"\"";
				}
				std::string charConvertCmd = ICONV_EXE" -c -f ";
				charConvertCmd += srcCode;
				charConvertCmd += " -t ";
				charConvertCmd += dstCode;
				charConvertCmd += " \"";
				charConvertCmd += subFile + "\" > ";
				charConvertCmd += tmpSub;
				system(charConvertCmd.c_str());
			}
			// Convert to utf-8 file(Using enca)
			//std::string charConvertCmd = ENCA" -L none -x UTF-8 < \"";
			//charConvertCmd += subFile + "\" > ";
			//charConvertCmd += tmpSub;
			//system(charConvertCmd.c_str());

			if(!_stricmp(subExt.c_str(), ".srt")) {
				std::string srtConvertCmd = FFMPEG" -i ";
				srtConvertCmd += tmpSub;
				srtConvertCmd += " -v error -y ";
				srtConvertCmd += dstAssSub;
				CProcessWrapper::Run(srtConvertCmd.c_str());
			} else if(!_stricmp(subExt.c_str(), ".ssa")) {
				// Just rename to .ass
				rename(tmpSub.c_str(), dstAssSub.c_str());
			}
			tmpPathVct.push_back(tmpSub);
		}

		if(FileExist(dstAssSub.c_str())) {
			std::string videoFilterStr = "ass=";
			videoFilterStr += dstAssSub;
			tmpPathVct.push_back(dstAssSub);
			return videoFilterStr;
		}
	}

	return "";
}

void CDecoderFFMpeg::ExtractTextSub(const char* mediaFile, int subId, std::string& subFile, const char* subType)
{
	/*if(m_pVInfo->src_container == CF_MKV) {
		if(!_stricmp(subType, "subrip")) {
			subFile = GetMd5(mediaFile) + ".srt";
		} else {
			subFile = GetMd5(mediaFile) + ".ass";
		}
		if(FileExist(subFile.c_str())) {
			return;
		}
		std::string srtExtractCmd = MKVEXTRACT" tracks \"";
		srtExtractCmd += mediaFile;
		srtExtractCmd += "\" ";
		
		// Get stream id of selected sub(stream order, first stream id is 0) 
		const char* subStreamId = m_pVideoPref->GetString("overall.subtitle.streamId");
		srtExtractCmd += subStreamId;
		srtExtractCmd += ":\"";
		srtExtractCmd += subFile;
		srtExtractCmd += "\"";
		CProcessWrapper::Run(srtExtractCmd.c_str());
	} else { */
	// Use ffmpeg to extract text subtitle
	subFile = GetMd5(mediaFile) + ".ass";
	if(FileExist(subFile.c_str())) {
		return;
	}
	std::string srtExtractCmd = FFMPEG" -i \"";
	srtExtractCmd += mediaFile;
	srtExtractCmd += "\" -v error -vn -an -map 0:s:";
	char strId[6] = {0};
	sprintf(strId, "%d", subId);
	srtExtractCmd += strId;
	srtExtractCmd += " -y ";
	srtExtractCmd += subFile;
	CProcessWrapper::Run(srtExtractCmd.c_str());
	
	tmpPathVct.push_back(subFile);
}

//int fileDur = 0;
//	if(m_bDecAudio && m_pAInfo) {
//		fileDur = m_pAInfo->duration;
//	}
//	if(m_bDecVideo && m_pVInfo) {
//		if(fileDur < m_pVInfo->duration) {
//			fileDur = m_pVInfo->duration;
//		}
//	}
