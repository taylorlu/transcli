#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef WIN32
#include "iconv.h"
#else
#include <iconv.h>
#endif
#include "charset.h"

#define STRING_LEN_MULTIPLE 4
#define DEFAULT_SYS_CODE "gb2312"

_MC_STRPRO_BEGIN
CCharset::CCharset():cd(0)
{
}

CCharset::CCharset(const char* tocode, const char* fromcode):cd(0)
{
	Set(tocode, fromcode);
}

CCharset::~CCharset()
{
	if (cd) {
		iconv_close(cd);
		cd = 0;
	}
}

void CCharset::Set(const char* tocode, const char* fromcode)
{
	Unset();
	if(tocode && !strcmp(tocode,"ANSI")) tocode = DEFAULT_SYS_CODE;
	if(fromcode && !strcmp(fromcode,"ANSI")) fromcode = DEFAULT_SYS_CODE;

	if (tocode || fromcode)
		cd = iconv_open(tocode ? tocode : "", fromcode ? fromcode : "");
}

void CCharset::Unset()
{
	if (cd) {
		iconv_close(cd);
		cd = 0;
	}
}

char* CCharset::UTF8toANSI(const char* str)
{
	if (!str || !(*str)) return NULL;

	void *tmpCd = iconv_open(DEFAULT_SYS_CODE, "UTF-8");
	if (!tmpCd || tmpCd == (void*)-1) {
		printf("Error calling iconv_open\n");
		return NULL;
	}
	size_t insize = strlen(str);
	size_t outsize = insize + 1;
	char* buffer = (char*)malloc(outsize);
	memset(buffer, 0, outsize);
	char* out = buffer;
    iconv(tmpCd, &str, &insize, &out, &outsize);
	iconv_close(tmpCd);
	return buffer;
}

char* CCharset::ANSItoUTF8(const char* str)
{
	if (!str || !(*str)) return NULL;

	void *tmpCd = iconv_open("UTF-8", DEFAULT_SYS_CODE);
	if (!tmpCd || tmpCd == (void*)-1) {
		printf("Error calling iconv_open\n");
		return NULL;
	}
	size_t insize = strlen(str);
	size_t outsize = insize * STRING_LEN_MULTIPLE + 1;
	char* buffer = (char*)malloc(outsize);
	memset(buffer, 0, outsize);
	char* out = buffer;
    iconv(tmpCd, &str, &insize, &out, &outsize);
	//*out = 0;
	iconv_close(tmpCd);
	
	return buffer;
}

char* CCharset::Convert(const char* str, int len)
{
	if (!str || !(*str)) return NULL;
	char* buffer = NULL;
	size_t insize = len > 0 ? len : strlen(str);
	size_t outsize = insize * STRING_LEN_MULTIPLE + 1;
	buffer = (char*)malloc(outsize);
	memset(buffer, 0, outsize);
	char* out = buffer;
    iconv(cd, &str, &insize, &out, &outsize);
	return buffer;
}

