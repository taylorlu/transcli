#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <signal.h>
#include <list>
#include "processwrapper.h"
#include "bit_osdep.h"
#include "logger.h"
#include "BiThread.h"
#include "util.h"

#define SF_BUFFER_SIZE 256
#define STDIN_NO 0
#define STDOUT_NO 1
#define STDERR_NO 2

//static std::list<int> fdList;
//static CBIMutex fdListMutex;

void MakeUnblock(int fd)
{
	long f;

	//Setting control fd to no-block mode 
	f = fcntl(fd, F_GETFL, 0);
	f |= O_NONBLOCK;
#ifndef BSD
	f |= O_NDELAY;
#endif
	fcntl(fd, F_SETFL, f);

}

//static bool checkExistFd(int fd) 
//{
//	for(std::list<int>::iterator it=fdList.begin(); it!=fdList.end();++it) {
//		if(fd == *it) return true;
//	}
//	return false;
//}
//
//static void removeFd(int fd) 
//{
//	fdListMutex.Lock();
//	for(std::list<int>::iterator it=fdList.begin(); it!=fdList.end();++it) {
//		if(fd == *it) {
//			it = fdList.erase(it);
//			break;
//		}
//	}
//	fdListMutex.Unlock();
//}

CProcessWrapper::CProcessWrapper(int flags):conbuf(0), pchCommandLine(0), envsize(0)
{
	this->flags = flags;
    m_pid = -1;
	fdStdinWrite = 0;
	fdStdoutRead = 0;
	fdStderrRead = 0;
    fdReadAudio = -1;
    fdReadVideo = -1;
	fdWriteAudio = -1;
	fdWriteVideo = -1;
}

int CProcessWrapper::ReadStdout()
{
	return ReadOutput(fdStdoutRead);
}

int CProcessWrapper::ReadStderr()
{
	return ReadOutput(fdStderrRead);
}

void CProcessWrapper::Cleanup()
{
	CloseStdin();
	CloseStdout();
	CloseStderr();
	if (IsProcessRunning()) {
		Terminate();
	}
	FreeBuffer();
	//CloseHandle(hProcess);
}

//FIXME:
bool CProcessWrapper::Spawn(const char* commandLine, const char* curDir, const char** env)
{
    return Create(commandLine, curDir, env, false);
}

bool VerifyCommand(const char *exe)
{
	if (exe == NULL || *exe == '\0') return false;

	if (exe[0] == '.' || exe[0] == '/') {
		return FileExist(exe);
	}

	char *paths;
	paths = getenv("PATH");
	if (paths == NULL) return false;

	const char *delim = ":";
	char *ptr;

	ptr = strtok(paths, delim);

	while (ptr != NULL) {
		std::string path(ptr);
		path.append("/");
		path.append(exe);
		//fprintf(stderr, "%s\n", path.c_str());
		if (FileExist(path.c_str())) return true;
		ptr = strtok(NULL, delim);
	}

	return false;
}

void closeFd(int* fdArr)
{
     if(fdArr[0] > 0) {
          close(fdArr[0]);
          fdArr[0] = 0;
     }
      if(fdArr[1] > 0) {
          close(fdArr[1]);
          fdArr[1] = 0;
     }
}

