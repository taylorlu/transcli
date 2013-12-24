#ifndef _CHARSET_H
#define _CHARSET_H

#include "StrProMacro.h"

_MC_STRPRO_BEGIN

enum charset_type_t {
	CHARSET_ANSI,
	CHARSET_UTF8,
	CHARSET_UNICODE,
	CHARSET_UNICODE_BE
};

class MC_STRPRO_EXT CCharset
{
public:
	CCharset();
	CCharset(const char* tocode, const char* fromcode = 0);
	~CCharset();
	void Set(const char* tocode, const char* fromcode = 0);
	void Unset();
	char* UTF8toANSI(const char* str);
	char* ANSItoUTF8(const char* str);
	char* Convert(const char* str, int len = 0);
	static int DetectCharset(const char* txtFileName);

private:
	void* cd;
};

_MC_STRPRO_END

#endif

