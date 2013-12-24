#ifndef _DECODER_COPY_H_
#define _DECODER_COPY_H_

#include "Decoder.h"

class CDecoderCopy : public CDecoder
{
public:
	CDecoderCopy(void);
	bool Start(const char* sourceFile);
	~CDecoderCopy(void);
};

#endif
