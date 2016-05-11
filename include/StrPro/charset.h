#ifndef _CHARSET_H
#define _CHARSET_H

#include "StrProMacro.h"

_MC_STRPRO_BEGIN

class MC_STRPRO_EXT CCharset
{
public:
	CCharset();
	// Encoding code("ANSI"/"UTF-8"/"UTF-16LE"/"UTF-16BE")
	CCharset(const char* tocode, const char* fromcode = 0);
	~CCharset();
	void Set(const char* tocode, const char* fromcode = 0);
	void Unset();
	char* UTF8toANSI(const char* str);
	char* ANSItoUTF8(const char* str);
	char* Convert(const char* str, int len = 0);

	// return value("ANSI"/"UTF-8"/"UTF-16LE"/"UTF-16BE")
	static const char* DetectCharset(const char* txtFileName);

	// from code will be dectected from srcfile
	void ConvertFileEncode(const char* srcFile, const char* dstFile, const char* dstCode);
private:
	void* cd;
};

_MC_STRPRO_END

#endif

