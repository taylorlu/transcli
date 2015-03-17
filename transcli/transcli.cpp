#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

#include <string>
#include <fstream>

#ifndef WIN32
#include <signal.h>
#include <unistd.h>
#include <getopt.h>
#else
#include <direct.h>
#include "_getopt.h"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include "WorkManager.h"
#include "mplayerwrapper.h"

#include "bitconfig.h"
#include "logger.h"
#include "bit_osdep.h"
#include "util.h"
#include "clihelper.h"
#include "svnver.h"

using namespace std;

enum {
    CLI_GENERIC_ERROR = 100,
	CLI_INVALID_INVOKE_ARG,
	CLI_INVALID_INPUT_FILE,
	CLI_INVALID_PRESET_FILE,
	CLI_MANAGER_INIT_ERROR,
	CLI_CREATE_WORKER_ERROR,
	CLI_GEN_STD_PRESET_ERROR,
    CLI_UNSUPPORTED_FORMAT,
    CLI_INSUFFICIENT_QUOTA,
    CLI_CONTENT_NOT_FOUND,
	CLI_TIMEOUT_ERROR
};

bool g_completed = false;
int g_retCode = 0;

#ifndef WIN32
void kill_adapter_core()
{
	system("for i in `ps -ef | awk '/.\\/adptcore.bin/ {print $2}'`; do kill -9 $i; done");
}

void kill_dead_encoders()
{
	const char *cmd = "for i in `ps -ef | egrep 'adptcore\\.bin|mencoder|ffmpeg'| awk '{print $2,$5}' | grep -v '[0-9]\\{1,2\\}:[0-9]\\{1,2\\}' | awk '{print $1}'`; do kill -9 $i; done";
	system(cmd);
}

void signal_handler(int dunno)
{
	fprintf(stderr, "Handler a signal %d\n", dunno);

    if (g_completed && g_retCode == EC_NO_ERROR) { //FIXME: It's failed in WorkerManager.CleanUp()!
        //exit(EC_NO_ERROR);
	}
	else {
		//kill_adapter_core();
		g_completed = true;	
		Sleep(2000);
        //exit(CLI_GENERIC_ERROR);
	}
} 

#ifdef _DEBUG
void dump_handler(int signo)
{
    char buf[1024];
    char cmd[1024];
    FILE *fd;

    snprintf(buf, sizeof(buf), "/proc/%d/trancli", getpid());

    fd = fopen(buf, "r");
    if (fd == NULL) exit(0);

    if (!fgets(buf, sizeof(buf), fd)) exit(0);

    fclose(fd);

    if (buf[strlen(buf) - 1] == '\n') buf[strlen(buf) - 1] = '\0';
    snprintf(cmd, sizeof(cmd), "gdb %s %d", buf, getpid());

    system(cmd);

    //exit(0);
}
#endif //DEBUG

#else // Windows
#include <DbgHelp.h>
#pragma comment(lib, "DbgHelp.lib")
void CreateDumpFile(const char* dumpFilePathName, EXCEPTION_POINTERS *pException)  
{  
    // 创建Dump文件  
    HANDLE hDumpFile = CreateFileA(dumpFilePathName, GENERIC_WRITE, 0, NULL,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);  
  
    // Dump信息  
    MINIDUMP_EXCEPTION_INFORMATION dumpInfo;  
    dumpInfo.ExceptionPointers = pException;  
    dumpInfo.ThreadId = GetCurrentThreadId();  
    dumpInfo.ClientPointers = TRUE;  
  
    // 写入Dump文件内容  
    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, 
		MiniDumpNormal, &dumpInfo, NULL, NULL);  
  
    CloseHandle(hDumpFile);  
}  

LONG ApplicationCrashHandler(EXCEPTION_POINTERS *pException)  
{     
    // Get timestamp
	time_t timep;
	time(&timep);
	char* curTime = ctime(&timep);
	*(curTime+strlen(curTime)-1) = '\0';	// Remove newline char
	for(char* p=curTime; p && *p; ++p) {
		if((*p) == ':') {
			*p = '-';
		}
	}

	// Get mini dump file path with timestamp
	char curDir[260] = {0};
	if (GetAppDir(curDir, MAX_PATH) > 0) {
		std::string miniDmpPath = curDir;
		miniDmpPath.append("\\temp");
		// Make sure temp dir is created
		MakeDirRecursively(miniDmpPath.c_str());
		miniDmpPath.append(1,PATH_DELIMITER).append("transcli_");
		miniDmpPath.append(curTime).append(".dmp");
		
		CreateDumpFile(miniDmpPath.c_str(), pException);
	}
	
    return EXCEPTION_EXECUTE_HANDLER;  
}  
#endif



