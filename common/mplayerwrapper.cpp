/*******************************************************************
* MediaCoder - The universal audio/video transcoder
* Distributed under GPL license
* Copyright (c) 2005-06 Stanley Huang <stanleyhuangyc@yahoo.com.cn>
* All rights reserved.
*******************************************************************/

#include <stdio.h>
#include <assert.h>
#ifdef WIN32
#include <io.h>
#define DD 
#else
#include <stdlib.h>
//#define SIMPLEST_PLAYTHREAD
#define NO_PLAYTHREAD
#endif
#include <errno.h>
#include "mplayerwrapper.h"
#include "processwrapper.h"

using namespace std;
static CMPlayer* me = 0;

static int hex2int(char *p)
{
    register char c;
    register unsigned int i=0;

    if (p[1] == 'x' || p[1] == 'X') p += 2;
    if (!p) return 0;
    while (*p && (*p==' ' || *p=='\t')) p++;
    for(c=*p;;){
        if (c>='A' && c<='F')
            c-=7;
        else if (c>='a' && c<='f')
            c-=39;
        else if (c<'0' || c>'9')
            break;
        i=(i<<4)|(c&0xF);
        c=*(++p);
    }
    return (int)i;
}

#ifdef WIN32
DWORD WINAPI PlayThread(CMPlayer *player)
{
#else
void *PlayThread(void *arg)
{
    CMPlayer *player = (CMPlayer *)arg;

    printf("Enter PlayThread..\n");

#if 1
	player->SetState(PLAYER_ENTRY);
#else
	player->SetState(PLAYER_PLAYING);
#endif

#endif
    bool eof = false;
    bool ready = false;
    bool error = false;
    int bytes = 0;
    do {
        char buf[1024];
        char *pbuf = buf + bytes;
		DD;
        bytes = player->Read(&pbuf, sizeof(buf) - bytes);
		DD;
        if (bytes <= 0) {
			printf("Read ret: %d\n", bytes);
            player->pProc->Cleanup();
            error = true;
            break;
        }
		printf("[Console]\n%s\n", buf);
        char *s;
        char *p = buf;
        for (;;) {
            s = p;
            for (; *p && *p != '\r' && *p != '\n'; p++);
            if (!*p) break;
            *p = 0;
            while (*(++p) == '\n' || *p == '\r');
            if (strstr(s, "Exiting...")) {
                //player->pProc->Cleanup();
                error = true;
                break;
            }
            if (strstr(s, "Video: no video")) {
                player->hasVideo = false;
                ready = true;					
                break;
            }
            if (strstr(s, "Audio: no sound"))
                player->hasAudio = false;

            if ((player->hasVideo && !strncmp(s, "VO: ", 4))) {
                ready = true;
                break;
            } else if (!strncmp(s, "ID_", 3)) {
                char *id = s + 3;
                char *v = strchr(id, '=');
                if (v) {
                    *(v++) = 0;
                    player->ParseInfo(id, v);
                }
            }
        }
        bytes = strlen(s);
        if (bytes)
            memcpy(buf + bytes, s, bytes + 1);
    } while (!ready && !error);

    if (error) {
        printf("ON error, exiting...");
        goto end;
    }

	player->DumpInfo();

    player->SetState(PLAYER_PLAYING);

    while (player->state != PLAYER_STOPPING) {

        if (player->state == PLAYER_PLAYING) {
#ifdef ENABLE_CONTROL
			if (player->pProc->Wait() || player->GetPosData() < 0) {
#else
			char *p_buf;
			if (player->pProc->Wait() || player->Read(&p_buf, 0) < 0 
				|| strstr(p_buf, "Exiting...")) {
#endif
				eof = true;
				break;
			}
        }

#ifdef WIN32
        {
            DWORD bytes;
            HANDLE hStdout=GetStdHandle(STD_OUTPUT_HANDLE);
            WriteFile(hStdout, player->console, strlen(player->console), &bytes, 0);
        }
#else
		//printf("-> %s\n", player->console);
#endif

        player->WaitEvent(250);
    }

end:
	printf("Exit PlayThread..\n");
    player->SetState(PLAYER_STOPPING);
    player->SetState(PLAYER_IDLE);
    player->pProc->Cleanup();
    player->pos = -1;
    player->hWnd = 0;

	printf("PlayThread Exited\n");
	
    return 0;
}

#if !defined(WIN32) && defined(SIMPLEST_PLAYTHREAD)
void *simplePlayThread(void *arg)
{
    CMPlayer *player = (CMPlayer *)arg;

    printf("Enter PlayThread..\n");

    player->SetState(PLAYER_PLAYING);
    while (player->state == PLAYER_PLAYING) {
		if (player->pProc->Wait()) break;
        usleep(1000*250);
    }

	printf("Exiting PlayThread..\n");

    player->SetState(PLAYER_STOPPING);
    player->SetState(PLAYER_IDLE);
    player->pProc->Cleanup();
    player->pos = -1;
    player->hWnd = 0;

	printf("PlayThread Exited\n");
	
    return 0;
}
#endif

bool CMPlayer::IsPlaying()
{
#if defined(NO_PLAYTHREAD)
	if (state != PLAYER_PLAYING) return false;
	if (pProc->Wait() == 0) return true;

	SetState(PLAYER_STOPPING);
	SetState(PLAYER_IDLE);
	pProc->Cleanup();
	pos = -1;
	hWnd = 0;
	return false;
#else
	return this->state == PLAYER_PLAYING;
#endif
}
#ifdef ENABLE_VIS
void WINAPI AudioSampleThread(CMPlayer *player)
{
	char buf[2 * 576];
	DWORD bytes;
	do {
		if (!ReadFile(player->pipeAudioSamples, buf, sizeof(buf), &bytes, 0)) break;
		(*player->pfnAudioSample)(buf, bytes);
	} while (player->pipeAudioSamples);
}
#endif


CMPlayer::CMPlayer(const char* path):callback(0),state(PLAYER_IDLE),pos(-1),duration(0), m_id(-1)
{
#ifdef ENABLE_VIS
	pfnAudioSample = 0,
	pipeAudioSamples = NULL;
#endif

    pProc = new CProcessWrapper(0);
#ifdef WIN32
	//hMutexQuery = CreateMutex(0, 0, 0);
    hEventState = CreateEvent(0, 0, 0, 0);
#else
    pthread_mutex_init(&m_mutex, NULL);
    pthread_cond_init(&m_cond, NULL);
#endif
    memset(&audio, 0, sizeof(audio));
    memset(&video, 0, sizeof(video));
	memset(&discInfo, 0, sizeof(discInfo));
    console = (char*)malloc(CONSOLE_BUF_SIZE);
	assert(console);
	mppath = (char*)malloc(MAX_PATH);
	assert(mppath);
	if(path) SetPlayerPath(path);
}

CMPlayer::~CMPlayer()
{
    free(console);
	console = NULL;
	free(mppath);
	mppath = NULL;
	delete pProc;
	pProc = NULL;
#ifdef WIN32
   // CloseHandle(hMutexQuery);
    CloseHandle(hEventState);
#else
    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_cond);
#endif
}

void CMPlayer::ParseInfo(char* id, char* v)
{
    if (!strcmp(id, "LENGTH")) {
        duration = (int)(atof(v) * 1000);
    } else if (!strcmp(id, "VIDEO_FORMAT")) {
        video.format = hex2int(v + 2);
    } else if (!strcmp(id, "VIDEO_ID")) {
        video.id = atoi(v);
    } else if (!strcmp(id, "VIDEO_BITRATE")) {
        video.bitrate = atoi(v);
    } else if (!strcmp(id, "VIDEO_WIDTH")) {
        video.width = atoi(v);
    } else if (!strcmp(id, "VIDEO_HEIGHT")) {
        video.height = atoi(v);
    } else if (!strcmp(id, "VIDEO_FPS")) {
        video.fps = (float)atof(v);
    } else if (!strcmp(id, "VIDEO_ASPECT")) {
        video.aspect = (float)atof(v);
    } else if (!strcmp(id, "VIDEO_CODEC")) {
        if (v[0] == 'f' && v[1] == 'f') v += 2;
		const int codecLen = 32;
        strncpy_s(video.codec, codecLen, v, strlen(v));
    } else if (!strcmp(id, "AUDIO_ID")) {
        audio.id = atoi(v);
    } else if (!strcmp(id, "AUDIO_FORMAT")) {
        audio.format = atoi(v);
    } else if (!strcmp(id, "AUDIO_BITRATE")) {
        audio.bitrate = atoi(v);
    } else if (!strcmp(id, "AUDIO_RATE")) {
        audio.samplerate = atoi(v);
    } else if (!strcmp(id, "AUDIO_NCH")) {
        audio.channels = atoi(v);
    } else if (!strcmp(id, "AUDIO_CODEC")) {
        if (v[0] == 'f' && v[1] == 'f') v += 2;
		const int codecLen = 32;
        strncpy_s(audio.codec, codecLen, v, strlen(v));
    }
	else if (!strcmp(id, "DVD_DISC_ID"))
	{
		memcpy(discInfo.disc_id, v, strlen(v));
		isDISC = true;
	}
	else if (!strcmp(id, "DVD_VOLUME_ID"))
	{
		memcpy(discInfo.volume_id, v, strlen(v));
		isDISC = true;
	}
}

void CMPlayer::DumpInfo()
{
	printf(
	"duration:%d\n"
	"video"
	"\tformat:0x%x\n"
	"\tid:%d\n"
	"\tbitrate:%d\n"
	"\twidth:%d\n"
	"\theight:%d\n"
	"\tfps:%d\n",
	duration,
	video.format,
	video.id,
	video.bitrate,
	video.width,
	video.height,
	video.fps);
}

int CMPlayer::DoCallback(int eventid, void* arg)
{
    if (callback) {
        return (*callback)(eventid, arg, this);
    } else {
        return 0;
    }
}

int CMPlayer::WaitEvent(int timeout)
{
#ifdef WIN32
    return WaitForSingleObject(hEventState, timeout);
#else
	//FIXME:
	usleep(1000);
	return 0;

    int ret;
    struct timespec ts;

	DD;
    pthread_mutex_lock(&m_mutex);
	DD;

    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout;
    ret = pthread_cond_timedwait(&m_cond, &m_mutex, &ts);

	DD;
    pthread_mutex_unlock(&m_mutex);
	DD;
    return ret;
#endif
}

void CMPlayer::SetState(PLAYER_STATES newstate)
{
    if (state != newstate) {
        state = newstate;
        DoCallback(PC_STATE_CHANGE, (void*)newstate);
    }
}

bool CMPlayer::Command(const char* cmd, char** result)
{
    bool success = false;
//    int ret;
//    int cmdlen;
	string strCmd;

#ifndef ENABLE_CONTROL
	fprintf(stderr, "Try to call %s\n But play control is disabled!\n", cmd);

	return false;
#else
	strCmd = cmd;
	strCmd += '\n';					// Append new line char
    console[0] = 0;

#ifdef WIN32
	//WaitForSingleObject(hMutexQuery, INFINITE);
#else
	printf("CMD: %s\n", cmd);
	DD;
    pthread_mutex_lock(&m_mutex);
	DD;
#endif

	cmdlen = strCmd.length();
	DD;
    if (pProc->Write(strCmd.c_str(), cmdlen) == cmdlen) {
		DD;
        if (result) {
            char *p = 0;
            Sleep(100);
			DD;
            ret = Read(&p, 0);
			DD;
			printf("%s\n", p);
            if (ret > 0) {
                p = strrchr(p, '=');
                if (p)
                    p++;
                else
                    p = console;
            } else {
                p = console;
            }
            *result = p;
        }
        success = true;
    }
	else {
		DD;
	}

#ifdef WIN32
	//ReleaseMutex(hMutexQuery);
#else
	DD;
    pthread_mutex_unlock(&m_mutex);
	DD;
#endif

    return success;
#endif
}

int CMPlayer::Read(char** pbuf, int bufsize)
{
    int bytes = -1;
    char *buf = console;
    int interval = 10;
    if (pbuf) {
        if (*pbuf)
            buf = *pbuf;
        else
            *pbuf = buf;
    }
    if (!bufsize) bufsize = CONSOLE_BUF_SIZE;

// #ifdef WIN32
//     WaitForSingleObject(hMutexQuery, INFINITE);
// #endif

    bytes = pProc->Read(buf, bufsize - 1);

// #ifdef WIN32
//     ReleaseMutex(hMutexQuery);
// #endif

    if (bytes >= 0) {
        buf[bytes] = 0;
        DoCallback(PC_CONSOLE_OUTPUT, buf);
    }
    return bytes;
}

void CMPlayer::Stop()
{
    if (state == PLAYER_PLAYING || state == PLAYER_PAUSED) {
#ifdef ENABLE_VIS
		if (pipeAudioSamples) {
			WaitForSingleObject(hThreadAudioSample, 500);
			TerminateThread(hThreadAudioSample, 0);
			CloseHandle(hThreadAudioSample);
			CloseHandle(pipeAudioSamples);
			pipeAudioSamples = NULL;
		}
#endif
        SetState(PLAYER_STOPPING);
        Command("quit");
#ifdef WIN32
        SetEvent(hEventState);
        WaitForSingleObject(hThread, 200);
        Sleep(200);
        pProc->Cleanup();
#else
        pProc->Cleanup(); //make Read() in PlayThread unblock and return -1
        pthread_cond_signal(&m_cond);
        pthread_join(m_thread, NULL);
#endif

        SetState(PLAYER_IDLE);
    }
}

bool CMPlayer::Seek(float seekpos, int type)
{
    const int bufSize = 32;
	char buf[bufSize];
    int bufPos = 0;
    bufPos += sprintf_s(buf, bufSize, "seek %1.2f %d", seekpos, type);
    if (state == PLAYER_PAUSED) {
        sprintf_s(buf+bufPos, bufSize-bufPos, "\npause");
    }
	
    if (Command(buf)) {
        GetPosData();
        return true;
    } else {
        return false;
    }
}

int CMPlayer::GetPos()
{
    return pos;
}

int CMPlayer::GetPosData()
{
    char* result = 0;
    if (Command("get_time_pos", &result)) {
        int n = (int)(atof(result) * 1000);
        pos = n;
        return pos;
    } else {
        return -1;
    }
}

void CMPlayer::RefreshVideo()
{
    Command("refresh_vo");
}

int CMPlayer::Pause()
{
    if (Command("pause")) {
        if (state == PLAYER_PAUSED) {
            SetState(PLAYER_PLAYING);
            return PLAYER_PLAYING;
        } else {
            SetState(PLAYER_PAUSED);
            return PLAYER_PAUSED;
        }
    } else {
        return -1;
    }
}

int CMPlayer::Step()
{
    if (Command("step")) {
        return 0;
    } else {
        return -1;
    }
}

bool CMPlayer::Play(const char* filename, HWND hWndParent, const char* options, PLAYER_PRIORITY priority)
{
	const int bufSize = 1024;
	char cmd[bufSize];
	bool success;
	int bufPos = 0;

	if (state == PLAYER_PLAYING) {
		Stop();
		Sleep(300);
	}

	pos = 0;

	if (strlen(filename) > 0)
	{
		bufPos += sprintf_s(cmd+bufPos, bufSize-bufPos, "\"%s\" \"%s\" -quiet", mppath, filename);
	}
	else
	{
		bufPos += sprintf_s(cmd+bufPos, bufSize-bufPos, "\"%s\" -quiet", mppath);
	}

#ifdef ENABLE_CONTROL
	bufPos += sprintf_s(cmd + bufPos, bufSize - bufPos, " -slave");
#endif

#define FIXME
#ifdef FIXME
	if (hWndParent) {
		bufPos += sprintf_s(cmd+bufPos, bufSize-bufPos, " -wid %d", hWndParent);
	}
#endif

#ifdef ENABLE_VIS
	HANDLE pipeAudioOut = 0;

	if (pfnAudioSample) {
		SECURITY_ATTRIBUTES saAttr = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
		//CProcessWrapper::MakePipe(pipeAudioSamples, pipeAudioOut, 512, false, true);
		CreatePipe(&pipeAudioSamples, &pipeAudioOut, &saAttr, 512);
		SetHandleInformation( pipeAudioSamples, HANDLE_FLAG_INHERIT, 0);
		bufPos += sprintf_s(cmd + bufPos, bufSize - bufPos, " -ao dsound,pcm:pipe=%d", pipeAudioOut);
	}
#endif

	if (options) {
		sprintf_s(cmd + bufPos, bufSize - bufPos, "%s -identify", options);
	}

#if defined(WIN32)
	pProc->flags = SF_REDIRECT_STDIN | SF_REDIRECT_STDOUT | SF_WIN32;
#elif defined(ENABLE_CONTROL)
	pProc->flags = SF_REDIRECT_OUTPUT;
#else
	pProc->flags = SF_REDIRECT_STDOUT; /*No control*/
#endif

	DoCallback(PC_CONSOLE_OUTPUT, cmd);

 	if (!strncmp(filename, "mf://", 5)) {
		char *curdir = _strdup(filename + 5);
		char *p = strrchr(curdir, '\\');
		if (!p) {
			success = pProc->Create(cmd);
		} else {
			*p = 0;
			success = pProc->Create(cmd, curdir);
		}
		free(curdir);
	} else {
		success = pProc->Create(cmd);       
	}

#ifdef ENABLE_VIS
	if (pipeAudioOut) CloseHandle(pipeAudioOut);
#endif
	if (!success) {
        fprintf(stderr, "create proc failed\n");
        return false;
    }

	if(priority != PRIORITY_NONE)
		pProc->SetPriority(priority); /*-20 - 19, -20 is the highest*/

	hasAudio = true;
	hasVideo = true;
	hasVisual = false;
	isDISC = false;
	memset(&audio, 0, sizeof(audio));
	memset(&video, 0, sizeof(video));
	memset(&discInfo, 0, sizeof(discInfo));

	DWORD id;
	state = PLAYER_IDLE;

#ifdef WIN32
	hThread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)PlayThread,(void*)this,0,&id);
