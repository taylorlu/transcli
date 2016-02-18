#ifndef _PROCESSWRAPPER
#define _PROCESSWRAPPER

#define SF_ALLOC 0x1
#define SF_SHOW_WINDOW 0x2
#define SF_WAIT_IDLE 0x4
#define SF_LOWER_PRIORITY 0x8			// Set low priority
#define SF_USE_AUDIO_PIPE 0x10
#define SF_USE_VIDEO_PIPE 0x20
#define SF_INHERIT_WRITE  0x40
#define SF_INHERIT_READ  0x80

#define SF_REDIRECT_STDIN 0x1000
#define SF_REDIRECT_STDOUT 0x2000
#define SF_REDIRECT_STDERR 0x4000
#define SF_REDIRECT_OUTPUT (0x8000 | SF_REDIRECT_STDOUT)

#ifdef _WIN32
#define SF_WIN32 0x10000
#include <Windows.h>
#else
#include <stddef.h>
#define BELOW_NORMAL_PRIORITY_CLASS    12
#define ABOVE_NORMAL_PRIORITY_CLASS   -12 
#endif

class  CProcessWrapper
{
public:
	CProcessWrapper(int flags = 0);
	~CProcessWrapper() {
		Cleanup();
	}
	void Cleanup();
	int Write(const void* data, size_t len = 0);
	int Read(void* buffer, int bufsize);
	int LoopRead(void* buffer, size_t bufsize);
	int ReadStdout();
	int ReadStderr();
	bool Spawn(const char* commandLine, const char* curDir = 0, const char** env = 0);
	bool Create(const char* commandLine, const char* curDir = 0, const char** env = 0, bool hasGui = false);
	static int Run(const char* commandLine, const char* curDir = 0, bool hidden = true);
	bool SetPriority(int priority);
	int Wait(int timeout = 0);
	static char** Tokenize(char* str, char delimiter = ' ');
	void CloseStdout();
	void CloseStdin();
	void CloseStderr();
	void AllocateBuffer(int bytes);
	void FreeBuffer();
	bool IsProcessRunning(int* pexitcode = 0);
	bool Terminate();
	bool Suspend(bool resume);
	void SetFlags(int fs) {flags = fs;}
	static bool MakePipe(int& readpipe, int& writepipe, int bufsize = 16384, bool inheritRead = false, bool inheritWrite = false, int flags = -1);
	int GetPid();
#ifdef WIN32
	HWND GetWindowHandle();
	int WaitIdle(int milliseconds);
	bool SetAffinity(int mask);
	void AddEnvVariable(char* envName, char* envValue);
	void PurgeVariable();
	DWORD GetExitCode();	 
	static int GetProcessIdByHandle(HANDLE hProcess);
	HANDLE hStdinWrite;
	HANDLE hStdoutRead;
	HANDLE hStderrRead;
	PROCESS_INFORMATION procInfo;
	HWND hWnd;
#else
	int m_pid;
    int m_child_pid;        /* save m_pid for use after child proc exited/terminated. */
    int m_status;
	bool Interrupt();
#endif
	int flags;
	char *conbuf;
	int conbufsize;
	int fdStdinWrite;
	int fdStdoutRead;
	int fdStderrRead;

	int fdReadAudio;
	int fdReadVideo;
	int fdWriteAudio;
	int fdWriteVideo;
private:
#ifdef WIN32
	int ReadOutput(HANDLE handle, int msecTimeout = 0);/*million seconds timeout*/
	static BOOL CALLBACK EnumWindowCallBack(HWND hwnd, LPARAM lParam);
#endif
	int ReadOutput(int fd);
	char *pchCommandLine;
	char *pchPath;
	int envsize;
};

#endif
