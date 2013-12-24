#ifndef __STREAM_OUTPUT_H__
#define __STREAM_OUTPUT_H__

#ifdef _WIN32
#include <io.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#endif

#include <fcntl.h>
#include <sys/stat.h>

#include "bit_osdep.h"
#include "util.h"

class CStreamOutput
{
public:
	enum {
		FILE_STREAM = 0,
		SOCKET_STREAM,
	};
	CStreamOutput();
	virtual ~CStreamOutput();

	virtual bool Open(const char* uri) = 0;
	virtual bool Open(int fd) {m_fd = fd; return m_fd >=0;}
	virtual int Write(char *p_nal, int i_size) = 0;
	virtual bool Close() = 0;

protected:
	int m_fd;
};

CStreamOutput *getStreamOutput(int outType);
void  destroyStreamOutput(CStreamOutput * outStream);

class CStreamFileOutput:public CStreamOutput
{
	FILE* m_fp;

public:
	CStreamFileOutput() {m_fp = NULL;}

	bool Open( const char* uri )
	{
		if(m_fp) return true;
		m_fp = fopen(uri, "wb");
		return m_fp ? true : false;
	}

	int Write( char *p_nal, int i_size )
	{
		return fwrite(p_nal, i_size, 1, m_fp);
	}

	bool Close()
	{
		if(m_fp) {
			fflush(m_fp);
			fclose(m_fp);
			m_fp = NULL;
		}
		return true;
	}

// 	bool Open( const char* uri )
// 	{
// #ifdef WIN32
// 		m_fd = _open(uri,  _O_CREAT | _O_BINARY | _O_WRONLY | _O_SEQUENTIAL, _S_IWRITE);
// #else
//         //ON linux, O_BINARY O_SEQUENTIAL is undefined
// 		m_fd = _open(uri,  _O_CREAT /*| _O_BINARY*/ | _O_WRONLY /*| _O_SEQUENTIAL*/, _S_IWRITE|S_IRUSR|S_IRGRP|S_IROTH);
// #endif
// 		return m_fd > 0;
// 	}
// 	int Write( char *p_nal, int i_size )
// 	{
// 		if (m_fd < 0) return -1;
// 		int offset = 0;
// 		while (offset < i_size) {
// 			int bytes = _write(m_fd, p_nal + offset, i_size - offset);
// 			if (bytes < 0) {
// 				return -1;
// 			}
// 			offset += bytes;
// 		}
// 		return offset;
// 	}
// 
// 	bool Close()
// 	{
// 		if (m_fd >= 0) {
// 			if(_close(m_fd) == 0) {
// 				m_fd = -1;
// 				return true;
// 			}
// 		}
// 		return false;
// 	}
};

/*
class CStreamFileOutput:public CStreamOutput
{
public:
	CStreamFileOutput() : m_fp(NULL) {}

	bool Open( const char* uri )
	{
		m_fp = fopen(uri, "wb");
		return m_fd != NULL;
	}
	int Write( char *p_nal, int i_size )
	{
		if (!m_fp) return -1;
		return fwrite(p_nal, 1, i_size, m_fp);
	}
	bool Close()
	{
		if(m_fp) {
			if(fclose(m_fp) == 0) {
				m_fp = NULL;
				return true;
			}
		}

		m_fp = NULL;
		return false;
	}

private:
	FILE* m_fp;
};
*/

class CStreamSockOutput:public CStreamOutput
{
public:
	bool Open( const char* uri )
	{
		//TODO
		return false;
	}

	int Write( char *p_nal, int i_size );
	

	bool Close()
	{
		m_fd = -1;
		return true;
	}
};


#endif