#else
	int i_ret;
    #if defined(NO_PLAYTHREAD)
		state = PLAYER_PLAYING;
		i_ret = 0;
    #elif defined(SIMPLEST_PLAYTHREAD)
        i_ret = pthread_create(&m_thread, NULL, simplePlayThread, (void*)this);
    #else
        i_ret = pthread_create(&m_thread, NULL, PlayThread, (void*)this);
    #endif
	if (i_ret != 0) {
		printf("Create Play Thread failed\n");
		goto error;
	}
#endif
#ifdef ENABLE_VIS
	hThreadAudioSample = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)AudioSampleThread,(void*)this,0,&id);
#endif
#ifdef WIN32
	for (int i = 0; i < 100 && state != PLAYER_PLAYING && WaitForSingleObject(hThread, 50) == WAIT_TIMEOUT; i++);
#else
	for (int i = 0; i < 4000 && state != PLAYER_PLAYING; i++) {
		//sleep 2 seconds mostly
		usleep(500);
	}
#endif
	if (state == PLAYER_PLAYING) {
		if (hWndParent) {
			hWnd = hWndParent;
		}
// #ifdef WIN32
// 		else {
// 			EnumWindows(EnumWindowCallBack, (LPARAM)this);
// 		}
// #endif
		DD;
		return true;
	}

