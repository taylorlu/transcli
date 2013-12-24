#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include "BiThread.h"
#include "util.h"
#include "logger.h"
#include "bit_osdep.h"

/* maximum message length of logger */
#define MSGSIZE_MAX 3072

#define LOG_FILE_LEN_LIMIT 524288		// 512K

int  logger_levels[LOGM_MAX]; // verbose level of this module. initialized to -2
int  logger_level_base = LOGL_WARN;
int  logger_verbose   = 0;
bool logger_color     = 0;
int  logger_module    = 1;
FILE* logger_stream   = NULL;
char* logger_buffer   = NULL;
//static CBIMutex	logger_mutex;
volatile int logger_buzy = 0;

static bool logger_inited = false;

// If log file exists and exceed LOG_FILE_LEN_LIMIT, then rename it
void check_file_size(const char *log_file);

void logger_init(const char *log_file)
{
	int i;
	char *env = getenv("BIT_VERBOSE");
	if (env)
		logger_verbose = atoi(env);

	logger_uninit();

	if (log_file != NULL && *log_file != '\0') 
	{
		check_file_size(log_file);
		logger_stream = fopen(log_file, "a");
		if (logger_stream == NULL) {
			fprintf(stderr, "log file %s can not be opened to log, std err/out is used\n", log_file);
		}
		else {
			time_t timep;
			time (&timep);
			fprintf(logger_stream, "\n\n%s\n", ctime(&timep));
		}
	}

	logger_buffer = (char*)malloc(MSGSIZE_MAX * sizeof(char));
	if (logger_buffer == NULL)
	{
		fprintf(stderr, "log buffer can not be alloc\n"); //FIXME
		return;
	}

	for(i=0; i<LOGM_MAX; i++) logger_levels[i] = -2;

	logger_inited = true;
}

void logger_uninit(void)
{
	if(!logger_inited) return;
	if (logger_stream != NULL &&
		logger_stream != stderr &&
		logger_stream != stdout)
	{
		fclose(logger_stream);
		logger_stream = NULL;
	}

	free(logger_buffer);
	logger_buffer = NULL;

	logger_inited = false;
}

void logger_set_verbose(int lev)
{
	logger_verbose = lev;
}

void logger_set_mod_verbose(int mod, int lev)
{
	if (mod < 0 || mod >= MSGSIZE_MAX) return;

	logger_levels[mod] = lev;
}

#ifndef WIN32
void logger_set_coloring(int color)
{
	logger_color = (color != 0);
}
#endif

int logger_test(int mod, int lev)
{
	if (mod < 0 || mod >= MSGSIZE_MAX) return 0;

	return lev <= (logger_levels[mod] == -2 ? logger_level_base + logger_verbose : logger_levels[mod]);
}

static void set_msg_color(FILE* stream, int lev)
{
    static const unsigned char v_colors[10] = {9, 1, 3, 15, 7, 2, 2, 8, 8, 8};
    int c = v_colors[lev];

#ifdef BIT_COLOR_TEST
    /* that's only a silly color test */
    {
        int c;
        static int flag = 1;
        if (flag)
            for(c = 0; c < 24; c++)
                printf("\033[%d;3%dm***  COLOR TEST %d  ***\n", c>7, c&7, c);
        flag = 0;
    }
#endif

	if (logger_color)
        fprintf(stream, "\033[%d;3%dm", c >> 3, c & 7);
}

static void print_msg_module(FILE* stream, int mod)
{
    static const char *module_text[LOGM_MAX] = {
		"GLOBAL",
		"APT",
		"TS",
		"TS_WM",
		"TS_WK",
		"TS_WK_SEP",
		"TS_WK_UNI",
		"TS_FQ",
		"TS_VD",
		"VD_MPLAYER",
		"VD_MENCODER",
		"VD_AVS",
		"E_CLI",
		"TS_VE",
		"VE_H264",
		"VE_H265",
		"VE_XVID",
		"VE_CUDA264",
		"VE_FFMPEG",
		"VE_WM",
		"VE_VC1",
		"VE_MII",
		"VE_COPY",
        "TS_AE",
		"AE_MP3",
		"AE_FAAC",
		"AE_EAC3",
		"AE_FFMPEG",
		"AE_WM",
		"AE_COPY",
		"AVENC",
	    "AVENC_FFMPEG",   // AV encoder ffmpeg
		"TS_SO",
        "TS_MUX",
        "TS_HTTPD",
        "VF_CUDA",
        "VF_CPU",
		"AFRESAMPLE",
        "REQHANDLER",
        "PROCWRAP",
		"WATERMARK",
        "M_TASK",
        "M_TASKQ",
        "M_NODE",
        "M_NODEQ",
		"WATCHFOLDER",
		"WATCHFOLDERQ",
        "MASTER",
        "M_MAIN",
		"M_USERS",
		"M_UIHANDLER",
		"SOCKET",
		"FTP",
		"DISCOVERER",
		"SYSPERFINFO",
		"STREAMPREFS",
		"ROOTPREFS",
		"PLAYLIST",
		"SENDMAIL",
		"UTIL",
		"DB_SAVER",
		"H_STREAMING",
		"YUVUTIL"
    };

    int c2 = (mod + 1) % 15 + 1;

    if (!logger_module)
        return;
    if (logger_color)
        fprintf(stream, "\033[%d;3%dm", c2 >> 3, c2 & 7);
    fprintf(stream, "%8s", module_text[mod]);
    if (logger_color)
        fprintf(stream, "\033[0;37m");
    fprintf(stream, " ");
}

