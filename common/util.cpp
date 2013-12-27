#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <direct.h>
#ifndef WIN32
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <sys/wait.h>
#endif

#include "bitconfig.h"
#include "util.h"
#include "MediaTools.h"
#include "processwrapper.h"
#include "BiThread.h"
#include "bit_osdep.h"
#include "logger.h"
extern "C" {
#include "md5.h"
}
#ifndef WIN32
#define _abs64 llabs
#define _stat64 stat
#define max(x,y) ((x)>(y)?(x):(y))
#endif

using namespace std;

bool FileExist(const char* filePath)
{
	if(!filePath) return false;
#ifdef WIN32
	struct _stat64 stat_ret;
	if (_stat64(filePath, &stat_ret) != 0) return false;
#else
	struct stat stat_ret;
	if (ts_lstat(filePath, &stat_ret) != 0) return false;
#endif

	return (stat_ret.st_mode & S_IFREG) != 0;
}

int64_t StatFileSize(const char* filename)
{
	if(!filename) return 0;
#ifdef WIN32
	struct _stat64 stat_ret;
	if(_stat64(filename, &stat_ret)<0) {
		return 0;
	}
#else
	struct stat stat_ret;
	if(ts_lstat(filename, &stat_ret)<0) {
		return 0;
	}
#endif
	return stat_ret.st_size;
}

