#include "processwrapper.h"
#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#include <Psapi.h>
#include <process.h>
#include <io.h>
#endif

#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "BiThread.h"
#include "logger.h"

#define SF_BUFFER_SIZE 256

//NOTE: if filename is a dir, it's also return false
static bool FileExist(const char* filename)
{
#ifdef _WIN32
	DWORD ret = GetFileAttributesA(filename);

	if (ret == INVALID_FILE_ATTRIBUTES) return false;

	return ((ret & FILE_ATTRIBUTE_ARCHIVE) != 0 ||
				(ret & FILE_ATTRIBUTE_READONLY) != 0 ||
				(ret & FILE_ATTRIBUTE_HIDDEN) != 0);
#else
	struct stat stat_ret;
	
	if (stat(filename, &stat_ret) != 0) return false;

	return (stat_ret.st_mode & S_IFREG) != 0;
#endif
}

CProcessWrapper::CProcessWrapper(int flags):envsize(0),pchCommandLine(0),conbuf(0)
{
	this->flags = flags;

	hStdinWrite = 0;
	hStdoutRead = 0;
	hStderrRead = 0;

	fdStdinWrite = 0;
	fdStdoutRead = 0;
	fdStderrRead = 0;
	memset(&procInfo, 0, sizeof(procInfo));
}

int CProcessWrapper::ReadStdout()
{
	if (flags & SF_WIN32) {
		return ReadOutput(hStdoutRead);
	} else {
		return ReadOutput(fdStdoutRead);
	}
}

int CProcessWrapper::ReadStderr()
{
	if (flags & SF_WIN32) {
		return ReadOutput(hStderrRead);
	} else {
		return ReadOutput(fdStderrRead);
	}
}

void CProcessWrapper::Cleanup()
{
	hWnd = 0;
	//if (IsProcessRunning()) {
	Terminate();
	//}
	CloseStdin();
	CloseStdout();
	CloseStderr();
	FreeBuffer();
	if(procInfo.hProcess) CloseHandle(procInfo.hProcess);
	memset(&procInfo, 0, sizeof(procInfo));
}

int CProcessWrapper::WaitIdle(int milliseconds)
{
	return WaitForInputIdle(procInfo.hProcess, milliseconds);
}

typedef DWORD (WINAPI *PFNGetProcessId)(HANDLE hProcess);
typedef BOOL (WINAPI *PFNEnumProcesses)(DWORD* pProcessIds, DWORD cb, DWORD* pBytesReturned);
typedef HANDLE (WINAPI *PFNOpenThread)(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwProcessId);

static HMODULE hKernel = LoadLibraryA("kernel32.dll");
static HMODULE hPsapi = LoadLibraryA("Psapi.dll");
static PFNGetProcessId pfnGetPid = (PFNGetProcessId)GetProcAddress(hKernel, "GetProcessId");
static PFNOpenThread pfnOpenThread = (PFNOpenThread)GetProcAddress(hKernel, "OpenThread");
static PFNEnumProcesses pfnEnumProc = (PFNEnumProcesses)GetProcAddress(hPsapi, "EnumProcesses");

int CProcessWrapper::GetProcessIdByHandle(HANDLE hProcess)
{
	if (pfnGetPid) {
		return (*pfnGetPid)(hProcess);
	} else if (pfnEnumProc) {
		DWORD dwProcessID = (DWORD)-1;
		DWORD aProcesses[256], cbNeeded, cProcesses;
		unsigned int i;

		if (!(*pfnEnumProc)( aProcesses, sizeof(aProcesses), &cbNeeded ) ) return -1;

		// Calculate how many process identifiers were returned.
		cProcesses = cbNeeded / sizeof(DWORD);

		// Print the name and process identifier for each process.
		for ( i = 0; i < cProcesses; i++ ) {
			HANDLE hTempProcess = OpenProcess( PROCESS_QUERY_INFORMATION, FALSE, aProcesses[i] );
			if (!hTempProcess) continue;
			CloseHandle( hTempProcess );
			if(hTempProcess == hProcess) {
				dwProcessID = aProcesses[i];
			}
		}
		return dwProcessID;
	}
	return 0;
}

