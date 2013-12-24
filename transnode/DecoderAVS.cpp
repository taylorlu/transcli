#include <string>
#include <sstream>
#include <algorithm>
#include <stdio.h>
#ifdef WIN32
#include <io.h>
#endif
#include "FileQueue.h"
#include "DecoderAVS.h"
#include "logger.h"
#include "MEvaluater.h"
#include "bit_osdep.h"
#include "util.h"
#include "yuvUtil.h"
using namespace std;

//LoadPlugin(\"VSFilter.dll\")
const char* delogoFunc = "function Delogo(clip c, int x, int y, int width, int height)\n\
{\n\
	top = c.crop(x, y, width, 2)\n\
	bottom = c.crop(x, y+height-2, width, 2)\n\
	mask = StackVertical(top,bottom).Blur(0.5,1.38).BilinearResize(width,height)\n\
	c = c.Layer(mask, \"add\", 255, x, y)\n\
	return c\n\
}";

// const char* downmixDts = "function DownmixStereo(clip c) \n\
// {\n\
//     flr = GetChannel(c, 2, 3) \n\
// 	fcc = GetChannel(c, 1, 1) \n\
// 	lrc = MixAudio(flr, fcc, 0.3694, 0.2612) \n\
// 	blr = GetChannel(c, 4, 5) \n\
// 	return MixAudio(lrc, blr, 1.0, 0.3694) \n\
// }";
// 
// const char* downmixAc3 = "function DownmixStereo(clip c) \n\
// {\n\
// 	flr = GetChannel(c, 1, 3) \n\
// 	fcc = GetChannel(c, 2, 2) \n\
// 	lrc = MixAudio(flr, fcc, 0.3694, 0.2612) \n\
// 	blr = GetChannel(c, 4, 5) \n\
// 	return MixAudio(lrc, blr, 1.0, 0.3694) \n\
// }";

const char* downmixWav = "function DownmixStereo(clip c) \n\
{\n\
	flr = GetChannel(c, 1, 2) \n\
	fcc = GetChannel(c, 3, 3) \n\
	lrc = MixAudio(flr, fcc, 0.3694, 0.2612) \n\
	blr = GetChannel(c, 4, 5) \n\
	return MixAudio(lrc, blr, 0.9, 0.3694) \n\
}";

static bool WriteTextFile(std::string filename, std::string content)
{
	FILE* fp = ts_fopen(filename.c_str(), "w");
	if (!fp) return false;

	fwrite(content.c_str(), content.length(), 1, fp);
	fclose(fp);
	return true;
}

CDecoderAVS::CDecoderAVS()
{
}

CDecoderAVS::~CDecoderAVS()
{
}

string CDecoderAVS::GetPluginPath()
{
	char curDir[MAX_PATH+1] = {0};
	if (GetAppDir(curDir, MAX_PATH) > 0) {
		std::string pluginPath = curDir;
		pluginPath += "\\codecs\\codecs";
		return pluginPath;
	}
	return "";
}

// types of delogo functions
struct mvClip {
	int startFrame;
	int endFrame;
};

struct delogoRect {
	int left;
	int top;
	int w;
	int h;
};

struct originClip {
	mvClip oriClip;
	delogoRect rect;
};

struct resultClip {
	mvClip curClip;
	std::vector<delogoRect> delogoRects;
};