bool CProcessWrapper::Create(const char* commandLine, const char* curDir, const char** env, bool hasGui)
{
#define FAIL_INFO(err) logger_err(LOGM_PROCWRAP, err); ret = false; break;
	enum {FD_R, FD_W, TWO_FD};
	int desc_r[TWO_FD] = {0}, desc_w[TWO_FD] = {0}, desc_err[TWO_FD] = {0};
	int fdAudio[TWO_FD] = {0}, fdVideo[TWO_FD] = {0};
	char **tokens = NULL;
	char *exe = NULL;
	bool ret = true;

	do {
		char endChar = '\"';
		if(*commandLine == '\"') {
			exe = strdup(commandLine + 1);
		} else {
			exe = strdup(commandLine);
			endChar = ' ';
		}
		for(char* p = exe; *p; ++p) {
			if(*p == endChar) {
				*p = NULL;
				break;
			}
		}

		if (!VerifyCommand(exe)) {
			FAIL_INFO("Failed to exe cmd\n");
		}

		if (flags & SF_REDIRECT_STDIN) {
			if (!MakePipe(desc_w[0], desc_w[1])) {
				FAIL_INFO("Create stdin pipe failed.\n");
			}
			logger_status(LOGM_PROCWRAP, "%s-Pipe: read:%d, write:%d\n", exe, desc_w[0], desc_w[1]);
		}

		if (flags & SF_REDIRECT_STDOUT) {
			if (!MakePipe(desc_r[0], desc_r[1])) {
				FAIL_INFO("Create stdout pipe failed\n");
			}
			if (((flags & SF_REDIRECT_OUTPUT) != SF_REDIRECT_OUTPUT) &&
				(flags & SF_REDIRECT_STDERR)) {
				if (!MakePipe(desc_err[0], desc_err[1])) {
					FAIL_INFO("Create stderr pipe failed.\n");
				}
			}
		}

		if(flags & SF_USE_AUDIO_PIPE) {
			if (!MakePipe(fdAudio[0], fdAudio[1])) {
				FAIL_INFO("Create audio pipe failed\n");
			}
		}

		if(flags & SF_USE_VIDEO_PIPE) {
			if (!MakePipe(fdVideo[0], fdVideo[1])) {
				FAIL_INFO("Create video pipe failed\n");
			}
		}

		char cmdLine[2048] = {0};
		if(flags & SF_USE_AUDIO_PIPE && flags & SF_USE_VIDEO_PIPE) {
			if(flags & SF_INHERIT_WRITE) {
				sprintf(cmdLine, commandLine, fdAudio[FD_W], fdVideo[FD_W]);
			} else {
				sprintf(cmdLine, commandLine, fdAudio[FD_R], fdVideo[FD_R]);
			}
		} else if(flags & SF_USE_AUDIO_PIPE) {
			if(flags & SF_INHERIT_WRITE) {
				sprintf(cmdLine, commandLine, fdAudio[FD_W]);
			} else {
				sprintf(cmdLine, commandLine, fdAudio[FD_R]);
			}
		} else if(flags & SF_USE_VIDEO_PIPE) {
			if(flags & SF_INHERIT_WRITE) {
				sprintf(cmdLine, commandLine, fdVideo[FD_W]);
			} else {
				sprintf(cmdLine, commandLine, fdVideo[FD_R]);
			}
		} else {
			strcpy(cmdLine, commandLine);
		}

		logger_status(LOGM_PROCWRAP, "[CMD]:%s\n", cmdLine);

		tokens = Tokenize(cmdLine);

		if (hasGui)  {
			//to do WHAT?
		}

		m_pid = fork();

		if (m_pid < 0) {
            closeFd(desc_r);
            closeFd(desc_w);
            closeFd(desc_err);
            closeFd(fdAudio);
            closeFd(fdVideo);
			FAIL_INFO("Fork failed.\n");
		}

		else if (m_pid > 0) {
			//parent
			if (flags & SF_REDIRECT_STDIN) {
				fdStdinWrite = desc_w[FD_W];
				logger_info(LOGM_PROCWRAP, "fdStdinWrite = %d\n", fdStdinWrite);
				close(desc_w[FD_R]);
			}

			if (flags & SF_REDIRECT_STDOUT) {
				fdStdoutRead = desc_r[FD_R];
				close(desc_r[FD_W]);
				
				if (((flags & SF_REDIRECT_OUTPUT) != SF_REDIRECT_OUTPUT) &&
					(flags & SF_REDIRECT_STDERR)) {
						fdStderrRead = desc_err[FD_R];
						close(desc_err[FD_W]);
						
				}
				else {
					fdStderrRead = -1;
				}
			}

			if(flags & SF_USE_AUDIO_PIPE) {
				if(flags & SF_INHERIT_WRITE) {
					fdReadAudio = fdAudio[FD_R];
					close(fdAudio[FD_W]);
				} else {
					fdWriteAudio = fdAudio[FD_W];
					close(fdAudio[FD_R]);
				}
			}

			if(flags & SF_USE_VIDEO_PIPE) {
				if(flags & SF_INHERIT_WRITE) {
					fdReadVideo = fdVideo[FD_R];
					close(fdVideo[FD_W]);
				} else {
					fdWriteVideo = fdVideo[FD_W];
					close(fdVideo[FD_R]);
				}
			}
		}
		else {
			//child
			if (curDir) {
				chdir(curDir);
			}
			
			if (flags & SF_REDIRECT_STDIN) {
				dup2(desc_w[FD_R], 0 /*STDIN_NO*/);
				close(desc_w[FD_W]);
			}

			if (flags & SF_REDIRECT_STDOUT) {
				dup2(desc_r[FD_W], 1 /*STDOUT_NO*/);
				close(desc_r[FD_R]);
				if ((flags & SF_REDIRECT_OUTPUT) == SF_REDIRECT_OUTPUT) {
					dup2(desc_r[FD_W], 2 /*STDERR_NO*/);
					close(desc_r[FD_R]);
				}
				else if (flags & SF_REDIRECT_STDERR) {
					dup2(desc_err[FD_W], 2 /*STDERR_NO*/);
					close(desc_err[FD_R]);
				}
			}

			if(flags & SF_USE_AUDIO_PIPE && flags & SF_USE_VIDEO_PIPE) {
				if(flags & SF_INHERIT_WRITE) {
					close(fdAudio[FD_R]);
					close(fdVideo[FD_R]);
				} else {
					close(fdAudio[FD_W]);
					close(fdVideo[FD_W]);
				}
				
			} else if(flags & SF_USE_AUDIO_PIPE) {
				if(flags & SF_INHERIT_WRITE) {
					close(fdAudio[FD_R]);
				} else {
					close(fdAudio[FD_W]);
				}
			} else if(flags & SF_USE_VIDEO_PIPE) {
				if(flags & SF_INHERIT_WRITE) {
              	    close(fdVideo[FD_R]);
				} else {
					close(fdVideo[FD_W]);
				}
			} 

			if(flags & SF_LOWER_PRIORITY) {
//				SetPriority(BELOW_NORMAL_PRIORITY_CLASS);
			}
                       
                 
			if (env) {
				//FIXME:
				execvp(exe, tokens);
			}
			else {
				execvp(exe, tokens);
			}

			if(flags & SF_INHERIT_WRITE) {
				close(fdAudio[FD_W]);
				close(fdVideo[FD_W]);
			} else {
				close(fdAudio[FD_R]);
				close(fdVideo[FD_R]);
			}

			fprintf(stderr, "Child thread return.\n");
			exit(27);
			//return true;
        }
	} while (false);

	free(exe);
	//free(pchCommandLine);
	free(tokens);

#undef FAIL_INFO
//	if(!ret) _exit(0);

	return ret;
}

