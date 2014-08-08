#ifndef __FILE_WRITER_H
#define __FILE_WRITER_H

#include <iostream>
#include <stdio.h>
using namespace std;

#ifdef WIN32
#define fseek _fseeki64
#define ftell _ftelli64
#else
#define _FILE_OFFSET_BITS 64
#define __USE_FILE_OFFSET64
#define __USE_LARGEFILE64
#define _LARGEFILE64_SOURCE
#define fseek fseeko64
#define fell ftello64
#endif

#define BUFFER_SIZE (10485760 * 2)

class FileWriter
{
public:
	FileWriter(void);
	~FileWriter(void);

public:
	bool Open(const char *pFile);
	void Close();
	void Seek(unsigned long long llPos, unsigned int uiMode);
	unsigned long long Tell();

	bool IsOpen();
	void Write8(unsigned char ucTemp);
	void Write16(unsigned int uiTemp);
	void Write24(unsigned int uiTemp);
	void Write32(unsigned int uiTemp);
	void Write64(unsigned long long llTemp);
	void WriteBytes(const char *pBuf,unsigned int uiSize);

	void Flush();

	FILE *Pointer();
	bool Error();

	bool GetBuffer(char *pBuffer,unsigned int &uiSize);

private:
	FILE *m_pFile;
	char *m_pBuffer;
	unsigned int m_uiBufferOffset;
	bool m_bDiskError;
};

#endif