std::string CDecoderAVS::GenDelogoScript(int movieStartFrame, int movieEndFrame, float inFps)
{
	int totalFrames = (int)((m_pVInfo->duration/1000.f + 0.5f)*inFps);

	int delogoNum = m_pVideoPref->GetInt("videofilter.delogo.num");
	int lastFrameNum = -1;
	if(delogoNum <= 0) return "";

	// Calculate start and end frame number of original clips
	std::vector<originClip> originClips;
	for (int i=0; i<delogoNum; ++i) {
		char posKey[50] = {0};
		sprintf(posKey, "videofilter.delogo.pos%d", i+1);
		const char* timePosStr = m_pVideoPref->GetString(posKey);
		std::vector<int> vctVal;
		float delogoStart= 0.f, delogoEnd = 0.f;
		int delogoX = 0, delogoY = 0, delogoW = 0, delogoH = 0;
		if(StrPro::StrHelper::parseStringToNumArray(vctVal, timePosStr)) {
			delogoStart = vctVal.at(0)/1000.f;	// ms to sec
			delogoEnd = vctVal.at(1)/1000.f;	// ms to sec
			delogoX = vctVal.at(2);
			delogoY = vctVal.at(3);
			delogoW = vctVal.at(4);
			delogoH = vctVal.at(5);
		}
		int startFrame = (int)(delogoStart*inFps+0.5f);
		int endFrame = (int)(delogoEnd*inFps+0.5f);
		if(endFrame<startFrame) continue;
		mvClip curClipInterval = {startFrame, endFrame};
		delogoRect curRect = {delogoX, delogoY, delogoW, delogoH};
		originClip curOriClip = {curClipInterval, curRect};
		originClips.push_back(curOriClip);
	}

	// Calculate and split into new clips according to start and end frame number of each clip
	std::vector<int> allTimePoint;
	if(movieStartFrame <= 0) movieStartFrame = 1;
	if(movieEndFrame <= 0) movieEndFrame = totalFrames;
	allTimePoint.push_back(movieStartFrame);
	allTimePoint.push_back(movieEndFrame);

	for (size_t i=0; i<originClips.size(); ++i) {
		int curFrameNum = originClips[i].oriClip.startFrame;
		if(curFrameNum > movieStartFrame && curFrameNum < movieEndFrame) {
			allTimePoint.push_back(curFrameNum);
		}
		
		curFrameNum = originClips[i].oriClip.endFrame;
		if(curFrameNum > movieStartFrame && curFrameNum < movieEndFrame) {
			allTimePoint.push_back(curFrameNum);
		}
	}

	// sort vector and remove same time point 
	sort(allTimePoint.begin(), allTimePoint.end());
	allTimePoint.erase(unique(allTimePoint.begin(), allTimePoint.end()), allTimePoint.end());

	// Generate new clips
	std::vector<resultClip> destClips;
	for (size_t i=0; i<allTimePoint.size()-1; ++i) {
		resultClip retClip;
		retClip.curClip.startFrame = allTimePoint[i];
		retClip.curClip.endFrame = allTimePoint[i+1];
		// Judge how many original clips overlap with the current clip
		for (size_t j=0; j<originClips.size(); ++j) {
			if(retClip.curClip.startFrame >= originClips[j].oriClip.startFrame &&
				retClip.curClip.endFrame <= originClips[j].oriClip.endFrame) {
				retClip.delogoRects.push_back(originClips[j].rect);
			}
		}
		destClips.push_back(retClip);
	}
	// Adjust start frame point of final clips
	for (size_t i=1; i<destClips.size(); ++i) {
		resultClip& curClip = destClips[i];
		resultClip& lastClip = destClips[i-1];
		curClip.curClip.startFrame = lastClip.curClip.endFrame+1;
	}

	// Generate delogo script
	std::stringstream sstr;
	sstr << "ConvertToRGB32()" << endl;
	// Process every clip
	for (size_t i=0; i<destClips.size(); ++i) {
		resultClip& curClip = destClips[i];
		int startFrame = curClip.curClip.startFrame;
		int endFrame = curClip.curClip.endFrame;
		size_t delogoNum = curClip.delogoRects.size();
		char clipVarName[8] = {NULL};
		sprintf(clipVarName, "clip%d", i+1);
		if(delogoNum > 0) {		// Clip that need to delogo
			for (size_t j=0; j<delogoNum; ++j) {
				int x = curClip.delogoRects[j].left;
				int y = curClip.delogoRects[j].top;
				int w = curClip.delogoRects[j].w;
				int h = curClip.delogoRects[j].h;
				if(j == 0) {	// First delogo of every clip (need trim the clip)
					if((i == destClips.size()-1) && movieEndFrame < 0) {	// last frame of last clip is special
						sstr << clipVarName <<"=Delogo(Trim("<<startFrame<<','<< "last.FrameCount),"
							<< x << ',' << y << ',' << w << ',' << h << ')' << endl;
					} else {
						sstr << clipVarName << "=Delogo(Trim(" << startFrame << ',' << endFrame << "),"
							<< x << ',' << y << ',' << w << ',' << h << ')' << endl;
					}
					
				} else {
					sstr << clipVarName << "=Delogo(" << clipVarName << ','
						<< x << ',' << y << ',' << w << ',' << h << ')' << endl;
				}
			}
		} else {
			if((i == destClips.size()-1) && movieEndFrame < 0 ) {	// last frame of last clip is special
				sstr << clipVarName << "=Trim(" << startFrame << ',' << "last.FrameCount)" << endl;
			} else {
				sstr << clipVarName << "=Trim(" << startFrame << ',' << endFrame << ")" << endl;
			}
			
		}
	}

	// Contact every clips into the whole movie
	for (size_t i=0; i<destClips.size(); ++i) {
		char clipVarName[8] = {NULL};
		sprintf(clipVarName, "clip%d", i+1);
		if(i == 0) {
			sstr << "last=";
		} 

		if(i < destClips.size()-1) {
			sstr << clipVarName << "+";
		} else {
			sstr << clipVarName;
		}
	}
	sstr << endl;
	return sstr.str();

/*	//Single delogo algorithm at different time position
    for (int i=0; i<delogoNum; ++i) {
		// Parse delogo time interval and position info
		char posKey[50] = {0};
		sprintf(posKey, "videofilter.delogo.pos%d", i+1);
		const char* timePosStr = m_pVideoPref->GetString(posKey);
		std::vector<float> vctVal;
		float delogoStart= 0.f, delogoEnd = 0.f;
		int delogoX = 0, delogoY = 0, delogoW = 0, delogoH = 0;
		if(StrPro::StrHelper::parseStringToNumArray(vctVal, timePosStr)) {
			delogoStart = vctVal.at(0);
			delogoEnd = vctVal.at(1);
			delogoX = (int)vctVal.at(2);
			delogoY = (int)vctVal.at(3);
			delogoW = (int)vctVal.at(4);
			delogoH = (int)vctVal.at(5);
		}
		int startFrame = (int)(delogoStart*fps_in+0.5f);
		if(startFrame <= lastFrameNum) startFrame = lastFrameNum+1;
		int endFrame = (int)(delogoEnd*fps_in+0.5f);
		if(endFrame<startFrame) continue;

		if(startFrame > lastFrameNum+1) {
			sstr << "Trim(" << lastFrameNum+1 << ',' << startFrame-1 << ")+";
		} 
		sstr << "Delogo(Trim(" << startFrame << ',' << endFrame << "),"
			<< delogoX << ',' << delogoY << ',' << delogoW << ',' << delogoH << ')';
		if(i == delogoNum-1) {
			if(endFrame < totalFrames-1) {
				sstr << "+Trim(" << endFrame+1 << ',' << "last.FrameCount)";
			}
			sstr << endl;
		} else {
			sstr << '+';
		}

		lastFrameNum = endFrame;
	}*/
}