const char* CCharset::DetectCharset(const char* txtFileName)
{
	const char* defualtCharset = "ANSI";
	do {
		if(!txtFileName) break;
		FILE* fp = fopen(txtFileName, "rb");
		if(!fp) break;
		size_t bufSize = 3;
		unsigned char* detectBuf = (unsigned char*)malloc(bufSize);
		fread(detectBuf, 1, bufSize, fp);
		if(detectBuf[0] == strtoul("ff", NULL, 16) &&
			detectBuf[1] == strtoul("fe", NULL, 16) ) {
			defualtCharset = "UTF-16LE";
		} else if(detectBuf[0] == strtoul("fe", NULL, 16) &&
			detectBuf[1] == strtoul("ff", NULL, 16) ) {
			defualtCharset = "UTF-16BE";
		} else if(detectBuf[0] == strtoul("ef", NULL, 16) &&
			detectBuf[1] == strtoul("bb", NULL, 16) &&
			detectBuf[2] == strtoul("bf", NULL, 16)) {
			defualtCharset = "UTF-8";
		} else {	// No bom text
			//1.===== If a string doesn't contain nulls, its UTF-8
			// :
			//else
			// :
			//2:===== If a string doesn't contain double nulls, it's UTF-16
			// :--.
			// : 3:== If the nulls are on odd numbered indices, it's UTF-16LE
			// :  :
			// : else
			// :  :
			// : 4'== The string defaults to UTF-16BE
			// :
			//else
			// :
			//5:===== If the index modulo 4 is 0 and the character is greater than
			// :      0x7F, the string is UTF-32LE. This is because the range of
			// :      UTF-32 only goes up to 0x7FFFFFFF, meaning approximately 22%
			// :      of the characters that can be represented will validate that
			// :      the string is not big endian; including a BOM.
			// :
			//else
			// :
			//6'===== The string defaults to UTF-32BE
			// Result: 0  = UTF-8, 1  = UTF-16BE, 2  = UTF-16LE, 3  = UTF-32BE, 4  = UTF-32LE

			// the first rule is that we must terminate all strings with a quadruple null, regardless of encoding
			fseek(fp, 0L, SEEK_END);
			long bufLen = ftell(fp);
			fseek(fp, 0L, SEEK_SET);
			char* buf = (char*)malloc(bufLen+3);
			memset(buf, 0, bufLen+3);
			fread(buf, 1, bufLen, fp);

			unsigned c, i = 0, flags = 0;
			while (buf[i] | buf[i + 1] | buf[i + 2] | buf[i + 3])
			  flags = (c = buf[i++]) ? flags | ((!(flags % 4) && 
			  c > 0x7F) << 3) : flags | 1 | (!(i & 1) << 1) 
			  | ((buf[i] == 0) << 2);
			int dectectRet = (flags & 1) + ((flags & 2) != 0) + ((flags & 4) != 0) + ((flags & 8) != 0);
			switch(dectectRet) {
			case 0: defualtCharset = "UTF-8"; break;
			case 1: defualtCharset = "UTF-16BE"; break;
			case 2: defualtCharset = "UTF-16LE"; break;
			case 3: defualtCharset = "UTF-32BE"; break;
			case 4: defualtCharset = "UTF-32LE"; break;
			}
		    free(buf);
		}
		free(detectBuf);
		fclose(fp);
	} while(false);

	return defualtCharset;
}

void CCharset::ConvertFileEncode(const char* srcFile, const char* dstFile, const char* dstCode)
{
	if(!srcFile || !dstFile || !(*srcFile) || !(*dstFile)) return;
	
	// Detect srcfile encoding
	const char* srcCode = DetectCharset(srcFile);
	// Open srcfile and read all the content
	FILE* srcFp = fopen(srcFile, "rb");
	if(!srcFp) return;
	fseek(srcFp, 0L, SEEK_END);
	long bufLen = ftell(srcFp);
	fseek(srcFp, 0L, SEEK_SET);
	char* srcText = (char*)malloc(bufLen);
	if(!srcText) return;
	memset(srcText, 0, bufLen);
	size_t readLen = fread(srcText, 1, bufLen, srcFp);
	fclose(srcFp);

	// No need to convert, just copy it
	if(srcCode && dstCode && !strcmp(srcCode, dstCode)) {
		FILE* copyFp = fopen(dstFile, "wb");
		if(copyFp) {
			fwrite(srcText, 1, readLen, copyFp);
			fclose(copyFp);
		}
		free(srcText);
		return;
	}

	Set(dstCode, srcCode);
	if(!cd) {
		free(srcText);
		return;
	}

	// Convert file encoding
	size_t outsize = readLen * STRING_LEN_MULTIPLE;
	char* dstText = (char*)malloc(outsize);
	memset(dstText, 0, outsize);
	char* outBuf = dstText;
	char* inBuf = srcText;
	iconv(cd, (const char**)&inBuf, &readLen, &outBuf, &outsize);
	FILE* dstFp = fopen(dstFile, "wb");
	if(dstFp) {
		fwrite(dstText, 1, strlen(dstText), dstFp);
		fclose(dstFp);
	}

	free(srcText);
	free(dstText);
}

_MC_STRPRO_END