static char* GetAppName(const char* commandLine)
{
	char *appname;
	const char *p;

	p = strrchr(commandLine, '.');
	if (p && !_stricmp(p + 1, "bat")) {
		return _strdup("cmd.exe");
	} else if (*commandLine == '\"') {
		appname = _strdup(commandLine + 1);
		char* quoteChar = strchr(appname, '\"');
		*quoteChar = 0;
	} else {
		appname = _strdup(commandLine);
		char* spacChar = strchr(appname, ' ');
		if (spacChar) {
			*spacChar = 0;
		}
	}
	return appname;	
}

bool CProcessWrapper::Spawn(const char* commandLine, const char* curDir, const char** env)
{
	int fdStdinRead = 0, fdStdinOld = 0;
	int fdStdoutWrite = 0, fdStdoutOld = 0;
	int fdStderrWrite = 0, fdStderrOld = 0;

	Cleanup();

	if (flags & SF_REDIRECT_STDIN) {
		MakePipe(fdStdinRead, fdStdinWrite, 256 * 1024, false, false);
		fdStdinOld = _dup(0);
		_dup2(fdStdinRead, 0);
	}
	if (flags & SF_REDIRECT_STDOUT) {
		MakePipe(fdStdoutRead, fdStdoutWrite, 256 * 1024, false, false);
		fdStdoutOld = _dup(1);
		_dup2(fdStdoutWrite, 1);
		if ((flags & SF_REDIRECT_OUTPUT) == SF_REDIRECT_OUTPUT) {
			fdStderrOld = _dup(2);
			_dup2(fdStdoutWrite, 2);
		}
	}
	if (flags & SF_REDIRECT_STDERR) {
		MakePipe(fdStderrRead, fdStderrWrite, 4096, false, false);
		fdStderrOld = _dup(2);
		_dup2(fdStderrWrite, 2);
	}

	char *pchCommandLine = _strdup(commandLine);
	char *oldCurDir = 0;
	char **tokens;
	char *exe;
	tokens = Tokenize(pchCommandLine);
	if (tokens[0][0] == '\"') {
		exe = _strdup(tokens[0] + 1);
		char *p = strrchr(exe, '\"');
		if (p) *p = 0;
	} else {
		exe = _strdup(tokens[0]);
	}
	if (curDir) {
		oldCurDir = (char*)malloc(MAX_PATH);
		GetCurrentDirectoryA(MAX_PATH, oldCurDir);
		SetCurrentDirectoryA(curDir);
	}
	if (env) {
		procInfo.hProcess = (HANDLE)_spawnvpe( _P_NOWAIT, exe, tokens, env);
	} else {
		procInfo.hProcess = (HANDLE)_spawnvp( _P_NOWAIT, exe, tokens);
	}
	if (procInfo.hProcess == INVALID_HANDLE_VALUE) {
		procInfo.hProcess = 0;
	} else {
		procInfo.dwProcessId = GetProcessIdByHandle(procInfo.hProcess);
	}

	if (oldCurDir) {
		SetCurrentDirectoryA(oldCurDir);
		free(oldCurDir);
	}

	if(flags & SF_LOWER_PRIORITY) {
		SetPriority(BELOW_NORMAL_PRIORITY_CLASS);
	}

	free(exe);
	free(tokens);
	free(pchCommandLine);

	if (fdStdinRead > 0) _close(fdStdinRead);
	if (fdStdoutWrite > 0) _close(fdStdoutWrite);
	if (fdStderrWrite > 0) _close(fdStderrWrite);
	if (fdStdinOld > 0) _dup2( fdStdinOld,	 0);
	if (fdStdoutOld > 0) _dup2( fdStdoutOld, 1 );
	if (fdStderrOld > 0) _dup2( fdStderrOld, 2 );

	if (!procInfo.hProcess) {
		Cleanup();
		return false;
	}

	flags &= ~SF_WIN32;
	return true;
}