#ifdef WIN32
	TerminateThread(hThread, 0);
#else
error:
#endif
	printf("open failed?\n");
	pProc->Cleanup();
	return false;
}

void CMPlayer::PrePlay(const char* filename, int milliSecs /* = 3000 */)
{
	char cmdStr[1024] = {0};
	sprintf_s(cmdStr, 1024, "\"%s\" \"%s\" -quiet -vo null -nosound", mppath, filename);
	pProc->Create(cmdStr);
	Sleep(milliSecs);
	pProc->Cleanup();
}

void CMPlayer::SetFS(bool fs)
{
	Command(fs ? "vo_fullscreen 1" : "vo_fullscreen 0");
}

bool CMPlayer::GetFS()
{
	char* result = 0;
	if (Command("get_vo_fullscreen", &result))
		return *result == '1';
	else return false;
}

// #ifdef WIN32
// BOOL CALLBACK CMPlayer::EnumWindowCallBack(HWND hwnd, LPARAM lParam)
// {
// 	((CMPlayer*)lParam)->hWnd = hwnd;
// 	return FALSE; 
// }
// #endif

void CMPlayer::SetPlayerPath(const char* playerPath)
{
	memset(mppath, 0, MAX_PATH);
	strcpy_s(mppath, MAX_PATH, playerPath);
}

const char* CMPlayer::GetPlayerPath() const
{
	return mppath;
}