static void FinishedCallback()
{
	g_completed = true;	
}

int InitWorkManager()
{
	CWorkManager* pMan = CWorkManager::GetInstance();
	if (!pMan->Initialize(10)) {
		logger_err(LOGM_GLOBAL, "Work manager initialize failed.\n");
		return CLI_MANAGER_INIT_ERROR;
	}
	return 0;
}

static void usage(const char *program)
{
	const char* verStr = TS_MAJOR_VERSION"."TS_MINOR_VERSION"."SVN_REVISION;
	printf("Usage: %s [OPTION]\n"
		"Version:%s\n"
		"-p, --preset        Preset file (must be in UTF-8 encoding)\n"
		"-i, --infile        Input file path\n"
		"-o, --outfile       Output file path\n"
		"-O, --outdir        Output file dir\n"
		"-m, --meta          Meta data file path\n"
		"-T, --tempdir	     Temporary dir\n"
		"-l, --logfile       Debugging log file\n"
		"-v, --version       Version info\n"
		"-V, --verbose       Log level\n"
		"-n, --noshot        Don't do thumbnail.\n"
		"-d, --dimout        Output video dimension\n"
		"-t, --template      Template setting\n" 
		"-z, --tempdir       Temp folder\n"
		"-x, --urlindex      Main file index\n"
		"\n",
		program,
		verStr
	);

	return;
}

#define BUFFER_SIZE 1024
#define DEFAULT_MAX_LIVETIME 14*60*60

/****************************************************************************
 * main:
 ****************************************************************************/

#ifndef SVN_NOW
#define SVN_NOW __DATE__" "__TIME__
#endif

const cli_type_t CONST_CLI_TYPE = CLI_TYPE_PPLIVE;

void TripFile(const char* filename, int foreBytes, int rearBytes=0)
{
	FILE* fp = fopen(filename, "rb");
	FILE* fpOut = fopen("e:\\keylow.ts", "wb");
	const int bufsize = 102400;
	char bytes[bufsize] = {0};
	int skipCount = foreBytes/bufsize;
	int counter = 0;
	do {
		counter++;
		size_t readBytes = fread(bytes, 1, bufsize, fp);
		if(counter > skipCount) {
			if(readBytes > 0) {
				fwrite(bytes, 1, readBytes, fpOut);
			} else {
				break;
			}
		}
		if(counter*bufsize >= rearBytes) break;
	} while (true);
	fclose(fp);
	fclose(fpOut);
}

