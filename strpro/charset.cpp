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
	if(tocode && !strcmp(tocode,"ANSI")) tocode = "";
	if(fromcode && !strcmp(fromcode,"ANSI")) fromcode = "";

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

	void *tmpCd = iconv_open("", "UTF-8");
	if (!tmpCd || tmpCd == (void*)-1) {
		printf("Error calling iconv_open\n");
		return NULL;
	}
	size_t insize = strlen(str)+1;
	size_t outsize = insize;
	char* buffer = (char*)malloc(outsize);
	memset(buffer, 0, outsize);
	char* out = buffer;
#ifdef WIN32
	iconv(tmpCd, (const char**)&str, &insize, &out, &outsize);
#else
	iconv(tmpCd, (char**)&str, &insize, &out, &outsize);
#endif
	iconv_close(tmpCd);
	return buffer;
}

char* CCharset::ANSItoUTF8(const char* str)
{
	if (!str || !(*str)) return NULL;

	void *tmpCd = iconv_open("UTF-8", "");
	if (!tmpCd || tmpCd == (void*)-1) {
		printf("Error calling iconv_open\n");
		return NULL;
	}
	size_t insize = strlen(str)+1;
	size_t outsize = insize * STRING_LEN_MULTIPLE;
	char* buffer = (char*)malloc(outsize);
	memset(buffer, 0, outsize);
	char* out = buffer;
#ifdef WIN32
	iconv(tmpCd, (const char**)&str, &insize, &out, &outsize);
#else
	iconv(tmpCd, (char**)&str, &insize, &out, &outsize);
#endif
	//*out = 0;
	iconv_close(tmpCd);
	
	return buffer;
}

char* CCharset::Convert(const char* str, int len)
{
	if (!str || !(*str)) return NULL;
	char* buffer = NULL;
	size_t insize = len > 0 ? len : strlen(str)+1;
	size_t outsize = insize * STRING_LEN_MULTIPLE;
	buffer = (char*)malloc(outsize);
	memset(buffer, 0, outsize);
	char* out = buffer;
#ifdef WIN32
	iconv(cd, (const char**)&str, &insize, &out, &outsize);
#else
	iconv(cd, (char**)&str, &insize, &out, &outsize);
#endif
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
		}
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
