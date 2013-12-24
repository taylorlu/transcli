#include <string.h>
#include <sstream>
#include <vector>
#include <string>

#include "DecoderCopy.h"
#include "MEvaluater.h"
#include "util.h"
#include "logger.h"
#include "bitconfig.h"

CDecoderCopy::CDecoderCopy(void)
{
	m_bForceAnnexb = true;
}

CDecoderCopy::~CDecoderCopy(void)
{
}

bool CDecoderCopy::Start(const char* sourceFile)
{
	std::ostringstream cmd;
	if(!sourceFile || *sourceFile == '\0') return false;

	bool useFFMpeg = true;
	const char* outExt = strrchr(m_dumpFile.c_str(), '.');
	if(outExt && _stricmp(".m2v", outExt) == 0) {
		useFFMpeg = false;
	}

	if(!useFFMpeg) {
		cmd << MPLAYER << " " << sourceFile;
	} else if (strstr(sourceFile, "dvd://")) {		// DVD playback 
		std::string dvdCmd = GetDVDPlayCmd(sourceFile);
		cmd << MPLAYER << " " << dvdCmd;
		useFFMpeg = false;
	} else if (strstr(sourceFile, "vcd://")) {	// VCD playback
		std::string vcdCmd = GetVCDPlayCmd(sourceFile);
		cmd << MPLAYER << " " << vcdCmd;
		useFFMpeg = false;
	} else {									// Media file playback
		cmd << FFMPEG << " -i \"" << sourceFile << "\" -v quiet";
	}

	if(useFFMpeg) {
		CXMLPref* pPref = NULL;
		if(m_bDecAudio) {
			pPref = m_pAudioPref;
		} else {
			pPref = m_pVideoPref;
		}
		int startpos = 0, duration = 0;
		if(pPref) {
			startpos = pPref->GetInt("overall.decoding.startTime");
			duration = pPref->GetInt("overall.decoding.duration");
		}
		if(startpos > 0) cmd << " -ss " << startpos/1000.f;
		if(duration > 0) cmd << " -t " << duration/1000.f;
		if(m_bDecAudio) {
			cmd << " -vn -c:a copy";
			if(outExt && _stricmp(".ec3", outExt) == 0) {
				cmd << " -f eac3";
			}
			cmd << " -y \"" << m_dumpFile << "\"";
		} else {
			if(outExt && _stricmp(".264", outExt) == 0) {
				cmd << " -f h264 -c:v copy";
				const char* inExt = strrchr(sourceFile, '.'); 
				if(m_bForceAnnexb) {
					cmd << " -vbsf h264_mp4toannexb";
				} 
			} else {
				cmd << " -c:v copy";
			}
			cmd << " -an -y \"" << m_dumpFile << "\"";
		}
	} else {
		if(m_bDecAudio) {
			cmd << " -dumpaudio -dumpfile \"" << m_dumpFile << "\"";
		} else {
			cmd << " -dumpvideo -dumpfile \"" << m_dumpFile << "\"";
		}
	}

#ifdef DEBUG_EXTERNAL_CMD
	logger_info(LOGM_TS_VD, "Copy Cmd: %s.\n", cmd.str().c_str());
#endif
	bool success = m_proc.Spawn(cmd.str().c_str());
	if(success) {
		success = (m_proc.Wait(DECODER_START_TIME/2) == 0);
		if(!success) {	// If process exit but exit code is 0, the process succeed.
			int exitCode = -1;
			m_proc.IsProcessRunning(&exitCode);
			if(exitCode == 0) success = true;
		}
	}
	if (!success) {
		m_proc.Cleanup();
	}
	return success;
}
