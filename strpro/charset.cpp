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

bool isUtf8WithoutBom(unsigned char* buf, long len)
{
	int rv = 1;
	int ASCII7only = 1;
	unsigned char *sx        = buf;
	unsigned char *endx      = sx + len;
 
	while (sx < endx) {
		if (!*sx) {                                                                                        // For detection, we'll say that NUL means not UTF8
			ASCII7only = 0;
			rv = 0;
			break;
		} else if (*sx < 0x80) {            // 0nnnnnnn If the byte's first hex code begins with 0-7, it is an ASCII character.
		    sx++;
		} else if (*sx < (0x80 + 0x40)) {   // 10nnnnnn 8 through B cannot be first hex codes
			ASCII7only=0;
			rv=0;
			break;
		} else if (*sx < (0x80 + 0x40 + 0x20)) {  // 110xxxvv 10nnnnnn  If it begins with C or D, it is an 11 bit character
			ASCII7only=0;
			if (sx >= endx-1)
				break;
			if (!(*sx & 0x1F) || (sx[1]&(0x80+0x40)) != 0x80) {
				rv=0; break;
			}
			sx+=2;
		} else if (*sx < (0x80 + 0x40 + 0x20 + 0x10)) {  // 1110qqqq 10xxxxvv 10nnnnnn If it begins with E, it is 16 bit
			ASCII7only=0;
			if (sx>=endx-2)
				break;
			if (!(*sx & 0xF) || (sx[1]&(0x80+0x40)) != 0x80 || (sx[2]&(0x80+0x40)) != 0x80) {
				rv=0; break;
			}
			sx+=3;
		} else {         // more than 16 bits are not allowed here
			ASCII7only=0;
			rv=0;
			break;
		}
	}
	
	if (rv)	// Utf8 without bom
		return true;
	return false;
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
			// the first rule is that we must terminate all strings with a quadruple null, regardless of encoding
			fseek(fp, 0L, SEEK_END);
			long bufLen = ftell(fp);
			fseek(fp, 0L, SEEK_SET);
			unsigned char* buf = (unsigned char*)malloc(bufLen+1);
			memset(buf, 0, bufLen+1);
			fread(buf, 1, bufLen, fp);
			if(isUtf8WithoutBom(buf, bufLen)) {
				defualtCharset = "UTF-8";
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