int CProcessWrapper::Run(const char* commandLine, const char* curDir, bool hidden)
{
#if 1
	if (curDir) {
		chdir(curDir);
	}

	return system(commandLine);
#else
	int i_ret;

	if (Create(commandLine, curDir, NULL, false) == false) {
		logger_err(LOGM_PROCWRAP, "Create failed\n");
		return -1;
	}

	while (true) {
		i_ret = Wait(10);
		if (i_ret != 1) break; 
	}

	if (i_ret > 0) i_ret = 0;
	return i_ret;
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
		bytes = read(fdStdoutRead, conbuf + offset, conbufsize - 1 - offset);
		if (bytes <= 0) break;
		offset += bytes;
	}
	conbuf[offset]=0;
	return offset;
}

int CProcessWrapper::Read(void* buffer, int bufsize)
{
	if (fdStdoutRead <= 0) return -1;
	int bytes = read((int)fdStdoutRead, buffer, bufsize);
	if (bytes > 0)
		return bytes;
	else if (bytes == 0 || (bytes == -1 && errno != EBADF))
		return -1;
	else
		return 0;
}

int CProcessWrapper::LoopRead(void* buffer, size_t bufsize)
{
	size_t offset = 0;

	if (fdStdoutRead > 0) {
		while (offset < bufsize) {
			int bytes = read((int)fdStdoutRead, (char*)buffer + offset, (unsigned int)(bufsize - offset));
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

	size_t w = 0;
	while (w < len) {
		int ret = write(fdStdinWrite, (char*)data + w, len - w);
#ifdef DEBUG_PIPE
		printf("%d Write:%d, fd:%d, errno:%d\n", len, ret, fdStdinWrite, errno);
#endif

		if (ret == -1) return errno == ENOSPC ? 0 : -1;
		w += ret;
	}

//	logger_dbg(LOGM_PROCWRAP, LOGL_V, "Write %d\n", w);

	return w;
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

// Return value: 1,  end up normally
//               0,  running
//               -1, error
int CProcessWrapper::Wait(int timeout)
{
    if(m_pid < 0) return -1;
	int status = -1, ret = 0;
	int everySleepMs = 50;
	int count = timeout / everySleepMs;
	for (int i = 0; i < count; ++i) {
		int waitId = waitpid(m_pid, &status, WNOHANG);
		if(waitId == m_pid) {
			ret = 1;
			m_pid = -1;
			break;
		} else if(waitId == 0) {
			Sleep(everySleepMs);
			continue;
		} else {       // Error
			switch(errno) {
			case ECHILD:
				logger_err(LOGM_PROCWRAP, "No child process:%d\n", m_pid);
				break;
			case EINTR:
				logger_err(LOGM_PROCWRAP, "The call is interrupted.\n");
				break;
			case EINVAL:
				logger_err(LOGM_PROCWRAP, "Invalid parameters.\n");
				break;
			}
			ret = -1;
			m_pid = -1;
			break;
		}
	}
	
	return ret;
}

bool CProcessWrapper::Interrupt()
{
	int count = 0, status = -1;
	if(m_pid < 0) return false;

	int waitRet = 1;
	for (count = 0; count < 5; count++) {
		if(m_pid <= 0) break;
		status = kill(m_pid, SIGTERM);

		if (status == 0) {
			waitRet = Wait(100);		// Wait 0.2 second for end up
			break;
		}
		if (errno == ESRCH) {
			m_pid = -1;
			break;
		}

		waitRet = Wait(100);		// Wait 0.2 second for end up
		if(waitRet == -1) break;
	}

	if(waitRet != 1 && m_pid > 0) {
		kill(m_pid, SIGKILL);
		Wait(200);					// Wait 0.2 second for end up 
	}
	
	m_pid = -1;
	return true;
}

bool CProcessWrapper::Terminate()
{
    if (m_pid < 0) return false;
	return Interrupt();
}

bool CProcessWrapper::IsProcessRunning(int* pexitcode)
{
    //TODO
    return m_pid > 0;
}

bool CProcessWrapper::Suspend(bool resume)
{
    int sig;

	if (m_pid < 0) return false;

    if (resume) sig = SIGCONT;
    else sig = SIGSTOP;
    
    return kill(m_pid, sig); 
}

bool CProcessWrapper::SetPriority(int priority)
{
	return setpriority(PRIO_PROCESS, 0, priority) == 0;
}

bool CProcessWrapper::MakePipe(int& readpipe, int& writepipe, int bufsize, bool inheritRead, bool inheritWrite, int flags)
{
	int fdpipe[2] = {0};
	int ret;
 	
	ret = pipe(fdpipe);
	if (ret != 0) {
		logger_err(LOGM_PROCWRAP, "Create pipe failed.\n");
		return false;
	}

	readpipe = fdpipe[0];
	writepipe = fdpipe[1];
	/*int tmpFd1 = fdpipe[0];
	int tmpFd2 = fdpipe[1];
	while(checkExistFd(tmpFd1) || checkExistFd(tmpFd2)) {
		logger_info(LOGM_PROCWRAP, "Duplicate pipe fd: Read(%d), Write(%d).\n", tmpFd1, tmpFd2);
		ret = pipe(fdpipe);
		if (ret != 0) {
			logger_err(LOGM_PROCWRAP, "Create pipe failed.\n");
			return false;
		}
		close(tmpFd1);
		close(tmpFd2);
		tmpFd1 = fdpipe[0];
		tmpFd2 = fdpipe[1];
	}

	readpipe = tmpFd1;
	writepipe = tmpFd2;
	fdListMutex.Lock();
	fdList.push_back(tmpFd1);
	fdList.push_back(tmpFd2);
	fdListMutex.Unlock();*/

	return true;
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
			tokens[i] = ++p;
			while (*p) {
				p++;
				if (*(p-1) == '\"') {
					*(p-1) = '\0';
					break;
				}
			}
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
	if(fdStdoutRead > 0) {
		//commit(fdStdoutRead);
		close(fdStdoutRead);
		//removeFd(fdStdoutRead);
		fdStdoutRead = 0;
	}
}

void CProcessWrapper::CloseStdin()
{
	if(fdStdinWrite > 0) {
		//commit(fdStdinWrite);
		close(fdStdinWrite);
		//removeFd(fdStdinWrite);
		fdStdinWrite = 0;
	}
}

void CProcessWrapper::CloseStderr()
{
	if(fdStderrRead > 0) {
		//commit(fdStderrRead);
		close(fdStderrRead);
		//removeFd(fdStderrRead);
		fdStderrRead = 0;
	}
}

int CProcessWrapper::GetPid()
{
    return m_pid;
}

