#ifndef __XZCOMPRESS_H
#define __XZCOMPRESS_H

#ifndef LZMA_API_STATIC
#define  LZMA_API_STATIC 1
#endif

#pragma comment(lib,"liblzma.a")

class xzcompress
{
public:
	xzcompress(void);
	~xzcompress(void);

	int compress(char *pBufferIn, int iBufferIn,char* pBufferOut,int &iBufferOut);


};

#endif
