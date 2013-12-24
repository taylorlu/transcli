#include <string.h>
#include <sstream>
#include <vector>
#include <string>

#ifdef WIN32
#include <io.h>
#include <shlobj.h>
#endif

#include "DecoderVLC.h"
#include "MEvaluater.h"
#include "util.h"
#include "bit_osdep.h"

CDecoderVLC::CDecoderVLC(void)
{
}

CDecoderVLC::~CDecoderVLC(void)
{
}

std::string CDecoderVLC::GenVideoFilterOptions()
{
	//FIXME: It does not work, fix it later.
	std::stringstream opts;

	if(m_bEnableVf) {
		if(m_pVInfo) {
			if(m_pVInfo->vfType != VF_ENCODER) {
//				bool needDeint = false;
				switch (m_pVideoPref->GetInt("videofilter.deint.mode")) {
				case 2:
					if (m_pVInfo->interlaced == VID_PROGRESSIVE) break;
				case 1:
					opts << " --deinterlace-mode blend";
					break;
				}
			}
			resolution_t resOut = m_pVInfo->res_out;
			if(resOut.width > 0 && resOut.height > 0) {
				//opts << " --sout-transcode-width=" << resOut.width << " --sout-transcode-height=" << resOut.height;
			}
		}
		else if (m_pVideoPref->GetBoolean("videofilter.scale.mode")) {
			int w = m_pVideoPref->GetInt("videofilter.scale.width");
			int h = m_pVideoPref->GetInt("videofilter.scale.height");
			if (w && h) {
				opts << " --sout-transcode-width=" << w << " --sout-transcode-height=" << h;
			}
		}
	}
	return opts.str();
}


std::string CDecoderVLC::GenAudioFilterOptions()
{
	//TODO: Add audio filter here
	return "";
}

/* Add Video and Audio Filter options here*/
std::string CDecoderVLC::GenFilterOptions()
{
	int w = 0, h = 0;
	std::stringstream out;
	std::stringstream opts;

	if (!m_bEnableVf) return "";

	if(m_pVInfo) {
		w = m_pVInfo->res_out.width;
		h = m_pVInfo->res_out.height;
	} else if (m_pVideoPref->GetBoolean("videofilter.scale.mode")) {
		w = m_pVideoPref->GetInt("videofilter.scale.width");
		h = m_pVideoPref->GetInt("videofilter.scale.height");
	}

	if (w > 0 && h > 0) {
		opts << "width=" << w << ",height=" << h << ",";
	}

	fraction_t fpsIn = m_pVInfo->fps_in;
	fraction_t fpsOut = m_pVInfo->fps_out;

	if(m_pVInfo->vfType == VF_DECODER && fpsOut.num > 0) {
		opts << "fps=" << (float)fpsOut.num / fpsOut.den << ",";
	}
	else {
		if (fpsIn.num > 0 && fpsIn.den > 0) {
			opts << "fps=" << (float)fpsIn.num / fpsIn.den << ",";
		}
	}

	if (opts.str().empty()) return "";


	out << " --sout=#transcode{vcodec=rawv," << opts.str() << "deinterlace}:standard{mux=dummy,dst=dummy}";

	return out.str();
}

bool CDecoderVLC::TestRun()
{
	CProcessWrapper proc;
	std::stringstream cmd;

#ifdef WIN32
	cmd << '\"' << VLC_BIN << "\" --no-crashdump --vout none --aout none vlc://quit";
#else
	cmd << '\"' << VLC_BIN << "\" --vout none --aout none vlc://quit";
#endif
	return proc.Run(cmd.str().c_str()) != -1;
}

bool CDecoderVLC::Start(const char* sourceFile)
{
	int fdWriteVideo = -1;
	int fdWriteAudio = -1;
	std::ostringstream cmd;
	if(!sourceFile || *sourceFile == '\0')
		return false;
	if (!TestRun())
		return false;

#ifdef WIN32
	cmd << '\"' << VLC_BIN << "\" --no-osd --no-crashdump --no-ffmpeg-hurry-up";
//	cmd << " --yuv-chroma=I420";
#else
	cmd << '\"' << VLC_BIN << "\" --play-and-exit --no-osd";
//	cmd << " --yuv-chroma=I420";
#endif

//output
	cmd << GenFilterOptions();

	if(m_bDecVideo) {
		CProcessWrapper::MakePipe(m_fdReadVideo, fdWriteVideo, VIDEO_PIPE_BUFFER, false, true);
		cmd << " --vout=yuv --yuv-file=\"pipe:" << fdWriteVideo << "\"";
	}
	else {
		cmd << " --vout=none";
	}

	if(m_bDecAudio) {		// Read PCM data and create audio encoders
		CProcessWrapper::MakePipe(m_fdReadAudio, fdWriteAudio, AUDIO_PIPE_BUFFER, false, true);
		cmd << " --aout=file --audiofile-file=\"pipe:" << fdWriteAudio << "\"";
	}
	else {
		cmd << " --aout=none";
	}

#ifdef _DEBUG
//		cmd << " -vvv -I dummy";
#else
//		cmd << " -I dummy";
#endif

//input
	if (strncmp(sourceFile, "cap://", 6) == 0) { // Capture dev input
		std::string vdev, adev;
		const char *p = strchr(sourceFile, '+');

		if (p) {
			vdev.assign(sourceFile + 6, p - sourceFile - 6);
			adev.assign(p+1);
		}
		else {
			vdev.assign(sourceFile + 6, strlen(sourceFile) - 6);
			//adev is setted to default;
		}

		if (!m_bDecVideo) vdev.assign("none");
		if (!m_bDecAudio) adev.assign("none");

#if defined(WIN32)
		cmd << " dshow:// --dshow-vdev=\""<< vdev << "\" --dshow-adev=\"" << adev <<"\"";
//		cmd << " --dshow-size=320*240";
//		cmd << " --dshow-chroma=YUY2";
#elif defined(HAVE_LINUX)
		cmd << " v4l2:// --v4l2-input=\""<< vdev << "\" --v4l2-audio-input=\"" << adev <<"\"";
//		cmd << " --v4l2-chroma=YUY2";
#endif
	}
	else {									// Media file playback
		cmd << " \"" << sourceFile << "\" vlc://quit";
	}

	bool success = m_proc.Spawn(cmd.str().c_str()) && (m_proc.Wait(DECODER_START_TIME) == 0);
	if(success) {
		success = m_proc.IsProcessRunning();
	}
	if (!success) {
		SetErrorCode(EC_VIDEO_SOURCE_ERROR);
	}

	if(fdWriteAudio != -1) _close(fdWriteAudio);
	if(fdWriteVideo != -1) _close(fdWriteVideo);

	return success;
}