bool CProcessWrapper::Create(const char* commandLine, const char* curDir, const char** env, bool hasGui)
{
	BOOL fSuccess; 
	SECURITY_ATTRIBUTES saAttr;
	HANDLE hChildStdoutRd = 0, hChildStdoutWr = 0;
	HANDLE hChildStderrRd = 0, hChildStderrWr = 0;
	HANDLE hChildStdinRd = 0, hChildStdinWr = 0;
	STARTUPINFOA siStartInfo;

	memset(&siStartInfo, 0, sizeof(siStartInfo));
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
	saAttr.bInheritHandle = TRUE; 
	saAttr.lpSecurityDescriptor = NULL; 

	Cleanup();

	if (flags & SF_REDIRECT_STDIN) {
		CreatePipe(&hChildStdinRd, &hChildStdinWr, &saAttr, 65536);
		SetHandleInformation( hChildStdinWr, HANDLE_FLAG_INHERIT, 0);
		siStartInfo.hStdInput = hChildStdinRd;
	}
	if (flags & SF_REDIRECT_STDOUT) {
		CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 65536);
		SetHandleInformation(hChildStdoutRd, HANDLE_FLAG_INHERIT, 0);
		siStartInfo.hStdOutput = hChildStdoutWr;
		if ((flags & SF_REDIRECT_OUTPUT) == SF_REDIRECT_OUTPUT) {
			siStartInfo.hStdError = siStartInfo.hStdOutput;
		}
	}
	if (flags & SF_REDIRECT_STDERR) {
		CreatePipe(&hChildStderrRd, &hChildStderrWr, &saAttr, 65536);
		SetHandleInformation(hChildStderrWr, HANDLE_FLAG_INHERIT, 0);
		siStartInfo.hStdError = hChildStderrWr;
	}

	// Set up members of the STARTUPINFO structure.
	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.dwFlags |= STARTF_USESHOWWINDOW;
	if (hasGui)  {
		siStartInfo.wShowWindow = SW_SHOW;
	} else {
		siStartInfo.wShowWindow = SW_HIDE;
		if (flags & (SF_REDIRECT_STDIN | SF_REDIRECT_STDOUT | SF_REDIRECT_STDERR)) {
			siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
			//if (!siStartInfo.hStdOutput) siStartInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
			if (!siStartInfo.hStdError) siStartInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
		}
	}

	char* appname = GetAppName((char*)commandLine);
	if (appname[1] != ':' && appname[0] != '/') {
		// is relative path
		if (!FileExist(appname)) {
			free(appname);
			appname = 0;
		}
	}

	fSuccess = CreateProcessA(0,
			(LPSTR)commandLine,		// command line
			NULL,					// process security attributes
			NULL,					// primary thread security attributes
			TRUE,					// handles are inherited
			0,						// creation flags
			NULL,					// process' environment
			curDir,					// current directory
			&siStartInfo,			// STARTUPINFO pointer
			&procInfo);				// receives PROCESS_INFORMATION

	if (appname) free(appname);

	if (hasGui) {
		WaitIdle(3000);
		GetWindowHandle();
	}

	if (hChildStdoutWr) CloseHandle(hChildStdoutWr);
	if (hChildStdinRd) CloseHandle(hChildStdinRd);
	hStdinWrite = hChildStdinWr;
	hStdoutRead = hChildStdoutRd;
	hStderrRead = hChildStderrRd;

	if (!fSuccess) {
		Cleanup();
		return false;
	}

	flags |= SF_WIN32;
	return true;
}

int CProcessWrapper::Run(const char* commandLine, const char* curDir, bool hidden)
{
#if 1
	BOOL fSuccess; 
	SECURITY_ATTRIBUTES saAttr;
	STARTUPINFOA siStartInfo;
	PROCESS_INFORMATION siProcInfo;

	memset(&siStartInfo, 0, sizeof(siStartInfo));
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
	saAttr.bInheritHandle = TRUE; 
	saAttr.lpSecurityDescriptor = NULL; 

	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.dwFlags |= STARTF_USESHOWWINDOW;
	siStartInfo.wShowWindow = hidden ? SW_HIDE : SW_SHOW;

	char* appname = GetAppName((char*)commandLine);

	fSuccess = CreateProcessA(appname,
			(LPSTR)commandLine,			// command line
			NULL,					// process security attributes
			NULL,					// primary thread security attributes
			FALSE,					// handles are inherited
			0,						// creation flags
			NULL,					// process' environment
			curDir,					// current directory
			&siStartInfo,			// STARTUPINFO pointer
			&siProcInfo);			// receives PROCESS_INFORMATION

	free(appname);

	if (fSuccess) {
		DWORD exitcode;
		WaitForSingleObject(siProcInfo.hProcess, INFINITE);
		GetExitCodeProcess(siProcInfo.hProcess, &exitcode);
		return exitcode;
	} else {
		return -1;
	}
#else
	char *oldCurDir = 0;
	char *cmdline = _strdup(commandLine);
	char **tokens;
	char *exe;
	tokens = Tokenize(cmdline);
	if (commandLine[0] == '\"') {
		char *end = strchr(commandLine + 1, '\"');
		int len = end - commandLine - 1;
		exe = (char*)malloc(len + 1);
		memcpy(exe, commandLine + 1, len);
		exe[len] = 0;
	} else {
		exe = _strdup(tokens[0]);
	}
	if (curDir) {
		oldCurDir = (char*)malloc(MAX_PATH);
		GetCurrentDirectory(MAX_PATH, oldCurDir);
		SetCurrentDirectory(curDir);
	}
	int ret = _spawnvp( _P_WAIT, exe, tokens );
	if (oldCurDir) {
		SetCurrentDirectory(oldCurDir);
	}
	free(exe);
	free(tokens);
	free(cmdline);
	return ret;
#endif
}

