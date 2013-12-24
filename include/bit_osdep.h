#ifndef __OS_DEP_INCLUDE_H__
#define __OS_DEP_INCLUDE_H__

//////////////////////////////////////////////////////////////////
//common part
#ifdef _DEBUG
#define DD printf("%s, %s(%d)\n", __FILE__,__FUNCTION__ , __LINE__)
#else
//#define DD FIXME
#define DD 
#endif

bool FileIsLocked(const char *path);
bool TerminateProcess(int pid);
int GetAppDir(char *dir, int size);


//////////////////////////////////////////////////////////////////
#if defined(WIN32) //win32
#include <windows.h>
#include <io.h>

__int64 atoll(const char *nptr);
#define ts_fopen(x,y) fopen((x), (y))
#define ts_open _open
#define ts_stat stat
#define ts_lstat lstat
#define rmdir(x) _rmdir(x)

#else //Linux or Mac osx

#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#define _open open
#define _lseek(x,y,z) lseek((x),(y),(z))
#define _write(x,y,z) write((x),(y),(z))
#define _read(x,y,z) read((x),(y),(z))
#define _close(x) close(x)

#define _O_CREAT O_CREAT
#define _O_WRONLY O_WRONLY
#define _S_IWRITE S_IWRITE
#define _S_IREAD S_IREAD
#define _O_RDONLY O_RDONLY 
#define _O_BINARY 0
#define _O_SEQUENTIAL 0

#define strtok_s strtok_r
#define _strdup strdup
#define sprintf_s snprintf
#define _itoa itoa
#define _stricmp strcasecmp
#define _snprintf snprintf
#define _abs64 llabs
//DWORD on x86_64 ?
#define __stdcall
#ifndef MAX_PATH
	#define MAX_PATH 260
#endif

#define HWND long
#define HANDLE long
#define HINSTANCE long

#define IS_READ_LOCK(filename, offset, whence, len) \
	lock_test(filename, F_RDLCK, offset, whence, len)
#define IS_WRITE_LOCK(filename, offset, whence, len) \
	lock_test(filename, F_WRLCK, offset, whence, len)

#ifndef TRUE
	#define TRUE (1==1)
#endif
#ifndef FALSE
	#define FALSE (1==0)
#endif

typedef int BOOL;
typedef int64_t __int64;

//min
//#define min(a,b) ((a)<(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))
//max
//#define max(a,b) ((a)>(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

typedef unsigned int DWORD;
typedef DWORD* LPWORD;
typedef void * HMODULE;

#define WINAPI  

//////////////////////////////////////////////////////////////////////////////////////
void Sleep(int ms);
void reverse(char *str);
int fopen_s(FILE **fpp, const char *filename, const char *mode);
int strcpy_s(char *dst, size_t n, const char *src);
int strncpy_s(char *dst, size_t dst_n, const char *src, size_t src_n);
char* itoa( int value, char* result, int base );
char* _itoa_s( int value, char* result, int len, int base );

int GetModuleFileName( char* sModuleName, char* sFileName, int nSize);

bool isFolderExist(const char *sptr);
bool isFileExist(const char *sptr);
int ShellExe(char *cmd);

#include <dlfcn.h>
HMODULE LoadLibrary(const char *filename);
void* GetProcAddress(HMODULE handle, const char *symbol);
int FreeLibrary(HMODULE handle);

#include<sys/time.h>
#include<unistd.h>
int64_t GetTickCount();		//return ms
BOOL MoveFile(const char * psz_src, const char *psz_dst);
BOOL DeleteFile(const char * psz_pathname);
int _mkdir(const char *psz_pathname);
BOOL CopyFile(const char *psz_src, const char *psz_dst, BOOL bFailIfExists);
BOOL CopyFile(const char *psz_src, const char *psz_dst);

FILE *ts_fopen (const char *filename, const char *mode);
int ts_stat( const char *filename, struct stat *buf);
int ts_lstat( const char *filename, struct stat *buf);

BOOL SetCurrentDirectory(const char*psz_path);

pid_t  lock_test(const char* filename, int type, off_t offset, int whence, off_t len);

#endif

#endif