static bool canDoSsrcResample(int sfrq,int dfrq)
{
	if (sfrq==dfrq) return true;
	if((sfrq == 44100 && dfrq == 32000) || (sfrq == 32000 && dfrq == 44100)) {
		return false;
	}

	int frqgcd = GetGcd(sfrq,dfrq);
	if (dfrq>sfrq)
	{
		int fs1 = sfrq / frqgcd * dfrq;

		if (fs1/dfrq == 1) return true;
		else if (fs1/dfrq % 2 == 0) return true;
		else if (fs1/dfrq % 3 == 0) return true;
		else return false;
	}
	else
	{
		if (dfrq/frqgcd == 1) return true;
		else if (dfrq/frqgcd % 2 == 0) return true;
		else if (dfrq/frqgcd % 3 == 0) return true;
		else return false;
	}
}

string CDecoderAVS::GetCmdString(const char* mediaFile)
{
	float fps_in = 0;
	if(m_pVInfo && m_pVInfo->fps_in.den > 0) {
		fps_in = (float)m_pVInfo->fps_in.num / m_pVInfo->fps_in.den;
	}
	if(fps_in < 0.0001f) {
		fps_in = 24.f;
	}
	float fps_out = fps_in;
	if(m_pVInfo && m_pVInfo->fps_out.num > 0 && m_pVInfo->fps_out.den > 0) {
		fps_out = (float)m_pVInfo->fps_out.num / m_pVInfo->fps_out.den;
	}
	
	string script;
	std::ostringstream sstr;

	CXMLPref* pPref = m_pVideoPref;
	if(!pPref) return "";

	// Delogo related params
	bool bDelogo = m_pVideoPref->GetBoolean("videofilter.delogo.enabled");
	int delogoNum = m_pVideoPref->GetInt("videofilter.delogo.num");
	std::string delogoxFile, delogoyFile, delogowFile, delogohFile;
	if(bDelogo && delogoNum > 0) {
		// Define user defined delogo filter function in script
		sstr << delogoFunc << endl;	
	}

	// Set plugin working path
	std::string pluginPath = GetPluginPath();
	if(!pluginPath.empty()) {
		sstr << "SetWorkingDir(\"" << pluginPath << "\")" << endl;
	}

	const char *p = NULL;
	int srcType = pPref->GetInt("videosrc.avisynth.source");
	switch (srcType) {
	case 0:		// AviSource
		sstr << "AviSource(\"" << mediaFile << "\"";
		p = pPref->GetString("videosrc.avisynth.fourcc");
		if (p && *p) sstr << ", fourcc=\"" << p << "\"";
		sstr << ")" << endl;
		break;
	case 1:		// AviFileSource
		sstr << "AviFileSource(\"" << mediaFile << "\"";
		p = pPref->GetString("videosrc.avisynth.fourcc");
		if (p && *p) sstr << ", fourcc=\"" << p << "\"";
		sstr << ")" << endl;
		break;
	case 3:		// DirectShow Source
		sstr << "LoadPlugin(\"DirectShowSource.dll\")" << endl;
		sstr << "DirectShowSource(\"" << mediaFile << "\"";
 		sstr << ",fps=" << fps_out << ",convertfps=true)" << endl;
		//sstr << ",pixel_type=\"YV12\")" << endl;
		break;
	case 4:		// OpenDMLSource
		sstr << "OpenDMLSource(\"" << mediaFile << "\"" << endl;
		break;
	default: 	// FFMS
		sstr << "LoadPlugin(\"ffms2.dll\")" << endl;
		sstr << "Import(\"FFMS2.avsi\")" << endl;
		sstr << "FFmpegSource2(\"" << mediaFile << "\",";
		if(m_bDecAudio) {
			sstr << "atrack=-1,";	// Auto-select audio track
		} else {
			sstr << "atrack=-2,";	// Disable audio track
		}

		std::string cacheIdxFile;
		if(m_pFileQueue && m_pFileQueue->GetTempDir()) {
			StrPro::StrHelper::getFileTitle(mediaFile, cacheIdxFile);
			cacheIdxFile += ".ffindex";
			cacheIdxFile.insert(0, m_pFileQueue->GetTempDir());
			if(!cacheIdxFile.empty()) {
				sstr << "cachefile=\"" << cacheIdxFile << "\",";
			}
		}
		int fpsNum = 0, fpsDen = 0;
		GetFraction(fps_out, &fpsNum, &fpsDen);
		sstr << "fpsnum="<<fpsNum<<",fpsDen=" << fpsDen << ")" << endl;

		// Generate ffindex file by external program
		if(!FileExist(cacheIdxFile.c_str())) {
			std::string ffindexCmd = "\"";
			ffindexCmd += pluginPath;
			if(m_bDecAudio) {
				ffindexCmd += "\\ffmsindex.exe\" -t -1 \"";
			} else {
				ffindexCmd += "\\ffmsindex.exe\" \"";
			}
			ffindexCmd += mediaFile;
			ffindexCmd += "\" \"";
			ffindexCmd += cacheIdxFile;
			ffindexCmd += "\"";
			if(CProcessWrapper::Run(ffindexCmd.c_str()) != 0) {
				return "";
			}
		}
		
		break;
	}

	std::string subFile = GetSubFile(mediaFile);
	if(!subFile.empty()) {
		sstr << "LoadPlugin(\"VSFilter.dll\")" << endl;
		const char *subExt = strrchr(subFile.c_str(), '.');
		if (subExt) {
		    // Create a style file for srt to change font
			if(!_stricmp(subExt, ".srt")) {
			    std::string srtStyleFile = subFile + ".style";
				FILE* srtFp = fopen(srtStyleFile.c_str(), "wt");
				if(srtFp) {
				    fprintf(srtFp, "ScriptType: v4.00+\n\n[V4+ Styles]\n");
					fprintf(srtFp, "Format:Name,Fontname\n");
					fprintf(srtFp, "Style:Default,simhei\n");
					fclose(srtFp);
				}
			}

			if (!_stricmp(subExt, ".srt") || !_stricmp(subExt, ".ssa") || !_stricmp(subExt, ".ass")) {
				sstr << "TextSub(\"" << subFile << "\")" << endl;
			} else if (!_stricmp(subExt, ".sub") || !_stricmp(subExt, ".idx")) {
				sstr << "VobSub(\"" << subFile << "\")" << endl;
			}
		}
	}

	int startpos = pPref->GetInt("overall.decoding.startTime");
	int duration = pPref->GetInt("overall.decoding.duration");
	int firstFrame = -1, lastFrame = -1;
	// Frame rate has been changed to out fps after directshow source
	if ((startpos > 0 || duration > 0) && fps_out > 0)  {
		firstFrame = (int)(startpos / 1000.f * fps_out + 0.5f);
		lastFrame = duration > 0 ? (int)((startpos + duration) / 1000.f * fps_out + 0.5f) : 0;
	}
	if (bDelogo && delogoNum > 0) {
		std::string delogoStr = GenDelogoScript(firstFrame, lastFrame, fps_out);
		if(!delogoStr.empty()) {
			sstr << delogoStr;
		}
	} else if ((firstFrame > 0 || lastFrame > 0) && fps_in > 0)  {
		sstr << "Trim(" << firstFrame << ',' << lastFrame << ')' << endl;
	}

	// After delogo, do color space convert
	sstr << "ConvertToYV12()" << endl;

	bool needDeint = false;
	if(m_pVInfo->vfType != VF_ENCODER) {
		switch (m_pVideoPref->GetInt("videofilter.deint.mode")) {
		case 2:
			if (m_pVInfo->interlaced != VID_PROGRESSIVE) {
				needDeint = true;
			}
			break;
		case 1:
			needDeint = true; break;
		}
	}

	if (needDeint) {
		int interlaceMode = pPref->GetInt("videofilter.deint.fieldOrder");
		if(interlaceMode == 0 && m_pVInfo->interlaced > 0) {
			interlaceMode = VID_INTERLACE_TFF;
		}

		bool useBob = false;
		
		switch (pPref->GetInt("videofilter.deint.algorithm")) {
		case 6: {
			sstr << "LoadPlugin(\"LeakKernelDeint.dll\")" << endl;
			sstr << "LeakKernelDeint(" << (m_pVInfo->interlaced == VID_INTERLACE_TFF ? 1 : 0) << ')' << endl;
		} break;		

		case 7: 
			sstr << "LoadCplugin(\"yadif.dll\")" << endl;
			sstr << "Yadif(" << pPref->GetInt("videofilter.deint.yadif");
			switch (interlaceMode) {
			case 1: sstr << ",1"; break;
			case 2: sstr << ",0"; break;
			}
			sstr << ")" << endl;
			break;
		default:
			sstr << "AssumeFieldBased" << endl;
			if(interlaceMode == 1) {
				sstr << "AssumeTFF" << endl;
			} else {
				sstr << "AssumeBFF" << endl;
			}
			// Change output fps because bob will double fps
			if(m_pVInfo->fps_out.den <= 0) {
				m_pVInfo->fps_out.num = m_pVInfo->fps_in.num;
				m_pVInfo->fps_out.den = m_pVInfo->fps_in.den;
			}
			sstr << "Bob()" << endl;
			break;
		}
	}

	//if (fps_out != fps_in && m_pVInfo->fps_out.num > 0) {
	//	sstr << "ChangeFPS(" << m_pVInfo->fps_out.num << ',' << m_pVInfo->fps_out.den << ",true)" << endl;
	//}

	int cropMode = pPref->GetInt("videofilter.crop.mode");
	bool bExpandAfterScale = m_pVideoPref->GetBoolean("videofilter.expand.afterScale");
	bool enableExpand = m_pVideoPref->GetBoolean("videofilter.expand.enabled");
		
	if (cropMode == 2 || cropMode == 1 || cropMode == 4) {
		int x = m_pVideoPref->GetInt("videofilter.crop.left");
		int y = m_pVideoPref->GetInt("videofilter.crop.top");
		int w = m_pVideoPref->GetInt("videofilter.crop.width");
		int h = m_pVideoPref->GetInt("videofilter.crop.height");
		if (x || y || w || h ) {
			sstr << "Crop(" << x << ',' << y << ',' << (w & 0xfffffffe) <<','
				<< (h & 0xfffffffe) << ")"<<endl;
		}
	}

// 		if ((cropMode == 3 || cropMode == 1) && enableExpand && !bExpandAfterScale) {
// 			int x = m_pVideoPref->GetInt("videofilter.expand.x");
// 			int y = m_pVideoPref->GetInt("videofilter.expand.y");
// 			int w = m_pVideoPref->GetInt("videofilter.expand.width");
// 			int h = m_pVideoPref->GetInt("videofilter.expand.height");
// 			if (x || y || w || h ) {
// 				sstr << "AddBorders(" << x << ',' << y << ',' 
// 					<< x << ',' << y << ")"<<endl;
// 			}
// 		}

	int scaleW = m_pVInfo->res_out.width, scaleH = m_pVInfo->res_out.height;
	if (scaleW > 0 || scaleH > 0) {
		switch (pPref->GetInt("videofilter.scale.algorithm")) {
		case 1:
		case 2:
			sstr << "BilinearResize";
			break;
		case 9:
			sstr << "SincResize";
			break;
		case 10:
			sstr << "LanczosResize";
			break;
		default:
			sstr << "BilinearResize";
		}
			
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
		}
		sstr << '(' << scaleW << ',' << scaleH << ')' << endl;
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
			if(x < 0) x = (w-scaleW)/2;
			EnsureMultipleOfDivisor(x, 2);
			if(y < 0) y = (h-scaleH)/2;
			EnsureMultipleOfDivisor(y, 2);
			sstr << "AddBorders(" << x << ',' << y << ',' 
				<< w-scaleW-x << ',' << h-scaleH-y << ")"<<endl;
			// Change video width and height
			//m_pVInfo->res_in.width = w;
			//m_pVInfo->res_in.height = h;
			m_pVInfo->res_out.width = w;
			m_pVInfo->res_out.height = h;
		}
	}

	ostringstream s;
	int n = pPref->GetInt("videofilter.eq.hue");
	if (n) s << ",hue=" << n;
	n = pPref->GetInt("videofilter.eq.saturation");
	if (n != 100) s << ",sat=" << (float)n / 100;
	n = pPref->GetInt("videofilter.eq.brightness");
	if (n) s << ",bright=" << (float)n / 100 * 255;
	n = pPref->GetInt("videofilter.eq.contrast");
	if (n != 50) s << ",contrast=" << (float)n / 50;
	if (s.str().length() > 0) sstr << "Tweak(" << (s.str().c_str() + 1) << ')' << endl;
	
	if (pPref->GetBoolean("videofilter.postproc.hdeblock") || pPref->GetBoolean("videofilter.postproc.vdeblock")) {
		sstr << "LoadPlugin(\"deblock.dll\")" << endl;
		sstr << "deblock()" << endl;
	}

	switch (pPref->GetInt("videofilter.denoise.mode")) {
	case 2:
	case 3:
		sstr << "LoadPlugin(\"hqdn3d.dll\")" << endl;
		sstr << "hqdn3d(" << pPref->GetInt("videofilter.denoise.luma") << ','
			<< pPref->GetInt("videofilter.denoise.chroma") << ','
			<< pPref->GetInt("videofilter.denoise.strength") << ')' << endl;
		break;
	}

	switch (pPref->GetInt("videofilter.unsharp.mode")) {
	case 1:
		sstr << "Sharpen" << endl; break;
	case 2:
		sstr << "Blur" << endl; break;
	}
		
	if(m_pVideoPref->GetBoolean("overall.task.interlace")) {
		sstr << "Assumetff()\r\nSeparateFields()\r\nDoubleWeave()" << endl;
		//GetFraction(fps_out*2, &m_pVInfo->fps_out.num, &m_pVInfo->fps_out.den);
	}

	// audio
	if(m_pAudioPref) {
		int srate = m_pAInfo->out_srate;
		if (srate > 0 /*&& (srate != m_pAInfo->in_srate || m_bForceResample)*/) {
			if (m_pAudioPref->GetBoolean("audiofilter.resample.precious")) {
				if(canDoSsrcResample(m_pAInfo->in_srate, srate)) {
					sstr << "SSRC(" << srate << ')' << endl ;
				} else {
					sstr << "ResampleAudio(" << srate << ')' << endl;
				}
			} else {
				sstr << "ResampleAudio(" << srate << ')' << endl;
			}
		} 

		int mixOption = m_pAudioPref->GetInt("audiofilter.channels.mix");
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
		
		if(mixOption > 0 && m_pAInfo->in_channels == 2) {
			switch(mixOption) {
			case 2:		// Left(mono)
				sstr << "ConvertAudioTo16bit" << endl;
				sstr << "audio=ConvertToMono" << endl; 
				break;
			default:	// Stereo
				sstr << "leftCh=GetLeftChannel(last)" << endl;
				sstr << "rightCh=GetRightChannel(last)" << endl;
				sstr << "mixCh=MixAudio(leftCh, rightCh)" << endl;
				sstr << "stereo=MergeChannels(mixCh, mixCh)" << endl;
				sstr << "audio=ConvertAudioTo16bit(stereo)" << endl;
				break;
			}
		} else if(channelEnum > 0) {
			if(channelEnum == 1) {	// Left channel
				sstr << "ConvertAudioTo16bit" <<endl;
				sstr << "audio=GetLeftChannel(last)" << endl;
			} else if(channelEnum == 2) {	// Right channel
				sstr << "ConvertAudioTo16bit" <<endl;
				sstr << "audio=GetRightChannel(last)" << endl;
			} else if(channelEnum == 3) {	// Stereo
				if(m_pAInfo->in_channels == 6) {
					sstr << downmixWav <<endl;
					sstr << "stereo=DownmixStereo(last)" << endl;
					sstr << "audio=ConvertAudioTo16bit(stereo)" << endl;
				} else {
					sstr << "ConvertAudioTo16bit" << endl;
					sstr << "audio=GetChannel(last, 1, 2)" << endl;
				}
			} else {
				sstr << "audio=ConvertAudioTo16bit" << endl;
			}
		}

		n = m_pAudioPref->GetInt("overall.audio.delay1");
		if (n) {
			sstr << "audio=DelayAudio(audio," << n / 1000.f << ')' << endl;
		} /*else if(m_pAInfo->delay) {
			sstr << "audio=DelayAudio(audio," << m_pAInfo->delay / 1000.f << ')' << endl;
		}*/

		if ((n = m_pAudioPref->GetInt("audiofilter.volume.normalization"))) {
			sstr << "audio=Normalize(audio,0.92)";
		}

		float volGain = m_pAudioPref->GetFloat("audiofilter.volume.gain");
		volGain += m_pAInfo->volGain;
		if (volGain > 0.9f || volGain < -0.9f) {
			int clipping = m_pAudioPref->GetBoolean("audiofilter.volume.clipping") ? 1:0;
			sstr << "audio=AmplifydB(audio," << volGain << ')' << endl;
		}

		sstr << "return AudioDub(last, audio)" << endl;
	}
	return sstr.str();
}	