int CProcessWrapper::ReadOutput(int fd)
{
	if (!conbuf || conbufsize <= 0) {
		if (flags & SF_ALLOC) {
			conbuf = (char*)malloc(SF_BUFFER_SIZE);
			conbufsize = SF_BUFFER_SIZE;
		} else {
			return -1;
		}
	}

	int offset=0;
	for(;;) {
		int bytes;
		if (offset >= conbufsize - 1) {
			if (flags & SF_ALLOC) {
				conbufsize <<= 1;
				conbuf = (char*)realloc(conbuf, conbufsize);
			} else {
				break;
			}
		}
		bytes = _read(fdStdoutRead, conbuf + offset, conbufsize - 1 - offset);
		if (bytes <= 0) break;
		offset += bytes;
	}
	conbuf[offset]=0;
	return offset;
}

int CProcessWrapper::ReadOutput(HANDLE handle, int msecTimeout /* = 0 */)
{
	if (!conbuf || conbufsize <= 0) {
		if (flags & SF_ALLOC) {
			conbuf = (char*)malloc(SF_BUFFER_SIZE);
			conbufsize = SF_BUFFER_SIZE;
		} else {
			return -1;
		}
	}
	/*COMMTIMEOUTS dB;
	GetCommTimeouts(handle,&dB);*/
	int offset=0;
	for(;;) {
		int bytes;
		if (offset >= conbufsize - 1) {
			if (flags & SF_ALLOC) {
				conbufsize <<= 1;
				conbuf = (char*)realloc(conbuf, conbufsize);
			} else {
				break;
			}
		}
		if (!ReadFile(handle, conbuf + offset, conbufsize - 1 - offset, (LPDWORD)&bytes, 0)) break;
		offset += bytes;
	}
	conbuf[offset]=0;
	return offset;
}

int CProcessWrapper::Read(void* buffer, int bufsize)
{
	if (flags & SF_WIN32) {
		DWORD bytes;
		return (hStdoutRead > 0 && ReadFile(hStdoutRead, buffer, bufsize, &bytes, 0)) ? bytes : -1;
	} else {
		if (fdStdoutRead <= 0) return -1;
		int bytes = _read((int)fdStdoutRead, buffer, bufsize);
		if (bytes > 0)
			return bytes;
		else if (bytes == 0 || (bytes == -1 && errno != EBADF))
			return -1;
		else
			return 0;
	}
}

int CProcessWrapper::LoopRead(void* buffer, size_t bufsize)
{
	size_t offset = 0;
	if ((flags & SF_WIN32) && hStdoutRead > 0) {
		while (offset < bufsize) {
			DWORD bytes;
			if (!ReadFile(hStdoutRead, (char*)buffer + offset, (DWORD)(bufsize - offset), &bytes, 0)) {
				if (offset == 0) return -1;
				break;
			}
			offset += bytes;
		}
		return (int)offset;
	} else if (fdStdoutRead > 0) {
		while (offset < bufsize) {
			int bytes = _read((int)fdStdoutRead, (char*)buffer + offset, (unsigned int)(bufsize - offset));
			if (bytes == 0) {
				if (offset == 0) return -1;
				break;
			} else if (bytes < 0) {
				if (errno == EBADF) continue;
				if (offset == 0) return 0;
				break;
			}
			offset += bytes;
		}
		return (int)offset;
	}
	return -1;
}

