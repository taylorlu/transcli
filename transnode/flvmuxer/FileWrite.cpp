#include "FileWriter.h"

FileWriter::FileWriter(void)
{
	m_pFile = NULL;
	m_pBuffer = new char[BUFFER_SIZE];
	memset(m_pBuffer, 0, BUFFER_SIZE);
	m_uiBufferOffset = 0;
	m_bDiskError = false;
}

FileWriter::~FileWriter(void)
{
	Close();
	if (m_pBuffer)
	{
		delete[] m_pBuffer;
		m_pBuffer = NULL;
	}
}

bool FileWriter::Open(const char *pFile)
{
	Close();
	m_pFile = fopen(pFile, "wb+");
	if (!m_pFile)
	{
		return false;
	}
	return true;
}

void FileWriter::Close()
{
	Flush();
	if (m_pFile)
	{
		fclose(m_pFile);
		m_pFile = NULL;
	}
	memset(m_pBuffer, 0, BUFFER_SIZE);
	m_uiBufferOffset = 0;
	m_bDiskError = false;
}

void FileWriter::Flush()
{
	if (!m_pFile || !m_uiBufferOffset || m_bDiskError)
	{
		return;
	}
	unsigned int iWrotenBytes = fwrite(m_pBuffer, 1, m_uiBufferOffset, m_pFile);
	unsigned int iOffset = iWrotenBytes;
	int iFailCount = 0;
	while (iOffset < m_uiBufferOffset)
	{
		unsigned int iLeft = m_uiBufferOffset - iOffset;
		iWrotenBytes = fwrite(m_pBuffer + iOffset, 1, iLeft, m_pFile);
		if (iWrotenBytes <= 0)
		{
			iFailCount++;
			if (iFailCount > 2)
			{
				m_bDiskError = true;
				return;
			}
			continue;
		}
		iOffset += iWrotenBytes;
	}
	m_uiBufferOffset = 0;
}

void FileWriter::Seek(unsigned long long llPos, unsigned int uiMode)
{
	if (!m_pFile)
	{
		return;
	}
	Flush();
	if (uiMode < 0 || uiMode > 2)
	{
		uiMode = 0;
	}
	fseek(m_pFile, llPos, uiMode);
}

unsigned long long FileWriter::Tell()
{
	if (!m_pFile)
	{
		return 0;
	}
	unsigned long long ullPos = ftell(m_pFile) + m_uiBufferOffset;
	return ullPos;
}

bool FileWriter::IsOpen()
{
	if (m_pFile)
	{
		return true;
	}
	return false;
}

void FileWriter::Write8(unsigned char ucTemp)
{
	if (m_uiBufferOffset >= BUFFER_SIZE)
	{
		Flush();
	}
	m_pBuffer[m_uiBufferOffset++] = (ucTemp) & 0xff;
}

void FileWriter::Write16(unsigned int uiTemp)
{
	if (m_uiBufferOffset + 1 >= BUFFER_SIZE)
	{
		Flush();
	}
	m_pBuffer[m_uiBufferOffset++] = (uiTemp >> 8) & 0xff;
	m_pBuffer[m_uiBufferOffset++] = (uiTemp) & 0xff;
}

void FileWriter::Write24(unsigned int uiTemp)
{
	if (m_uiBufferOffset + 2 >= BUFFER_SIZE)
	{
		Flush();
	}
	m_pBuffer[m_uiBufferOffset++] = (uiTemp >> 16) & 0xff;
	m_pBuffer[m_uiBufferOffset++] = (uiTemp >> 8) & 0xff;
	m_pBuffer[m_uiBufferOffset++] = (uiTemp) & 0xff;
}

void FileWriter::Write32(unsigned int uiTemp)
{
	if (m_uiBufferOffset + 3 >= BUFFER_SIZE)
	{
		Flush();
	}
	m_pBuffer[m_uiBufferOffset++] = (uiTemp >> 24) & 0xff;
	m_pBuffer[m_uiBufferOffset++] = (uiTemp >> 16) & 0xff;
	m_pBuffer[m_uiBufferOffset++] = (uiTemp >> 8) & 0xff;
	m_pBuffer[m_uiBufferOffset++] = (uiTemp) & 0xff;
}

void FileWriter::Write64(unsigned long long llTemp)
{
	if (m_uiBufferOffset + 7 >= BUFFER_SIZE)
	{
		Flush();
	}

	m_pBuffer[m_uiBufferOffset++] = (llTemp >> 56) & 0xff;
	m_pBuffer[m_uiBufferOffset++] = (llTemp >> 48) & 0xff;
	m_pBuffer[m_uiBufferOffset++] = (llTemp >> 40) & 0xff;
	m_pBuffer[m_uiBufferOffset++] = (llTemp >> 32) & 0xff;
	m_pBuffer[m_uiBufferOffset++] = (llTemp >> 24) & 0xff;
	m_pBuffer[m_uiBufferOffset++] = (llTemp >> 16) & 0xff;
	m_pBuffer[m_uiBufferOffset++] = (llTemp >> 8) & 0xff;
	m_pBuffer[m_uiBufferOffset++] = (llTemp) & 0xff;
}

void FileWriter::WriteBytes(const char *pBuf,unsigned int uiSize)
{
	unsigned int uiBufferSize = uiSize;
	unsigned int uiBufOffset = 0;
	unsigned int uiCurrentLeft = BUFFER_SIZE - m_uiBufferOffset;
	if (uiCurrentLeft >= uiBufferSize)
	{
		memcpy(m_pBuffer + m_uiBufferOffset, pBuf, uiBufferSize);
		m_uiBufferOffset += uiBufferSize;
	}else
	{
		while (uiBufferSize)
		{
			memcpy(m_pBuffer + m_uiBufferOffset, pBuf + uiBufOffset, uiCurrentLeft);
			m_uiBufferOffset += uiCurrentLeft;
			uiBufOffset += uiCurrentLeft;
			uiBufferSize -= uiCurrentLeft;
			if (m_uiBufferOffset >= BUFFER_SIZE)
			{
				Flush();
			}
			uiCurrentLeft = BUFFER_SIZE - m_uiBufferOffset;
			if (uiCurrentLeft > uiBufferSize)
			{
				uiCurrentLeft = uiBufferSize;
			}
		}
	}
}

FILE* FileWriter::Pointer()
{
	Flush();
	return m_pFile;
}

bool FileWriter::Error()
{
	return m_bDiskError;
}

bool FileWriter::GetBuffer(char *pBuffer,unsigned int &uiSize)
{
	if (m_uiBufferOffset == 0)
	{
		return false;
	}
	memcpy(pBuffer, m_pBuffer, m_uiBufferOffset);
	uiSize = m_uiBufferOffset;
	Flush();
	return true;
}