int main( int argc, char **argv )
{
	//TripFile("F:\\video\\problem\\black\\7d10dbdec0d5b90683e5029595b46dfc-9488656100.ts", 0, 102400000);
	//return 0;
	char psz_presetfile[MAX_PATH] = {0};
	char psz_infile[MAX_PATH*4] = {0};
	char psz_outfile[MAX_PATH] = {0};
	char psz_outdir[MAX_PATH] = {0};
	char psz_metafile[MAX_PATH] = {0};
	char psz_logfile[MAX_PATH] = {0};
	char psz_failed_dir[MAX_PATH] = {0};
	char psz_dimout[MAX_PATH] = {0};
	char psz_templatefile[MAX_PATH] = {0};
	char curDir[MAX_PATH] = {0};
	char psz_temp_dir[MAX_PATH] = {0};

	std::string strPreset;
	CCliHelper *clihelper = NULL;
	int i_verbose = 10;
	int main_url_index = 0;

	//bool enable_screenshot = false;
	//long shot_startpos = 0;
	//long shot_endpos = 0;
	//int shot_number = 1;
	bool disableThumbnail = false;

	const char* verStr = TS_MAJOR_VERSION"."TS_MINOR_VERSION"."SVN_REVISION;

	string strOutFileName;
	
	int64_t max_livetime = DEFAULT_MAX_LIVETIME; // in second

	int opt = 0, optind = 0;
	struct option lopts[] = {
		{ "help",        no_argument,       NULL, 'h' },
		{ "version",     no_argument,       NULL, 'v' },
		{ "preset",      required_argument, NULL, 'p' },
		{ "infile",      required_argument, NULL, 'i' },
		{ "outfile",     required_argument, NULL, 'o' },
		{ "outdir",      required_argument, NULL, 'O' },
		{ "noshot",      no_argument, NULL, 'n' },
		{ "meta",        required_argument, NULL, 'm' },
		{ "logfile",     required_argument, NULL, 'l' },
		{ "verbose",     required_argument, NULL, 'V' },
		{ "faileddir",   required_argument, NULL, 'f' },
		{ "dimension",   required_argument, NULL, 'd' },
		{ "template",    required_argument, NULL, 't' },
		{ "tempdir",    required_argument, NULL, 'z' },
		{ "urlindex",    required_argument, NULL, 'x' },
		{ 0, 0, 0, 0 },
	};

	const char *sopts = "hvp:i:o:O:m:l:V:f:nd:t:z:x:";

	while ((opt = getopt_long(argc, argv, sopts, lopts, &optind)) != -1) {
		switch (opt) {
		case '?':
		case 'h':
			usage(argv[0]);
			return CLI_INVALID_INVOKE_ARG;
		case 'v':
			printf("Transcli version:%s\nSVN Revision Date:%s\nBuild Date: %s\n",
				verStr, SVN_DATE, SVN_NOW);
			return 0;
		case 'p':
			strncpy(psz_presetfile, optarg, MAX_PATH);
			break;
		case 'i':
			// There may be multi files to join
			strncpy(psz_infile, optarg, MAX_PATH*4);
			break;
		case 'o':
			strncpy(psz_outfile, optarg, MAX_PATH);
			break;
		case 'O':
			strncpy(psz_outdir, optarg, MAX_PATH);
			break;
		case 'm':
			strncpy(psz_metafile, optarg, MAX_PATH);
			break;
		case 'l':
			strncpy(psz_logfile, optarg, MAX_PATH);
			break;
		case 'V':
			i_verbose = atoi(optarg);
			break;
		case 'f':
			strncpy(psz_failed_dir, optarg, MAX_PATH);
			break;
		case 'n':
			disableThumbnail = true;
			break;
		case 'd':
			strncpy(psz_dimout, optarg, MAX_PATH);
			break;
		case 't':
			strncpy(psz_templatefile, optarg, MAX_PATH);
			break;
		case 'z':
			strncpy(psz_temp_dir, optarg, MAX_PATH);
			break;
		case 'x':
			main_url_index = atoi(optarg);
			break;
		default:
			usage(argv[0]);
			return CLI_INVALID_INVOKE_ARG;
		}
	}

	if (*psz_infile == '\0') {
		logger_err(LOGM_GLOBAL, "Input file is not given\n");
		usage(argv[0]);
		return CLI_INVALID_INPUT_FILE;
	}
	/*
	const char* ext = strrchr(psz_infile, '.');
	if(_stricmp(ext, ".mp4") && _stricmp(ext, ".mp4")) {
		printf("Video format not support!\n", verStr);
		return -1;
	}*/

	logger_info(LOGM_GLOBAL, "Transcli Version %s\n\n", verStr);

	//set dir
#ifndef _WIN32
#ifdef _DEBUG
	logger_info(LOGM_GLOBAL, "============This is debug mode.===========\n", verStr);
	signal(SIGSEGV, dump_handler);
#else
	//signal(SIGSEGV, signal_handler);
#endif
	char *dirname = getenv("TRANSCODER_BINDIR");
	if (dirname != NULL) {
		chdir(dirname);
	}
#else 
	SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)ApplicationCrashHandler);
#endif

	////////////////////////////////////Initialize logger////////////////////////////
	if (*psz_logfile != '\0') {
		logger_init(psz_logfile);
	}
	else {
		logger_init(NULL);
#ifndef WIN32
		logger_set_coloring(1);
#endif
	}

	logger_set_verbose(i_verbose);

	if (GetAppDir(curDir, MAX_PATH) > 0) {
#ifdef WIN32
        SetCurrentDirectoryA(curDir);
#else
        chdir(curDir);
	// Config subtitle fonts
		//const char* systemFont = "/usr/share/fonts/msyh.ttf";
		//if(!FileExist(systemFont)) {
		//	TsCopyFile("./APPSUBFONTDIR/msyh.ttf", systemFont);
		//}
#endif
	}

	// If not set temp dir, create it in transcli folder
	if(!(*psz_temp_dir)) {	
        sprintf(psz_temp_dir, "%s%ctemp", curDir, PATH_DELIMITER);
	}
	// Make sure temp dir is created
	MakeDirRecursively(psz_temp_dir);

//#ifndef BUILD_FOR_PPLIVE
//	InitSocket();
//	if (!Auth_foo()) return -1;
//#endif

	/////////////////////If want to get screenshot, do that////////////
	//if (enable_screenshot) {
	//	return QuickScreenshot(psz_infile, psz_outdir, shot_startpos, shot_endpos, shot_number); 
	//}