int CProcessWrapper::Write(const void* data, size_t len)
{
	if (!len) len = strlen((const char*)data);
	if (flags & SF_WIN32) {
		DWORD bytes = 0;
		return (hStdinWrite > 0 && WriteFile(hStdinWrite, data, (DWORD)len, &bytes, 0)) ? bytes : -1;
	} else {
		int ret = _write(fdStdinWrite, data, len);
		if (ret == -1) {
			switch(errno) {
			case EBADF:
				logger_err(LOGM_PROCWRAP, "CProcessWrapper::Write: Bad file descriptor!\n");
				break;
			case ENOSPC:
				logger_err(LOGM_PROCWRAP, "CProcessWrapper::Write: No space left on device!\n");
				break;
			case EINVAL:
				logger_err(LOGM_PROCWRAP, "CProcessWrapper::Write: Buffer was NULL!\n");
				break;
			default: 
				logger_err(LOGM_PROCWRAP, "CProcessWrapper::Write: Unexpected error!\n");
				break;
			}
		}
		return ret;
	}
}

void CProcessWrapper::AllocateBuffer(int bytes)
{
	if (conbuf) free(conbuf);
	conbuf = (char*)malloc(bytes);
	conbufsize = bytes;
}

void CProcessWrapper::FreeBuffer()
{
	if (conbuf) {
		free(conbuf);
		conbuf = 0;
	}
}

int CProcessWrapper::Wait(int timeout)
{
	switch (WaitForSingleObject(procInfo.hProcess, timeout)) {
	case WAIT_TIMEOUT:
		return 0;
	case WAIT_OBJECT_0:
		return 1;
	default:
		return -1;
	}
}

bool CProcessWrapper::Terminate()
{
	return procInfo.hProcess && TerminateProcess(procInfo.hProcess,0) ? true : false;
}

void CProcessWrapper::AddEnvVariable(char* envName, char* envValue)
{
	/*
	int varbytes = strlen(envName) + strlen(envValue) + 2;
	pchEnv = (char*)realloc(pchEnv, envsize + varbytes + 1);
	sprintf(pchEnv + envsize, "%s=%s", envName, envValue);
	envsize += varbytes;
	pchEnv[envsize] = 0;
	*/
}

void CProcessWrapper::PurgeVariable()
{
	/*
	if (pchEnv) {
		free(pchEnv);
	}
	*/
}

bool CProcessWrapper::IsProcessRunning(int* pexitcode)
{
	DWORD exitcode = 0;
	if (!procInfo.hProcess || !GetExitCodeProcess(procInfo.hProcess, &exitcode)) {
		if (pexitcode) *pexitcode = -1;
		return false;
	}
	if (exitcode == STILL_ACTIVE) return true;
	if (pexitcode) *pexitcode = exitcode;
	return false;

}

bool CProcessWrapper::Suspend(bool resume)
{
	if (!pfnOpenThread || !procInfo.dwProcessId) return false;

    HANDLE        hThreadSnap = NULL; 
    bool          bRet        = false; 
    THREADENTRY32 te32        = {0}; 
 
    // Take a snapshot of all threads currently in the system. 
    hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0); 
    if (hThreadSnap == INVALID_HANDLE_VALUE) 
        return (FALSE); 
 
    // Fill in the size of the structure before using it. 
    te32.dwSize = sizeof(THREADENTRY32); 
 
    // Walk the thread snapshot to find all threads of the process. 
    // If the thread belongs to the process, add its information 
    // to the display list.
 
    if (Thread32First(hThreadSnap, &te32)) { 
        do 
        { 
            if (te32.th32OwnerProcessID == procInfo.dwProcessId) 
            {
				HANDLE hThread = (*pfnOpenThread)(THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID);
				if (hThread) {
					if (resume)
						ResumeThread(hThread);
					else
						SuspendThread(hThread);
					CloseHandle(hThread);
				}
            } 
        }
        while (Thread32Next(hThreadSnap, &te32)); 
        bRet = true; 
    } 
 
    // Do not forget to clean up the snapshot object. 
    CloseHandle (hThreadSnap); 
 
    return (bRet); 
}

bool CProcessWrapper::SetPriority(int priority)
{
	return SetPriorityClass(procInfo.hProcess, priority) ? true : false;
}

bool CProcessWrapper::SetAffinity(int mask)
{
	return SetProcessAffinityMask(procInfo.hProcess, mask) ? true : false;
}

#define READ_FD 0
#define WRITE_FD 1