void logger_log(int mod, int lev, const char *format, ... )
{
    va_list va;
    static int header = 1;
	static int timestamp = 1;

	if (!logger_inited) return;

    if (!logger_test(mod, lev)) return; // do not display

	if (logger_stream == NULL ||
		logger_stream == stderr ||
		logger_stream == stdout)
	{
		logger_stream = (lev <= LOGL_WARN) ? stderr : stdout;
	}

	//logger_mutex.Lock();
	if(logger_buzy) return;
	logger_buzy = 1;

    va_start(va, format);
    vsnprintf(logger_buffer, MSGSIZE_MAX, format, va);
    va_end(va);
    logger_buffer[MSGSIZE_MAX-2] = '\n';
    logger_buffer[MSGSIZE_MAX-1] = 0;

#if defined(CONFIG_ICONV) && defined(MSG_CHARSET)
    if (logger_charset && strcasecmp(logger_charset, "noconv")) {
      char tmp2[MSGSIZE_MAX];
      size_t inlen = strlen(logger_buffer), outlen = MSGSIZE_MAX;
      char *in = logger_buffer, *out = tmp2;
      if (!old_charset || strcmp(old_charset, logger_charset)) {
        if (old_charset) {
          free(old_charset);
          iconv_close(msgiconv);
        }
        msgiconv = iconv_open(logger_charset, MSG_CHARSET);
        old_charset = strdup(logger_charset);
      }
      if (msgiconv == (iconv_t)(-1)) {
        fprintf(stderr,"iconv: conversion from %s to %s unsupported\n"
               ,MSG_CHARSET,logger_charset);
      }else{
      memset(tmp2, 0, MSGSIZE_MAX);
      while (iconv(msgiconv, &in, &inlen, &out, &outlen) == -1) {
        if (!inlen || !outlen)
          break;
        *out++ = *in++;
        outlen--; inlen--;
      }
      strncpy(logger_buffer, tmp2, MSGSIZE_MAX);
      logger_buffer[MSGSIZE_MAX-1] = 0;
      logger_buffer[MSGSIZE_MAX-2] = '\n';
      }
    }
#endif

	// Add modifier to log info
	switch(lev) {
		case LOGL_INFO:
			fprintf(logger_stream, "[INFO] ");
			break;
		case LOGL_WARN:
			fprintf(logger_stream, "[WARN] ");
			break;
		case LOGL_ERR:
			fprintf(logger_stream, "[ ERR] ");
			break;
		case LOGL_STATUS:
			fprintf(logger_stream, "[STAT] ");
			break;
	}

    if (header) print_msg_module(logger_stream, mod);

	//add timestamp
	time_t timep;
	struct tm *p;

	time(&timep);
	p = localtime(&timep);
	if (timestamp) fprintf(logger_stream, "[%02d/%02d %02d:%02d:%02d] ", 1 + p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);

	set_msg_color(logger_stream, lev);
    header = logger_buffer[strlen(logger_buffer)-1] == '\n' || logger_buffer[strlen(logger_buffer)-1] == '\r';

    fprintf(logger_stream, "%s", logger_buffer);
	
	// Every 5s flush once
    if(p->tm_sec%5 == 0 || LOGL_ERR == lev) fflush(logger_stream);
	//logger_mutex.Unlock();
	logger_buzy = 0;
}

void check_file_size(const char *log_file)
{
	FILE* fp = fopen(log_file, "r");
	if(fp) {
		fseek(fp, 0L, SEEK_END);
		long fLen = ftell(fp);
		fclose(fp);
		if(fLen > LOG_FILE_LEN_LIMIT) {
			time_t timep;
			time(&timep);
			char* curTime = ctime(&timep);
			*(curTime+strlen(curTime)-1) = '\0';	// Remove newline char
			for(char* p=curTime; p && *p; ++p) {
				if((*p) == ':') {
					*p = '-';
				}
			}

			// Get log path dir
			char* logpath = _strdup(log_file);
			char* lastDelimiter = strrchr(logpath, PATH_DELIMITER);
			if(lastDelimiter) *(lastDelimiter+1) = '\0';

			char newLogFile[256] = {0};
			if(lastDelimiter) {
				sprintf(newLogFile, "%s%s.log", logpath, curTime);
			} else {
				sprintf(newLogFile, "%s.log", curTime);
			}
			
			rename(log_file, newLogFile);
			free(logpath);
		}
	}
}