// 	if(psz_dimout) {
// 		CMPlayer mplayerWrap("./codecs/mplayer.exe");
// 		const char* playerCmd = " -vo null -nosound -parse-only -really-quiet";
// 		mplayerWrap.Play(psz_infile, NULL, playerCmd);
// 
// 		FILE* fpInfo = fopen(psz_dimout, "wb");
// 		if(fpInfo) {
// 			fprintf(fpInfo, "<?xml version=\"1.0\"?>\n<DimensionInfo>\n<Dimension width=\"");
// 			fprintf(fpInfo, "%d", mplayerWrap.video.width);
// 			fprintf(fpInfo, "\" height=\"");
// 			fprintf(fpInfo, "%d", mplayerWrap.video.height);
// 			fprintf(fpInfo, "\"/>\n</DimensionInfo>");
// 			fclose(fpInfo);
// 		}
// 	}

	////////////////////////////////////Create prefs helper///////////////////////////
	if (*psz_presetfile == '\0') {
		logger_err(LOGM_GLOBAL, "Please provide preset.\n");
		return CLI_INVALID_PRESET_FILE;
	}
	/*if(*psz_infile == '\0' ||  !FileExist(psz_infile)) {
		logger_err(LOGM_GLOBAL, "Media file doesn't exist.\n");
		return CLI_INVALID_INPUT_FILE;
	}*/

	clihelper = new CCliHelperPPLive;
	if (clihelper == NULL) {
		logger_err(LOGM_GLOBAL, "Failed to create prefs helper.\n");
		return CLI_GENERIC_ERROR;
	}

	CWorkManager* pMan = CWorkManager::GetInstance();
	int workId = -1;

	do {
		///////////////////////////////////Process prefs//////////////////////////////
        g_retCode = clihelper->GetStdPrefString(psz_infile, psz_outdir, psz_outfile, psz_presetfile, psz_templatefile,
			strPreset, psz_temp_dir, disableThumbnail, main_url_index);
        if (g_retCode != 0) {
			logger_err(LOGM_GLOBAL, "Failed to process prefs.\n");
            g_retCode = CLI_GEN_STD_PRESET_ERROR;
			break;
		}

#ifdef DEBUG_TRANSCLI
		printf("\n%s\n", strPreset.c_str());
#endif

		///////////////////////////////Init Manager and Create worker/////////////////////////////
        g_retCode = InitWorkManager();
        if (g_retCode != 0) {
			logger_err(LOGM_GLOBAL, "Failed to init worker manager\n");
			break;
		}

		CTransWorker* pWorker = pMan->CreateWorker(&workId, WORKER_TYPE_SEP);

		if (pWorker == NULL) {
			logger_err(LOGM_GLOBAL, "Failed to create new worker\n");
            g_retCode = CLI_CREATE_WORKER_ERROR;
			break;
		}

		pWorker->SetFinishCallbackForCli(FinishedCallback);
		//The worker will start at once
        g_retCode = pMan->CreateWorkerTask(workId, 0/*uuid*/, strPreset.c_str(), psz_outfile);
        if (g_retCode != 0) {
			logger_err(LOGM_GLOBAL, "Failed to create task.\n");
            g_retCode = pMan->GetErrorCode(workId);
			break;
		}
		
		strOutFileName = pWorker->GetOutputFileName(0);
		int taskDur = pWorker->GetTaskDuration()/1000;
		
		logger_info(LOGM_GLOBAL, "Worker is started, task duration is %ds.\n", taskDur);
		if(taskDur > 0) {	// Calculate timeout limit.
			int cpuCoreNum = pMan->GetCPUCoreNum();
			int multiFactor = 12;
			if(cpuCoreNum < 4) {
				multiFactor = 16;
			} else if(cpuCoreNum < 6) {
				multiFactor = 12;
			} else if(cpuCoreNum <= 8) {
				multiFactor = 10;
			} else {
				multiFactor = 8;
			}
			max_livetime = taskDur*multiFactor;
		}
	} while (false);

#ifndef WIN32
	signal(SIGHUP, signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGQUIT, signal_handler); 	
	signal(SIGPIPE, signal_handler); 
#endif

	// waiting for completing	
    while(g_retCode == 0) {
		if(g_completed) break;
		/*clock_t runTime = clock();
		if (runTime > 0 && runTime/CLOCKS_PER_SEC > max_livetime) {
			logger_err(LOGM_GLOBAL, "Timeout (%d min), maybe the transcoder is hunged up!!\n", (int)(max_livetime/60) );
#ifndef WIN32
			kill_adapter_core();
#endif
            g_retCode = CLI_TIMEOUT_ERROR;
			break;
		}*/

		Sleep(2000);
	}

	// Clean up
	if (clihelper != NULL) delete clihelper;
 	
    if(g_retCode == 0) {
        g_retCode = pMan->GetErrorCode(workId);
	}
    logger_info(LOGM_GLOBAL, "Completed! Return code: %d\n", g_retCode);
	logger_uninit();

    return g_retCode;
}