bool CProcessWrapper::MakePipe(int& readpipe, int& writepipe, int bufsize, bool inheritRead, bool inheritWrite, int flags)
{
	int fdpipe[2] = {0};
	static CBIMutex pipeMutex;
	pipeMutex.Lock();
	int pipeRet = _pipe(fdpipe, bufsize, flags == -1 ? O_NOINHERIT|O_BINARY : flags);
	bool ret = false;
	if (pipeRet == 0) {
		if (inheritRead) {
			readpipe = _dup(fdpipe[READ_FD]);
			_close(fdpipe[READ_FD]);
		} else {
			readpipe = fdpipe[READ_FD];
		}
		if (inheritWrite) {
			writepipe = _dup(fdpipe[WRITE_FD]);
			_close(fdpipe[WRITE_FD]);
		} else {
			writepipe = fdpipe[WRITE_FD];
		}
		ret = true;
	}
	pipeMutex.Unlock();

	return ret;
}

char** CProcessWrapper::Tokenize(char* str, char delimiter)
{
	char** tokens;
	int n = 1;
	int i;
	char *p;
	
	// find out number of tokens
	p = str;
	for (;;) {
		while (*p && *p != delimiter) p++;
		if (!*p) break;
		n++;
		while (*(++p) == delimiter);
	}
	// allocate buffer for array
	tokens = (char**)malloc((n + 1) * sizeof(char*));
	// store pointers to tokens
	p = str;
	for (i = 0; i < n; i++) {
		if (*p == '\"') {
			tokens[i] = p++;
			while (*p && *p != '\"') p++;
			while (*p && *p != delimiter) p++;
			if (!*p) {
				i++;
				break;
			}
		} else {
			tokens[i] = p;
			while (*p && *p != delimiter) p++;
			if (!*p) {
				i++;	
				break;
			}
		}
		*p = 0;
		while (*(++p) == delimiter);
	}
	tokens[i] = 0;
	return tokens;
}

void CProcessWrapper::CloseStdout()
{
	if (flags & SF_WIN32) {
		if (hStdoutRead) {
			CloseHandle(hStdoutRead);
			hStdoutRead = 0;
		}
	} else {
		int fd = fdStdoutRead;
		fdStdoutRead = 0;
		if (fd > 0 ) _close(fd);
	}
}

void CProcessWrapper::CloseStdin()
{
	if (flags & SF_WIN32) {
		if (hStdinWrite) {
			CloseHandle(hStdinWrite);
			hStdinWrite = 0;
		}
	} else {
		int fd = fdStdinWrite;
		fdStdinWrite = 0;
		if (fd > 0 ) {
			if (Wait(10) == 0) _commit(fd);
			_close(fd);
		}
	}
}

void CProcessWrapper::CloseStderr()
{
	if (flags & SF_WIN32) {
		if (hStderrRead) {
			CloseHandle(hStderrRead);
			hStderrRead = 0;
		}
	} else {
		int fd = fdStderrRead;
		fdStderrRead = 0;
		if (fd > 0 ) _close(fd);
	}
}

HWND CProcessWrapper::GetWindowHandle()
{
	hWnd = 0;
	EnumWindows( EnumWindowCallBack, (LPARAM)this ) ;
	return 0;
}

BOOL CALLBACK CProcessWrapper::EnumWindowCallBack(HWND hwnd, LPARAM lParam) 
{ 
	CProcessWrapper *me = (CProcessWrapper * )lParam; 
	DWORD ProcessId; 
	GetWindowThreadProcessId ( hwnd, &ProcessId ); 

	// note: In order to make sure we have the MainFrame, verify that the title 
	// has Zero-Length. Under Windows 98, sometimes the Client Window ( which doesn't 
	// have a title ) is enumerated before the MainFrame 

	TCHAR title[128];
	if ( ProcessId  == me->procInfo.dwProcessId && GetWindowText(hwnd, title, sizeof(title)) > 0) 
	{ 
		me->hWnd = hwnd; 
		return FALSE; 
	} 
	else 
	{ 
		// Keep enumerating 
		return TRUE; 
	} 
}

unsigned long CProcessWrapper::GetExitCode()
{
	DWORD code;
	if(!GetExitCodeProcess(procInfo.hProcess, &code))
		return 88888888;
	return code;
}

int CProcessWrapper::GetPid()
{
	return GetProcessIdByHandle(hWnd);
}

