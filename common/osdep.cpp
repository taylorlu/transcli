#include "bit_osdep.h"

#ifdef WIN32
__int64 atoll(const char *nptr)
{
	int flag = 1;
	__int64 ret = 0;

	if (nptr == NULL || *nptr == '\0') return 0;

	while (*nptr == ' ') nptr++;
	if (*nptr == '-') {
		flag = -1;
		nptr++;
	}
	while (*nptr && *nptr >= '0' && *nptr <= '9') {
		ret = ret * 10 + (*nptr - '0');
		nptr++;
	}
	return flag * ret;
}

__int64 GetDirTime(const char *path)
{ 
	HANDLE hDir = CreateFileA(path, GENERIC_READ, 
							FILE_SHARE_READ|FILE_SHARE_DELETE, 
							NULL, OPEN_EXISTING,
							FILE_FLAG_BACKUP_SEMANTICS, NULL); 
//	FILETIME lpCreationTime; 
//	FILETIME lpLastAccessTime;
	FILETIME lpLastWriteTime;

	if ( hDir == INVALID_HANDLE_VALUE) return 0;

	if (!GetFileTime(hDir, NULL, NULL, &lpLastWriteTime)) {
		CloseHandle(hDir);
		return 0;
	}

	CloseHandle(hDir);

	return (__int64)lpLastWriteTime.dwHighDateTime << 32 | (__int64)lpLastWriteTime.dwLowDateTime; 
}

