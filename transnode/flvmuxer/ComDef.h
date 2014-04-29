#ifndef __COM_DEF_H
#define __COM_DEF_H

//enum eTaskStatus
//{
//	TASK_UNDEFINE,
//	TASK_CREATE,
//	TASK_READY,
//	TASK_RUNNING,
//	TASK_PAUSE,
//	TASK_FINISH,
//	TASK_ERROR
//};

#define  MAX_FRAME_SIZE  (1920*1080*3)
#define FF_MAX_PATH  4096


//�ж��ļ���·���Ƿ����
#define		CHECK_READWRITE	 0x06
#define		CHECK_WRITE	 0x02
#define		CHECK_READ	 0x04
#define		CHECK_EXCUTE	 0x01
#define		CHECK_ONLYEXIST	 0x00
#ifdef _WIN32
#include <io.h>
#define access access
#include <errno.h>
#include <direct.h>
#define mkdir  _mkdir
#else
#include <unistd.h>
#include <fcntl.h>
#define access access
#endif




//error code
enum {
	OK = 0

	,ERROR_CONTROL_ROLE_EMPTY = 100

	,ERROR_LOAD_LIBRARY_ERROR = 1000
	,ERROR_INIT_PREARATION_ERROR
	,ERROR_EXCUTE_PREARATION_ERROR
	,ERROR_SAVE_ROLE_LIST
	,ERROR_SAVE_CONFIG

	,ERROR_HAS_RUNNING_TASK
	,ERROR_TASK_NOT_EXIST
	,ERROR_PAUSE_TASK_FAILED
	,ERROR_STOP_TASK_FAILED
	,ERROR_DEL_TASK_FAILED

	,ERROR_CONVERT_EMPTY_STRUCT = 1200
	,ERROR_CONVERT_MAPPING_PARAM

	,ERROR_CONVERT_AUDIO_ADJUST_VOLUME
	,ERROR_CONVERT_AUDIO_NO_ENCODER
	,ERROR_CONVERT_AUDIO_NO_WAVFILE
	,ERROR_CONVERT_AUDIO_CREATE_PROCESS
	,ERROR_CONVERT_AUDIO_ERROR

	,ERROR_CONVERT_NO_VIDEO_FILE
	,ERROR_CONVERT_NO_VIDEO_CFG
	,ERROR_DECODE_PTS_LESS_THEN_PREFRAME
	,ERROR_CONVERT_NO_VIDEO_ENCODER

	,ERROR_CONVERT_MIX_FAIL

};

//��Ƶ����״̬
enum {
	AUDIO_ENCODING
	,AUDIO_ENCODE_SUCCEED
	,AUDIO_ENCODE_FAILED
};

//����ļ���ʽ����
enum OUTPUT_TYPE{
	OUTPUT_TYPE_MKV,
	OUTPUT_TYPE_MP4,
	OUTPUT_TYPE_FLV
};

//��������
enum {
	TASK_TYPE_UNDEFINED,
	TASK_TYPE_CONVERT,
	TASK_TYPE_UPLOAD,
	TASK_TYPE_UPLOAD_FILE
};

//��ȡת������ִ�г����·��
#ifdef WIN32
#define APP_PATH(out_buffer, buffer_length) { \
	char logPath[1024]; \
	::GetModuleFileNameA(NULL,(LPCH)logPath, 1024); \
	std::string logpath(logPath); \
	logpath = logpath.substr(0,logpath.find_last_of("\\")); \
	if (buffer_length > 1 + logpath.length()) {  \
	memcpy((void*)out_buffer, logpath.c_str(), 1 + logpath.length()); \
	}}

#else
#define APP_PATH(out_buffer, buffer_length) { \
	char logPath[1024]; \
	readlink("/proc/self/exe",   logPath,   sizeof(logPath)); \
	std::string logpath(logPath); \
	logpath = logpath.substr(0,logpath.find_last_of("/")); \
	if (buffer_length > 1 + logpath.length()) {  \
	memcpy((void*)out_buffer, logpath.c_str(), 1 + logpath.length()); \
	}}
#endif


//�ж�Ŀ¼�Ƿ���ڣ��������ھʹ���Ŀ¼
bool  CheckCreateFolder(const char *folder);

//��ȡת��洢xml��·��
bool  GetConvertStorePath(char *buf, int length);

//��ǰϵͳʣ��ռ����ķ�ϵͳ���̷�������ֵ�Ǵ�д��ϵͳ���̷��� D��E��F
char  GetTheLargestDisksSymbol();

//���Ĭ������
float GetDefaultBitrate( float fOriginBitrate, int iOutWidth, int iOutHeight );

#endif