#ifndef _FILE_OPERATION_H_
#define _FILE_OPERATION_H_

#include <string>
#include <vector>
#include <stdint.h>

#ifdef WIN32
#define INT64_FORMAT "%lld"
#define PATH_DELIMITER '\\'
// Muxers
#define MP4BOX "./tools/MP4Box.exe"
#define TSMUXER "./tools/tsMuxeR.exe"
#define ATOMICPARSLEY "./tools/AtomicParsley.exe"
#define ASFBIN "./tools/asfbin.exe"
#define FFMPEG "./codecs/ffmpeg.exe"
#define FFMPEG_MUXER "./codecs/ffmpeg.exe"
#define MKVMERGE "./tools/mkvmerge.exe"
#define FLVMID "./tools/flvmdi.exe"
#define YAMDI "./tools/yamdi.exe"
#define ICONV_EXE ".\\tools\\iconv.exe"
#define JPEG_TRAN "./tools/jpegtran.exe"

// Codecs
#define X264_BIN "./codecs/x264.exe"
#define CUDAENC_BIN "./codecs/cudaH264Enc.exe"
#define NEROAAC_BIN "./codecs/neroAacEnc.exe"
#define FAAC_BIN "./codecs/faac.exe"
#define MINDER_BIN "./codecs/mindcoder.exe"
#define CUDA_BIN "./codecs/cudaH264Enc.exe"

#define MENCODER  "./codecs/mencoder.exe"
#define MPLAYER   "./codecs/mplayer.exe"
#define AVSINPUT  "./codecs/avsinput.exe"
#define VLC_BIN   "./codecs/vlc/vlc.exe"
#define DSHOW_BIN "./codecs/dshow.exe"
#define HIK		  "./codecs/HIK/MHkiDecoder.exe"	
#define NULL_FILE "-o nul"
#define FFMPEG_NUL "NUL"
#define MP_PARSE_FORMAT "./codecs/mpparser.xml"
#define SENDMAIL_APP "./tools/sendmail.exe"
#define FFPROBE "./codecs/ffprobe.exe"

#else 
#define INT64_FORMAT "%I64d"
#define PATH_DELIMITER '/'

// Muxers
#define MP4BOX "./tools/MP4Box"
#define TSMUXER "./tools/tsMuxeR"
#define ATOMICPARSLEY "./tools/AtomicParsley"
#define ASFBIN "wine asfbin.exe"
#define FFMPEG "./codecs/ffmpeg"
#define MKVMERGE "./tools/mkvmerge"
#define ICONV_EXE "iconv"
#define JPEG_TRAN "jpegtran"
#define YAMDI "./tools/yamdi"
// Codecs
#define X264_BIN "./codecs/x264"
#define CUDAENC_BIN "./codecs/cudaH264Enc"
#define NEROAAC_BIN "./codecs/neroAacEnc"
#define FAAC_BIN "./codecs/faac"
#define MINDER_BIN "./mindcoder"
#define MENCODER  "./mencoder"
#define MPLAYER   "./mplayer"
#define AVSINPUT  "wine ./avsinput.exe"
#define VLC_BIN       "./vlc"
#define NULL_FILE "-o /dev/null"
#define FFMPEG_NUL "/dev/null"
#define MP_PARSE_FORMAT "./mpparser.xml"
#define SENDMAIL_APP "./sendmail"
#define FFPROBE "./codecs/ffprobe"
#endif

bool FileExist(const char* filePath);
bool FolderExist(const char* folderPath);
int MakeDirRecursively(const char* dirname);
std::string GetHomeDir(const char* appName);
std::string GetHomePath(const char* appName, const char* folderName = NULL);
int SafeRead(int fd, unsigned char* buffer, int bytes);
int SafeWrite(int fd, unsigned char* buffer, int bytes);
int64_t StatFileSize(const char* filename);
std::string FormatFileSize(int64_t kbytes);
std::string SecondsToTimeString(int secs, const char* connector=":");
// parsing time string, ex. 00:00:00
long ParsingTimeString(const char* strTime);
// parsing time string, ex. 00:00:00.000
float ParsingFloatTimeString(const char* strTime);

const std::vector<std::string>& GetLocalIpList();
std::string ts_escape(const std::string &str);

std::string GetStringBetweenDelims(const char* str, const char* delimStart, const char* delimEnd);
std::string GetMd5(const char *data, int len = 0);
std::string GetMd5(const std::string& str);
std::string GetMd5Part(const std::string& str, int ret_size);
std::string GetFileMd5(const char* filePath);

bool MatchFilterCase(const char *filename, const char *filters);
bool GetFileListInFolder(const char *path /*IN*/, std::vector<std::string>& outFileList /*OUT*/, 
						const char *filters = "*.*" /*IN*/, bool recursive = true /*IN*/);
char* ReadTextFile(const char* txtFile);	// Return variable need to be free by user
bool  RemoveFile(const char* filePath);
bool  TsMoveFile(const char* srcFile, const char* destFile);
bool  TsCopyFile(const char* srcFile, const char* destFile);
char *RunAndGetStdOut(const char *psz_cmd, unsigned timeout = 0);
void EnsureMultipleOfDivisor(int& num, int divisor);
void NormalizeResolution(int& w, int& h, int darNum, int darDen, int dividor=0);
bool IsMediaFormat(const char* fileName);
int GetSecretCode(int seed);
int GetRandBetween(int minVal, int maxVal);
int GetCpuCoreNum();

char * Strsep (char **stringp, const char *delim);

// Get disk free space (return kb left of the partition)
// If failed, return -1
int64_t GetFreeSpaceKb(const char* filePath);

// Return string error message of windows error. Use FreeErrorMsgBuf to free memory
void* GetLastErrorMsg();
void  FreeErrorMsgBuf(void* errMsgBuf);

// Math
bool FloatEqual(float a, float b, float epsilon = 1e-4);
bool DoubleEqual(double a, double b, double epsilon = 1e-4);

// Image optimize
void OptimizeJpeg(const char* jpegFile);

void ReplaceSubString(std::string& str, const char* origSub, const char* destSub);

// Read clip config file, the format is as following
// 00:00:00.000|00:28:22.000
// 00:28:25.000|00:35:23.000
// 00:35:30.000|00:36:06.000
void ReadExtraClipConfig(const char* clipFile, std::vector<int>& clipStarts, 
		std::vector<int>& clipEnds);
#endif