bool CDecoderAVS::CreateScriptFile(const char* mediaFile)
{
	if(m_cmdString.empty()) {
		if(m_pVInfo) {
			AutoCropOrAddBand(mediaFile);
		}
		m_cmdString = GetCmdString(mediaFile);
		CXMLPref* pPref = m_pVideoPref;
		if(!pPref) pPref = m_pAudioPref;
		m_bKeepScriptFile  =  pPref->GetBoolean("videosrc.avisynth.keepScript");
	}
	
	if (m_cmdString.empty() || !m_pFileQueue) return false;
	m_mainScriptFile = m_pFileQueue->GetStreamFileName(ST_VIDEO, 0, 0, "avs");
	
	return WriteTextFile(m_mainScriptFile, m_cmdString);;    
}

bool CDecoderAVS::RemoveScriptFile()
{
	if (!m_bKeepScriptFile) {
		DeleteFile(m_mainScriptFile.c_str());
		m_mainScriptFile.clear();
		for (size_t i=0; i<m_subScriptFiles.size(); ++i) {
			DeleteFile(m_subScriptFiles.at(i).c_str());
		}
		m_subScriptFiles.clear();
	}
	return true;
}

bool CDecoderAVS::Start(const char* sourceFile)
{
	int fdAWrite = -1, fdVWrite = -1;
	ostringstream cmd;
	const char *extName = strrchr(sourceFile, '.');
	if (extName && !_stricmp(extName, ".avs")) {
		cmd << AVSINPUT << " \"" << sourceFile << "\"";
	} else {
		if (!CreateScriptFile(sourceFile))
			return false;
		cmd << AVSINPUT << " \"" << m_mainScriptFile << "\"";
	}

	if(m_bDecVideo) {		// Read YUV data and create video encoders
		CProcessWrapper::MakePipe(m_fdReadVideo, fdVWrite, VIDEO_PIPE_BUFFER, false, true);
		cmd << ' ' << fdVWrite;
	} else {
		cmd << " 0";
	}

	if(m_bDecAudio) {		// Read PCM data and create audio encoders
		CProcessWrapper::MakePipe(m_fdReadAudio, fdAWrite, AUDIO_PIPE_BUFFER, false, true);
		cmd << ' ' << fdAWrite;
	} else {
		cmd << " 0";
	}

#ifdef DEBUG_EXTERNAL_CMD
	printf("CMD:%s\n", cmd.str().c_str());
#endif

	// AVS need more time to start and parse script
	const int avsWaitTime = DECODER_START_TIME + 1200;
	bool success = m_proc.Spawn(cmd.str().c_str());
	if(success) {
		success = (m_proc.Wait(avsWaitTime) == 0);
		// AVSInput always return 0 when exit, AVSInput should be changed.
		if(!success) {	// If process exit but exit code is 0, the process succeed.
			int exitCode = -1;
			m_proc.IsProcessRunning(&exitCode);
			if(exitCode == 0) success = true;
		}
	}
	
#ifdef WIN32
	if(fdAWrite != -1) {
		_close(fdAWrite);
	}
	if(fdVWrite != -1) {
		_close(fdVWrite);
	}
#endif
	return success;
}

void CDecoderAVS::Cleanup()
{
	RemoveScriptFile();
	CDecoder::Cleanup();
}