// To open a file in exclusive sharing mode, 
// you must set the dwShareMode parameter of CreateFile to 0
bool FileIsLocked(const char *path)
{
	HANDLE hFile = CreateFileA(
		path,
		GENERIC_READ/* | GENERIC_WRITE*/,
		0, // The object cannot be shared 
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if ( hFile == INVALID_HANDLE_VALUE) {
		//Locked, or not exist
		return true;
	}

	CloseHandle(hFile);
	return false;
}

int GetAppDir(char *dir, int size)
{
    if (dir == NULL || size <= 0) return 0;

	if (GetModuleFileName(GetModuleHandle(NULL), dir, size) == 0) return 0;

	char* p = strrchr(dir, '\\');
	if (p) *p = 0;

    return strlen(dir);
}

#else

#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <langinfo.h>
#include <iconv.h>
#include <assert.h>
#include <stdarg.h>

bool FileIsLocked(const char *path)
{
#if 1
    char cmd[512];

    if (path == NULL) return false;

    snprintf(cmd, 512, "lsof \"%s\" 2>/dev/null| tail -n1 | awk '{print $4}' | egrep 'w$'", path);
    
    return system(cmd)==0?true:false;
#else
    return false;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////
void Sleep(int ms)
{
    if (ms > 0) usleep(1000*ms); 
}

void reverse(char *str)
{
    char *p;

    if (str == NULL || *str == '\0') return;

    p = str + strlen(str) - 1;
    while (str < p) {
        char t = *str;
        *str = *p;
        *p = t;
        str++; p--;
    }
}

static char charset[sizeof ("CSISO11SWEDISHFORNAMES")] = "";

static void find_charset_once (void)
{
    strncpy (charset, nl_langinfo (CODESET), sizeof (charset));
    if (!strcasecmp (charset, "ASCII")
     || !strcasecmp (charset, "ANSI_X3.4-1968"))
        strcpy (charset, "UTF-8"); /* superset... */
}

static int find_charset (void)
{
    if (*charset == '\0') find_charset_once();

    return !strcasecmp (charset, "UTF-8");
}
/**
 * ToLocale: converts an UTF-8 string to local system encoding.
 *
 * @param utf8 nul-terminated string to be converted
 *
 * @return a nul-terminated string, or NULL in case of error.
 * To avoid memory leak, you have to pass the result to LocaleFree()
 * when it is no longer needed.
 */

char *ToLocale (const char *string /*utf8*/)
{
    if( string == NULL )
        return NULL;

    if (find_charset ())
        return (char *)string;

    iconv_t hd = iconv_open ( charset, "UTF-8");
    if (hd == (iconv_t)(-1))
        return NULL;

    const char *iptr = string;
    size_t inb = strlen (string);
    size_t outb = inb * 6 + 1;
    char output[outb], *optr = output;

    while (iconv (hd, (char**)&iptr, &inb, &optr, &outb) == (size_t)(-1))
    {
        *optr++ = '?';
        outb--;
        iptr++;
        inb--;
        iconv (hd, NULL, NULL, NULL, NULL); /* reset */
    }
    *optr = '\0';
    iconv_close (hd);

    assert (inb == 0);
    assert (*iptr == '\0');
    assert (*optr == '\0');
    assert (strlen (output) == (size_t)(optr - output));
    return strdup (output);
}

void LocaleFree(const char* local_name)
{
    if (!find_charset ())
        free ((char *)local_name);
}

/**
 * Opens a system file handle.
 *
 * @param filename file path to open (with UTF-8 encoding)
 * @param flags open() flags, see the C library open() documentation
 * @return a file handle on success, -1 on error (see errno).
 * @note Contrary to standard open(), this function returns file handles
 * with the close-on-exec flag enabled.
 */
int ts_open (const char *filename, int flags, ...)
{
    unsigned int mode = 0;
    va_list ap;

    va_start (ap, flags);
    if (flags & O_CREAT)
        mode = va_arg (ap, unsigned int);
    va_end (ap);

#ifdef O_CLOEXEC
    flags |= O_CLOEXEC;
#endif

    const char *local_name = ToLocale (filename);

    if (local_name == NULL)
    {
        errno = ENOENT;
        return -1;
    }

    int fd = open (local_name, flags, mode);
    if (fd != -1)
        fcntl (fd, F_SETFD, FD_CLOEXEC);

    LocaleFree(local_name);

    return fd;
}

/**
 * Opens a FILE pointer.
 * @param filename file path, using UTF-8 encoding
 * @param mode fopen file open mode
 * @return NULL on error, an open FILE pointer on success.
 */
FILE *ts_fopen (const char *filename, const char *mode)
{

    int rwflags = 0, oflags = 0;
    bool append = false;

    for (const char *ptr = mode; *ptr; ptr++)
    {
        switch (*ptr)
        {
            case 'r':
                rwflags = O_RDONLY;
                break;

            case 'a':
                rwflags = O_WRONLY;
                oflags |= O_CREAT;
                append = true;
                break;

            case 'w':
                rwflags = O_WRONLY;
                oflags |= O_CREAT | O_TRUNC;
                break;

            case '+':
                rwflags = O_RDWR;
                break;

#ifdef O_TEXT
            case 't':
                oflags |= O_TEXT;
                break;
#endif
        }
    }

    int fd = ts_open (filename, rwflags | oflags, 0666);
    if (fd == -1)
        return NULL;

    if (append && (lseek (fd, 0, SEEK_END) == -1))
    {
        close (fd);
        return NULL;
    }

    FILE *stream = fdopen (fd, mode);
    if (stream == NULL)
        close (fd);

    return stream;
}

int fopen_s(FILE **fpp, const char *filename, const char *mode)
{
    *fpp = ts_fopen(filename, mode);
    if (*fpp) return 0;
    else return -1;
}

int strcpy_s(char *dst, size_t n, const char *src)
{
    char *ret = strncpy(dst, src, n);
    if (ret) return 0;
    else return -1;
}

int strncpy_s(char *dst, size_t dst_n, const char *src, size_t src_n)
{
    size_t n = src_n > dst_n ? dst_n : src_n;

    char *ret = strncpy(dst, src, n);
    if (ret) return 0;
    else return -1;
}

char* itoa( int value, char* result, int base )
{
	
    // check that the base if valid
	if (base < 2 || base > 16) { *result = 0; return result; }
	
	char* out = result;
	int quotient = value;
	
	do {
	
		*out = "0123456789abcdef"[ abs( quotient % base ) ];
		++out;
		quotient /= base;
	} while ( quotient );
	
	
	// Only apply negative sign for base 10
	if ( value < 0 && base == 10) *out++ = '-';
	
	reverse( result );
	*out = 0;
	return result;
}

char* _itoa_s( int value, char* result, int len, int base )
{
	
    // check that the base if valid
	if (base < 2 || base > 16) { *result = 0; return result; }
	
	char* out = result;
	int quotient = value;
	
	do {
        if (out >= result + len) return NULL;
		*out = "0123456789abcdef"[ abs( quotient % base ) ];
		++out;
		quotient /= base;
	} while ( quotient );
	
	// Only apply negative sign for base 10
	if ( value < 0 && base == 10) *out++ = '-';
	
	reverse( result );
	*out = 0;
	return result;
}

static int ts_statEx( const char *filename, struct stat *buf,
                        bool deref )
{
    const char *local_name = ToLocale( filename );

    if( local_name != NULL )
    {
        int res = deref ? stat( local_name, buf )
                       : lstat( local_name, buf );
        LocaleFree( local_name );
        return res;
    }

    errno = ENOENT;
    return -1;
}

/**
 * Finds file/inode informations, as stat().
 * Consider using fstat() instead, if possible.
 *
 * @param filename UTF-8 file path
 */
int ts_stat( const char *filename, struct stat *buf)
{
    return ts_statEx( filename, buf, true );
}

/**
 * Finds file/inode informations, as lstat().
 * Consider using fstat() instead, if possible.
 *
 * @param filename UTF-8 file path
 */
int ts_lstat( const char *filename, struct stat *buf)
{
    return ts_statEx( filename, buf, false );
}
#if 0
bool isFolderExist(const char *sptr)
{
	struct stat sb;
	
	if (stat(sptr, &sb) != 0) return false;

	return (sb.st_mode & S_IFDIR) != 0;
}

bool isFileExist(const char *sptr)
{
	struct stat stat_ret;
	if (stat(sptr, &stat_ret) != 0) return false;

	return (stat_ret.st_mode & S_IFREG) != 0;
}
#endif

int ShellExe(char *cmd)
{
    pid_t pid;
    pid=fork();
    if (pid == 0)
    {
        if (system(cmd)<0) return -1;
        else return 1;
    }
    else if (pid > 0) return 2;
    else return 0;
}

#include <dlfcn.h>
HMODULE LoadLibrary(const char *filename)
{
	return dlopen(filename, RTLD_LAZY);
}

void* GetProcAddress(HMODULE handle, const char *symbol)
{
	return dlsym(handle, symbol);
}

int FreeLibrary(HMODULE handle)
{
	return dlclose(handle);
}

#include<sys/time.h>
#include<unistd.h>

int64_t GetTickCount()
{
	struct timeval tv;
	int64_t ret;

	if (gettimeofday(&tv, NULL) != 0) return 0;

	ret = (int64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
	return ret;
}

BOOL MoveFile(const char * psz_src, const char *psz_dst)
{
	char psz_cmd[MAX_PATH*2];

	if (psz_src == NULL || *psz_src == '\0' || 
		psz_dst == NULL || *psz_dst == '\0') return FALSE;

	snprintf(psz_cmd, sizeof(psz_cmd), "mv \"%s\" \"%s\"", psz_src, psz_dst);

	return (system(psz_cmd) == 0)? TRUE:FALSE;
}

BOOL DeleteFile(const char * psz_pathname)
{
#if 1
	if (psz_pathname == NULL || *psz_pathname == '\0') return FALSE;
//or unlink()
	return (remove(psz_pathname) == 0)? TRUE:FALSE;
#else
	return TRUE;
#endif
}

int _mkdir(const char *psz_pathname)
{
	if (psz_pathname == NULL || *psz_pathname == '\0') return -1; 

#if 1
	char cmd[2*MAX_PATH];
	snprintf(cmd, sizeof(cmd), "mkdir -p \"%s\"", psz_pathname);
	return system(cmd);
#else
	return mkdir(psz_pathname, 755);
#endif
}

BOOL CopyFile(const char *psz_src, const char *psz_dst, BOOL bFailIfExists)
{
    struct stat st;
	char psz_cmd[MAX_PATH*2];

	if (psz_src == NULL || *psz_src == '\0' || 
		psz_dst == NULL || *psz_dst == '\0') return FALSE;

    if ( bFailIfExists && ts_stat(psz_dst, &st) == 0) return false;

	snprintf(psz_cmd, sizeof(psz_cmd), "cp \"%s\" \"%s\"", psz_src, psz_dst);

	return (system(psz_cmd) == 0)? TRUE:FALSE;
}

BOOL CopyFile(const char *psz_src, const char *psz_dst)
{
    return CopyFile(psz_src, psz_dst, FALSE);
}

BOOL SetCurrentDirectory(const char*psz_path)
{
    if (psz_path == NULL || *psz_path == '\0') return FALSE;

    return (chdir(psz_path) == 0) ? TRUE:FALSE;
}

pid_t  lock_test(const char* filename, int type, off_t offset, int whence, off_t len)  
{  
	struct   flock   lock;  
	lock.l_type   =   type;  
	lock.l_start   =   offset;  
	lock.l_whence   =   whence;  
	lock.l_len   =   len;  

	int fd = open(filename, _O_WRONLY | O_APPEND);
	if(fcntl(fd,   F_GETLK,   &lock)   <   0)  {
		printf("Use fcntl to test file lock failed.\n");
		if(fd > 0) close(fd);
		return 1;
	}

	if(lock.l_type   ==   F_UNLCK)  {
		if(fd > 0) close(fd);
		return   0;  
	}
	if(fd > 0) close(fd);
	return   (lock.l_pid);  
}   

int daemonize(const char * stdoutfile, const char * stderrfile, const char * stdinfile, const char * pid_file_name)
{
    pid_t pid;
    FILE *fp;
    int si, so, se;

    pid = fork();
    if (pid < 0) return -1;

    /* If we got a good PID, then
       we can exit the parent process. */
    if (pid > 0) {
        /* first parent exit*/
        exit(EXIT_SUCCESS);
    }

    /* Change the file mode mask */
    umask(0);
    /* Create a new SID for the child process */
    setsid();

    chdir("/");

    /* do second fork*/
    pid = fork();
    if (pid < 0) return -1;

    if (pid > 0) {
        /* exit from second parent */
        exit(EXIT_SUCCESS);
    }

#if 0
    if (stdinfile == NULL) stdinfile = "/dev/null";
    if (stdoutfile == NULL) stdoutfile = "/dev/null";
    if (stderrfile == NULL) stderrfile = "/dev/null";

    si = open(stdinfile, O_RDONLY);
    so = open(stdoutfile, O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);
    se = open(stderrfile, O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);

    dup2(si, STDIN_FILENO);
    dup2(so, STDOUT_FILENO);
    dup2(se, STDERR_FILENO);
#endif

    if (pid_file_name != NULL && *pid_file_name != '\0') {
        char pid_path[128];
        sprintf(pid_path, "/var/run/%s", pid_file_name);
        /*save pid file*/
        fp = fopen(pid_path, "w");
        if (fp != NULL) {
            fprintf(fp, "%d\n", getpid());
            fclose(fp);
        }
    }

    return 0;
}

#if defined(__LINUX__)
int GetModuleFileName( char* sModuleName, char* sFileName, int nSize)
{
    char sLine[1024] = { 0 };
    void* pSymbol = (void*)"";
    FILE *fp;
    char *pPath;
    
    fp = fopen ("/proc/self/maps", "r");
    if ( fp != NULL ) {
        while (!feof (fp)) {
            unsigned long start, end;
            char *tmp;
            size_t len, m_len;
            
            if ( !fgets (sLine, sizeof (sLine), fp)) continue;
            if ( !strstr (sLine, " r-xp ") || !strchr (sLine, '/')) continue;
            
            /* Get rid of the newline */
            tmp = strrchr (sLine, '\n');
            if (tmp) *tmp = 0;

            if (sModuleName && *sModuleName) {
                len = strlen(sLine);
                m_len = strlen(sModuleName);
#if 0
                if (m_len > len || strcmp(sLine + len - m_len, sModuleName)) continue;
#else
                if (strstr(sLine, sModuleName) == NULL) continue;
#endif
            }
            else {
                sscanf (sLine, "%lx-%lx ", &start, &end);
                if (pSymbol < (void *) start || pSymbol >= (void *) end) continue;
            }
                
            /* Extract the filename; it is always an absolute path */
            pPath = strchr (sLine, '/');

            /* Get rid of "(deleted)" */
                
            len = strlen (pPath);
            if (len > 10 && strcmp (pPath + len - 10, " (deleted)") == 0) {
                tmp = pPath + len - 10;
                *tmp = 0;
            }

            strncpy( sFileName, pPath, nSize );
            fclose (fp);
            return strlen(sFileName);
        }

        fclose (fp);
    }
    
    return 0; /*failed*/
}

int GetAppDir(char *dir, int size)
{
	if (dir == NULL || size <= 0) return 0;

	if (GetModuleFileName(NULL, dir, size) == 0) return 0;

	char* p = strrchr(dir, '/');
	if (p) *p = 0;

	return strlen(dir);
}

int GetCpuCoreNum()
{
	return sysconf( _SC_NPROCESSORS_ONLN );
}

#elif defined(__APPLE__)

#include <CoreFoundation/CoreFoundation.h>
int GetAppDir(char *dir, int size)
{
	CFBundleRef mainBundle = CFBundleGetMainBundle();
	if(!mainBundle)
		return -1;

	CFURLRef mainBundleURL = CFBundleCopyBundleURL(mainBundle);
	if(!mainBundleURL)
		return -1;

	CFStringRef cfStringRef = CFURLCopyFileSystemPath(mainBundleURL, kCFURLPOSIXPathStyle);
	if(!cfStringRef)
		return -1;

	*dir = '\0';
	CFStringGetCString(cfStringRef, dir, size, kCFStringEncodingASCII);

	CFRelease(mainBundleURL);
	CFRelease(cfStringRef);

	return strlen(dir);
}

int GetCpuCoreNum()
{
	int mib[4];
	int numCPU = 0;

	size_t len = sizeof(numCPU); 

	/* set the mib for hw.ncpu */
	mib[0] = CTL_HW;
	mib[1] = HW_AVAILCPU;  // alternatively, try HW_NCPU;

	/* get the number of CPUs from the system */
	sysctl(mib, 2, &numCPU, &len, NULL, 0);

	if( numCPU < 1 ) 
	{
		 mib[1] = HW_NCPU;
		 sysctl( mib, 2, &numCPU, &len, NULL, 0 );

		 if( numCPU < 1 )
		 {
			  numCPU = 1;
		 }
	}
	return numCPU;
}

#endif

#endif