bool FolderExist(const char* folderPath)
{
	if(!folderPath) return false;
#ifdef WIN32
    DWORD dwAttributes = GetFileAttributesA(folderPath);
	if(dwAttributes == 0xFFFFFFFF) {
		return false;
	}
	if((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) {
		return true;
	}
	return false;
#else
	struct stat s;
	int ret = ts_stat(folderPath, &s);
	if(ret == 0 && (s.st_mode & S_IFDIR) ) {
		return true;
	}
	return false;
#endif
}

std::string FormatFileSize(int64_t kbytes)
{
	float unitM = 1024.0f;
	float unitG = 1024.0f*1024.0f;
	char formatStr[24] = {0};
	if(kbytes < unitM) {
		sprintf(formatStr,"%ldK", kbytes);
	} else if(kbytes < unitG) {
		sprintf(formatStr, "%1.2fM", (kbytes / unitM));
	} else {
		sprintf(formatStr, "%1.2fG", (kbytes / unitG));
	}
	return formatStr;
}

int MakeDirRecursively(const char* dirname)
{
#ifdef WIN32
	if(!dirname || strlen(dirname) < 4) return -1;

	char* _dirname = _strdup(dirname);
	char *p = _dirname;
	if (p[1] == ':') p += 2;
	int ret = 0;
	for(;;) {
		p = strchr(p + 1, PATH_DELIMITER);
		if(p) {		// Other delimiter exist
			*p = 0;
		}
		ret = _mkdir(_dirname);

		if(!p) break;	// Only one layer

		// Process other layer
		if (ret == ENOENT) {	// failed
			ret = -1;
			break;
		} 
		*p = PATH_DELIMITER;
	}
	free(_dirname);
	return ret;
#else
    char buf[1024];
	printf("==== Make App dir: %s \n", dirname);
    snprintf(buf, 1024, "mkdir -p %s", dirname);
    return system(buf);
#endif
}

#ifdef WIN32
#define APPDATA_ENV "APPDATA"
#define HOME_PATH_NAME "\\Broad Intelligence\\"
#else //no WIN32
#define APPDATA_ENV     "HOME"
#define HOME_PATH_NAME "/broadIntelligence/"
#endif

std::string GetHomeDir(const char* appName)
{
	string path;
	if (path.empty()) {		

#ifdef HAVE_MINDERIN_ENCODER
		path = "/home/videos"HOME_PATH_NAME;
#else
		char *p = getenv(APPDATA_ENV);
		if (p == NULL) {
			fprintf(stderr, "getenv(APPDATA_ENV) failed\n");
			return ".";
		}
		path = p;
		path += HOME_PATH_NAME;
		path += appName;
#endif

		if (!FolderExist(path.c_str())) {
			if(MakeDirRecursively(path.c_str()) != 0) {
				return ".";
			}
		}
	}
	return path;
}

std::string GetHomePath(const char* appName, const char* folderName /* = NULL */)
{
	string path = GetHomeDir(appName);
	if (folderName && (*folderName != '\0')) {
		path += PATH_DELIMITER;
		path += folderName;
	}
	if (!FolderExist(path.c_str())) {
		if(MakeDirRecursively(path.c_str()) != 0) {
			path = ".";
		}
	}
	return path;
}

int SafeRead(int fd, unsigned char* buffer, int bytes)
{
	int offset = 0;
	while (offset < bytes) {
		int n = _read(fd, buffer + offset, bytes - offset);
		if (n > 0) {
			offset += n;
		} else if (n == 0) {
			break;
		} else {
			if(n == -1 && errno == 8) {	// errno = EBADF
				continue;
			} else {
				break;
			}
		}
	}
	return offset;
}

int SafeWrite(int fd, unsigned char* buffer, int bytes)
{
	int offset = 0;

	while (offset < bytes) {
		int n = _write(fd, buffer + offset, bytes - offset);
		if (n < 0) {
			switch(errno) {
			case EBADF:
				perror("Bad file descriptor!");
				break;
			case ENOSPC:
				perror("No space left on device!");
				break;
			case EINVAL:
				perror("Invalid parameter: buffer was NULL!");
				break;
			default:
				// An unrelated error occured 
				perror("Unexpected error!");
			}
			return -1;
		}
		offset += n;
	}
	return offset;
}

std::string SecondsToTimeString(int secs, const char* connector) 
{
	if(secs < 0) return "";
	int hh = secs/3600;
	int ss = secs%3600;
	int mm = ss/60;
	ss = ss%60;
	char timeString[32] = {0};
	sprintf(timeString, "%02d%s%02d%s%02d", hh, connector, mm, connector, ss);
	return timeString;
}

// parsing time string, ex. 00:00:00
long ParsingTimeString(const char* strTime)
{
	const char *delimiters = ":";
	long h,m,s;
	
	if (strTime == NULL) return -1;
	if(!strchr(strTime, ':')) {
		return atoi(strTime);
	}

	char *tmpTimeStr = _strdup(strTime);
	char* tmpStr = tmpTimeStr;
	char* pch = Strsep(&tmpStr, delimiters);
	if (pch == NULL) {
		free(tmpTimeStr);
		return -1;
	}
	h = atoi(pch);

	pch = Strsep(&tmpStr, delimiters);
	if (pch == NULL) {
		free(tmpTimeStr);
		return -1;
	}
	m = atoi(pch);

	pch = Strsep(&tmpStr, delimiters);
	if (pch == NULL) {
		free(tmpTimeStr);
		return -1;
	}
	s = atoi(pch);

	free(tmpTimeStr);
	return h * 3600 + m * 60 + s;
}

// parsing time string, ex. 00:00:00.000
float ParsingFloatTimeString(const char* strTime)
{
	const char *delimiters = ":";
	int h = 0, m = 0;
	float s = 0.f;
	char *pch = NULL;

	if (strTime == NULL) return -1.f;

	if(!strchr(strTime, ':')) {
		return (float)atof(strTime);
	}

	char *tmpTimeStr = _strdup(strTime);
	char* tmpStr = tmpTimeStr;
	pch = Strsep(&tmpStr, delimiters);
	if (pch == NULL) return -1.f;
	h = atoi(pch);

	pch = Strsep(&tmpStr, delimiters);
	if (pch == NULL) return -1.f;
	m = atoi(pch);

	pch = Strsep(&tmpStr, delimiters);
	if (pch == NULL) return -1.f;
	s = (float)atof(pch);

	free(tmpTimeStr);
	return h * 3600 + m * 60 + s;
}

const char* mediaFormats[] = {".avi", ".wmv", ".asf", ".mpg", ".mpeg", ".mpe",
			".mp4", ".3gp", ".3gp2", ".3gpp", ".3g2", ".ogm",
			".m4v", ".flv", ".f4v", ".ts", ".m2ts", ".m2t", ".mkv", ".mov", ".qt",
			".rm", ".rmvb", ".vob", ".iso", ".264"};
bool IsMediaFormat(const char* fileName)
{
	std::string extStr(fileName);
	size_t dotIdx = extStr.find_last_of('.');
	if(dotIdx != extStr.npos) {
		extStr = extStr.substr(dotIdx);
		for (int i=0; i<sizeof(mediaFormats)/sizeof(mediaFormats[0]); ++i) {
			if(_stricmp(extStr.c_str(), mediaFormats[i]) == 0) return true;
		}
		return false;
	}
	return false;
}

const vector<string>& GetLocalIpList()
{
	static vector<string> ipList;

	if (ipList.empty()) {
#ifdef WIN32
		char hostname[256] = {0};

		if (gethostname(hostname, sizeof(hostname)) != 0) {
			fprintf(stderr, "gethostname failed\n");
			return ipList;
		}

//		printf("%s\n", hostname);
		hostent *hp = gethostbyname(hostname);

		if (hp == NULL) return ipList;

		struct in_addr  **pptr = (struct in_addr **)hp->h_addr_list;

		while (pptr != NULL && *pptr != NULL) {
//				printf("ip address: %s\n", inet_ntoa(**(pptr)));
				ipList.push_back(inet_ntoa(**(pptr++)));
		}
#else
		struct ifaddrs *ifaddr = 0, *ifa = 0;
		int s = 0;
		char host[NI_MAXHOST] = {0};

		if (getifaddrs(&ifaddr) == -1) return ipList; 

		/* Walk through linked list, maintaining head pointer so we
		  can free list later */

		for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {

			if (ifa->ifa_addr == NULL || 
				(ifa->ifa_name != NULL && strcmp(ifa->ifa_name, "lo") == 0)) continue;

			/* For an AF_INET interface address, display the address */

			if (ifa->ifa_addr->sa_family == AF_INET) {
				s = getnameinfo(ifa->ifa_addr,
						sizeof(struct sockaddr_in),
						host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
				if (s != 0) {
					fprintf(stderr, "getnameinfo() failed: %s\n", gai_strerror(s));
					continue;
				}
                
				//printf("address: %s\n", host);
				ipList.push_back(host);
			}
		}

		freeifaddrs(ifaddr);
#endif
	}

	return ipList;
}

bool  RemoveFile(const char* filePath)
{
	if(!filePath) return false;
#ifdef WIN32
    return (::DeleteFileA(filePath) == TRUE);
#else
	return (::remove(filePath) == 0);
#endif
}

bool  TsMoveFile(const char* srcFile, const char* destFile)
{
	if(!destFile || !destFile) {
		return false;
	}

	if(!FileExist(srcFile)) {
		logger_err(LOGM_UTIL_UTIL, "Source file doesn't exist when move file.\n");
		return false;
	}

#ifdef WIN32
    return (::MoveFileA(srcFile, destFile) == TRUE);
#else
	return (::rename(srcFile, destFile) == 0);
#endif
}

std::string ts_escape(const string &str)
{
	string ret;
	for (size_t i = 0; i < str.size(); i++) {
		if (str[i] == ' ' || str[i] == '!' ||
			str[i] == '"' || str[i] == '\\'||
			str[i] == '/' || str[i] == ':') {
			char t[4] = {0};
			sprintf(t, "%%%2x", str[i]);
			ret.append(t);
		}
		else {
			ret += str[i];
		}
	}

	return ret;
}

std::string GetStringBetweenDelims(const char* str, const char* delimStart, const char* delimEnd)
{
	if(!str) return "";
	std::string tmpStr = str;
	size_t delimStartPos = 0;
	size_t delimStartLen = 0;
	if(delimStart) {
		delimStartPos = tmpStr.find(delimStart);
		delimStartLen = strlen(delimStart);
	}
	size_t delimEndPos = std::string::npos;
	if(delimEnd) delimEndPos = tmpStr.find(delimEnd);
	if(delimStartPos != std::string::npos) {
		if(delimEndPos != std::string::npos) {
			return tmpStr.substr(delimStartPos+delimStartLen, delimEndPos-delimStartPos-delimStartLen);
		} else {
			return tmpStr.substr(delimStartPos+delimStartLen);
		}
	}
	return "";
}

string GetMd5(const char *data, int len)
{
	if (data == NULL || *data == '\0') {
		return ""; /*Md5("") is d41d8cd98f00b204e9800998ecf8427e*/
	}

	int i = 0;
	char md5buf[33] = {NULL};
	MD5_CTX m;

	MD5Init(&m, 0);
	if (len == 0) len = strlen(data);
	MD5Update(&m, (unsigned char*)data, len);
	MD5Final(&m);
	for(i = 0; i < 16; i++) {
		sprintf(md5buf + i * 2, "%02x", m.digest[i]);
	}
	md5buf[32] = '\0';
	return md5buf;
}

string GetMd5(const string& str)
{
	return GetMd5(str.c_str(), str.length());
}

string GetMd5Part(const string& str, int ret_size)
{
	return GetMd5(str.c_str(), str.length()).substr(0, ret_size);
}

std::string GetFileMd5(const char* filePath)
{
	FILE *inFile = fopen (filePath, "rb");
	if (inFile == NULL) {
		logger_err(LOGM_UTIL_UTIL, "%s can't be opened.\n", filePath);
		return "";
	}

	MD5_CTX mdContext;
	int bytes = 0;
	unsigned char data[1024] = {0};
	char md5Str[33] = {0};
	
	MD5Init (&mdContext, 0);
	while ((bytes = fread (data, 1, 1024, inFile)) != 0)
		MD5Update (&mdContext, data, bytes);
	MD5Final (&mdContext);
	for(int i = 0; i < 16; i++) {
		sprintf(md5Str + i * 2, "%02x", mdContext.digest[i]);
	}

	fclose (inFile);
	return md5Str;
}
//FIXME: Cleanup this function!
bool MatchFilterCase(const char *filename, const char *filters)
{
	if (filename == NULL || filters == NULL) return false;

	// If filter contain "*.*", then return true (1)
	if(strstr(filters, "*.*")) return true;

	// Get file extension from filename and turn to lower case
	const char* lastDot = strrchr(filename, '.');
	if(!lastDot) return false;	// No file extension

	char fileExt[10] = {0};
	strncpy(fileExt, lastDot, 9);
	for (char* p=fileExt; *p; ++p) {
		if(*p >= 'A' && *p <= 'Z') *p = (char)(tolower(*p));
	}

	// Turn filter string to lower case
	char strFilters[128] = {0};
	strncpy(strFilters, filters, 127);
	for (char* p=strFilters; *p; ++p) {
		if(*p >= 'A' && *p <= 'Z') *p = (char)(tolower(*p));
	}

	if(strstr(strFilters, fileExt)) {
		return true;
	}
	return false;
// 	int slen1 = strlen(str1);
// 	int slen2 = strlen(str2);
// 
// 	char *matchmap = (char*)malloc((slen1+1)*(slen2+1));
// 
// 	memset(matchmap, 0, (slen1+1)*(slen2+1));
// 	matchmap[0] = 1;
// 	
// 	int i, j, k;
// 
// 	for(i = 1; i<= slen1; ++i) {
// 		for(j = 1; j<=slen2; ++j) {
// 			if (matchmap[(i-1) * (slen2+1) + j-1] == 0) continue;
// 			if (toupper(str1[i-1]) == toupper(str2[j-1]) || str2[j-1] == '?') {
// 				matchmap[i * (slen2+1) + j] = 1;
// 				if( i != slen1 || j >= slen2) continue;
// 				for ( k = j+1 ; k <= slen2 ; ++k ) {
// 					if( '*' != str2[k-1]) break;
// 					matchmap[i * (slen2+1) + k] = 1;
// 				}
// 			}
// 			else if(str2[j-1] == '*') {
// 				for(k = i-1; k<=slen1; ++k) {
// 					matchmap[k * (slen2+1) + j] = 1;
// 				}
// 			}
// 		}
// 
// 		for (k = 1; k<=slen2; ++k) {
// 			if (matchmap[i * (slen2+1) + k]) break;
// 		}
// 
// 		if(k > slen2) {
// 			free(matchmap);
// 			return 0;
// 		}
// 	}
// 
// 	char ret = matchmap[slen1 * (slen2+1) + slen2];
// 
// 	free(matchmap);
//return ret;
}

static bool ScanDir(const string &curFolder /*IN*/, const char *filter /*IN*/, int curDepth, int maxDepth,  vector<string> &outFileList /*OUT*/)
{
	if (++curDepth > maxDepth && maxDepth > 0) return true;

	string name(curFolder);

#ifdef WIN32
	WIN32_FIND_DATAA FindFileData;   
	HANDLE hFind;
	bool ret = true;

	name.append("\\");
	string findPath(name);
	findPath.append("*");

	hFind = FindFirstFileA(findPath.c_str(), &FindFileData);   

	if (hFind == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "Warning: Invalid find path: %s\n", findPath.c_str());
		return false;
	}

	do {
		if((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
			if (FindFileData.cFileName[0] == '.') {
				continue;
			}
			string subPath(name);
			subPath.append(FindFileData.cFileName);
			ScanDir(subPath, filter, curDepth, maxDepth, outFileList);
		}
#if 0
		else if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE
			||FindFileData.dwFileAttributes & FILE_ATTRIBUTE_READONLY
			||FindFileData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
		{
#else
		else {
#endif
			if ( MatchFilterCase(FindFileData.cFileName, filter) == 0) continue;

			string fullPath(name);

			fullPath.append(FindFileData.cFileName);

			//*************************************//
			outFileList.push_back(fullPath);
			//*************************************//
		}
	} while( /*ret &&*/ FindNextFileA(hFind, &FindFileData));

	FindClose(hFind); 
	return /*ret*/true;
#else

	DIR   *dp;

	dp = opendir(name.c_str());
	if (dp == NULL) {
		fprintf(stderr, "Failed to open dir:%s\n", name.c_str());
		return false;
	}

	struct   dirent   *dirp;
	struct stat stStat;

	while ((dirp = readdir(dp)) != NULL) {
		if((dirp->d_name[0]== '.') && (dirp->d_name[1]== '.' || dirp->d_name[1]==0)) continue;

		string fullPath(name);
		fullPath += "/";
		fullPath += dirp->d_name;

		if (stat(fullPath.c_str(), &stStat) == -1) continue;

		if (stStat.st_mode & S_IFREG) {
			if ( MatchFilterCase(dirp->d_name, filter) == 0) continue;
			outFileList.push_back(fullPath);
		}
		else if (stStat.st_mode & S_IFDIR)
		{
			ScanDir(fullPath, filter, curDepth, maxDepth, outFileList);
		}
	}

	closedir(dp);	
    return true;
#endif
}

bool GetFileListInFolder(const char *path /*IN*/, std::vector<string>& outFileList /*OUT*/, 
						const char *filters, bool recursive)
{
	if (path == NULL || *path == '\0') {
		fprintf(stderr, "Invalid parameter\n");
		return false;
	}

	if (filters == NULL) {
		filters = "*.*";
	}

	if (!FolderExist(path)) {
		logger_err(LOGM_UTIL_UTIL, "%s is not a valid folder\n", path);
		return false;
	}

	if (!ScanDir(path, filters, 0, recursive?-1:1, outFileList)) {
		logger_err(LOGM_UTIL_UTIL, "Failed to scan watching folder %s, filter %s\n", path, filters);
		return false;
	}
	
	return true;
}

struct _cmd_t {
	const char *psz_cmd;
	char *psz_output;
	bool exited;
};

static unsigned long _RuningThread(void* arg)
{
	if (arg == NULL) return -1;

	_cmd_t *cmd = (_cmd_t*)arg;

	cmd->psz_output = NULL;
	if (cmd->psz_cmd == NULL) {
		cmd->exited = true;
		return -1;
	}

	CProcessWrapper proc;
	proc.flags = SF_REDIRECT_STDOUT | SF_ALLOC;

	bool success = (proc.Create(cmd->psz_cmd) && proc.ReadStdout() > 0);
	if (!success) {
		cmd->exited = true;
		return -1;
	}

	cmd->psz_output = _strdup(proc.conbuf);
	cmd->exited = true;
	return 0;
}

/*please free the return string*/
char *RunAndGetStdOut(const char *psz_cmd, unsigned timeout)
{
	if (psz_cmd == NULL) {
		fprintf(stderr, "RunAndGetStdOut(): Invalid paramter\n");
		return NULL;
	}
	struct _cmd_t cmd;

	cmd.psz_cmd = psz_cmd;
	cmd.psz_output = NULL;
	cmd.exited = false;

	if (timeout == 0) {
		_RuningThread(&cmd);
		if (cmd.psz_output == NULL) {
			fprintf(stderr,
				"RunAndGetStdOut(): there are something wrong to run cmd %s with timeout %u\n", 
				psz_cmd, timeout);
			return NULL;
		}
		return cmd.psz_output;
	}

	CBIThread::Handle h;
	int ret = CBIThread::Create(h, (CBIThread::ThreadFunc)_RuningThread, &cmd);

	if (ret != 0 || !h) {
		fprintf(stderr, "RunAndGetStdOut(): failed to create thread to run cmd %s\n", psz_cmd);
		return NULL;
	}

	DWORD t1 = GetTickCount();
	while (!cmd.exited && GetTickCount() - t1 < timeout) {
		Sleep(200);
	}

	if (!cmd.exited) {
		//FIXME: memory leak?
		CBIThread::Kill(h);
		fprintf(stderr, "RunAndGetStdOut(): timeout!\n");
		return NULL;
	}

	return cmd.psz_output;
}

void NormalizeResolution(int& w, int& h, int darNum, int darDen, int dividor)
{
	if(w <= 0 && h > 0) {
		w = h*darNum/darDen;
		if(dividor > 0) EnsureMultipleOfDivisor(w, dividor);
	}
	if(h <= 0 && w > 0) {
		h = w*darDen/darNum;
		if(dividor > 0) EnsureMultipleOfDivisor(h, dividor);
	}
}

void EnsureMultipleOfDivisor(int& num, int dividor)
{
	if(dividor <= 0) return;
	int remainNum = num%dividor;
	if(remainNum == 0) {
		return;
	} else if(remainNum > dividor/2) {
		num += dividor-remainNum;
	} else if (remainNum < dividor/2){
		num -= remainNum;
	} else {	// remainNum = divisor/2
		// if dividor is 4, when num=270, then 272(can be devide by 8) is better than 268
		if((num+dividor/2)%(dividor*2) == 0) {
			num += dividor/2;
		} else {
			num -= dividor/2;
		}
	}
}

bool TerminateProcess(int pid)
{
	if (pid < 0) return false;

#ifdef WIN32
//TODO
	return true;
#else
	int count = 0, status = -1;

	while (count < 10) {
		status = kill(pid, SIGTERM);

		if (status == 0) break;
		if (errno == ESRCH) {
			status = 0;
			break;
		}
		Sleep(100);
		count++;
	}

	for (count = 0; count < 10; count++) {
		int waitId = waitpid(pid, &status, WNOHANG);
		if(waitId == pid) {
			//OK
			return true;
		}
		else if(waitId == 0) {
			Sleep(100);
			continue;
		} else {
			//ERROR
			return false;
		}
	}

	//KILL this process if waitpid failed
	kill(pid, SIGKILL);

	//try the last waitpid
	waitpid(pid, &status, WNOHANG);

	return true;
#endif
}

int GetSecretCode(int seed)
{
	if (GetTickCount() % 3100 > 2500) Sleep(25);
	int ret = (int) (GetTickCount() / 3100 * 13);

	if (seed > 0) {
		ret |= (seed << 15);
	}

	return ret;
}

int GetRandBetween(int minVal, int maxVal)
{
	srand((unsigned int)(time(NULL)));
	int retVal = (rand()%(maxVal-minVal+1)) + minVal;
	return retVal;
}

int GetCpuCoreNum()
{
	SYSTEM_INFO sysinfo;
	GetSystemInfo( &sysinfo );
	return sysinfo.dwNumberOfProcessors;
}

char* ReadTextFile(const char* txtFile)
{
	if(!txtFile || !(*txtFile)) return NULL;

	FILE* tmpfp = fopen(txtFile, "rb");
	if (tmpfp == NULL) {
		printf("File %s can not be opened\n", txtFile);
		return NULL;
	}

	fseek(tmpfp, 0L, SEEK_END);
	long bufLen = ftell(tmpfp);
	fseek(tmpfp, 0L, SEEK_SET);
	char* buf = (char*)malloc(bufLen+1);
	if(buf) {
		memset(buf, 0, bufLen+1);
		fread(buf, 1, bufLen, tmpfp);
	}
	fclose(tmpfp);
	return buf;
}

char* Strsep(char** stringp, const char* delim) 
{
	char *begin, *end;
	 
	begin = *stringp;
	if (begin == NULL)
	return NULL;
	 
	/** A frequent case is when the delimiter string contains only one
	    character.  Here we don't need to call the expensive `strpbrk'
	    function and instead work using `strchr'.  */
	if (delim[0] == '\0' || delim[1] == '\0') {
	    char ch = delim[0];
	 
	    if (ch == '\0')
			end = NULL;
	    else {
			if (*begin == ch)
				end = begin;
			else if (*begin == '\0')
				end = NULL;
			else
				end = strchr (begin + 1, ch);
		}
	} else {
		/** Find the end of the token.  */
		end = strpbrk (begin, delim);
	}
	 
	if (end) {
	    /** Terminate the token and set *STRINGP past NUL character.  */
	    *end++ = '\0';
	    *stringp = end;
	} else {
		/** No more delimiters; this is the last token.  */
		*stringp = NULL;
	}
	return begin;
}

void* GetLastErrorMsg()
{
	DWORD dwErr = GetLastError();
	LPVOID lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_IGNORE_INSERTS,
	NULL, dwErr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),(LPTSTR) &lpMsgBuf, 0, NULL );
	return lpMsgBuf;
}

int64_t GetFreeSpaceKb(const char* filePath)
{
	if(!filePath || !(filePath) || strlen(filePath) < 2) return -1;

	ULARGE_INTEGER nFreeBytesAvailable;
    ULARGE_INTEGER nTotalNumberOfBytes;
	ULARGE_INTEGER nTotalNumberOfFreeBytes;
	if(filePath[0] == '\\' && filePath[1] == '\\' && strlen(filePath) > 5) {	// Share path
		const char* thirdSlash = strchr(filePath+2, '\\');
		if(thirdSlash) {
			const char* forthSlash = strchr(thirdSlash+1, '\\');
			char sharePath[MAX_PATH] = {0};
			strncpy(sharePath, filePath, forthSlash-filePath+1);
            BOOL bRet = GetDiskFreeSpaceExA(sharePath, &nFreeBytesAvailable, &nTotalNumberOfBytes, &nTotalNumberOfFreeBytes);
			if(!bRet) {
				LPVOID errMsg = GetLastErrorMsg();
				logger_err(LOGM_UTIL_UTIL, "GetDiskFreeSpaceEx failed: %s\n", (char*)errMsg);
				LocalFree(errMsg);
				return 0;
			}
			return (int64_t)(nFreeBytesAvailable.QuadPart/1024);
		}
	} else {		// Local path
		if(filePath[1] == ':') {
			char drivePath[3] = {0};
			strncpy(drivePath, filePath, 3);
            BOOL bRet = GetDiskFreeSpaceExA(drivePath, &nFreeBytesAvailable, &nTotalNumberOfBytes, &nTotalNumberOfFreeBytes);
			if(!bRet) {
				LPVOID errMsg = GetLastErrorMsg();
				logger_err(LOGM_UTIL_UTIL, "GetDiskFreeSpaceEx failed: %s\n", (char*)errMsg);
				LocalFree(errMsg);
				return 0;
			}
			return (int64_t)(nFreeBytesAvailable.QuadPart/1024);
		}
	}
	return -1;
}

bool FloatEqual(float a, float b, float epsilon)
{
	if(a-b > -epsilon && a-b < epsilon) return true;
	return false;
}

bool DoubleEqual(double a, double b, double epsilon)
{
	if(a-b > -epsilon && a-b < epsilon) return true;
	return false;
}

void OptimizeJpeg(const char* jpegFile)
{
	if(!FileExist(jpegFile)) return;
	CProcessWrapper proc;

	// Get temp output jpeg file
	std::string tmpOutFile = jpegFile;
	size_t lastDotIdx = tmpOutFile.find_last_of('.');
	if(lastDotIdx == std::string::npos) return;
	tmpOutFile.insert(lastDotIdx-1, "_jpegtran_tmp");

	// Get file size(if jpeg size > 10k, use progressive mode)
	int64_t jpegSize = StatFileSize(jpegFile);

	// Get optimize jpeg commandline
	std::string cmdString = JPEG_TRAN" -copy none -optimize ";
	if(jpegSize > 10240) {
		cmdString += "-progressive ";
	}
	cmdString += "\"";
	cmdString += jpegFile;
	cmdString += "\" \"";
	cmdString += tmpOutFile + "\"";
	int jpegtranRet = proc.Run(cmdString.c_str());
	if(jpegtranRet == 0) {
		RemoveFile(jpegFile);
		TsMoveFile(tmpOutFile.c_str(), jpegFile);
	}
}

void ReplaceSubString(std::string& str, const char* origSub, const char* destSub)
{
	size_t replaceLen = strlen(origSub);
	size_t replaceStartIdx = str.find(origSub);
	if(replaceStartIdx != std::string::npos) {
		str.replace(replaceStartIdx, replaceLen, destSub);
	}
}

void ReadExtraClipConfig(const char* clipFile, std::vector<int>& clipStarts, 
		std::vector<int>& clipEnds)
{
	FILE *fp = fopen(clipFile, "r");
	if (fp == NULL) return;
	char* lineBuf = (char*)malloc(36);
	while(!feof(fp)) {
		memset(lineBuf, 0, 36);
		// Read a text line (00:02:00.010|00:32:08.090)
		char* tmpBuf = fgets(lineBuf, 36, fp);
		if (tmpBuf == NULL) {
			continue;
		}
		char* startTimeStr = tmpBuf;
		// Find delimiter '|'
		char* delim = strchr(tmpBuf, '|');
		if(delim) {
			char* endTimeStr = delim + 1;
			*delim = '\0';
			float startSecs = ParsingFloatTimeString(startTimeStr);
			float endSecs = ParsingFloatTimeString(endTimeStr);
			if(endSecs > startSecs) {
				clipStarts.push_back((int)(startSecs*1000));
				clipEnds.push_back((int)(endSecs*1000));
			}
		}
	}
	free(lineBuf);
	fclose(fp);
}
