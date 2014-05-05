//#include "StdAfx.h"
#include <iostream>
#include "xzcompress.h"
extern "C"
{
    #include "lzma/lzma.h"
};
using namespace std;

/* analogous to xz CLI options: -0 to -9 */
#define COMPRESSION_LEVEL 6

/* boolean setting, analogous to xz CLI option: -e */
#define COMPRESSION_EXTREME true

/* see: /usr/include/lzma/check.h LZMA_CHECK_* */
#define INTEGRITY_CHECK LZMA_CHECK_CRC64

/* read/write buffer sizes */
#define IN_BUF_MAX	4096
#define OUT_BUF_MAX	4096

#define RET_OK			0
#define RET_ERROR_INIT		1
#define RET_ERROR_INPUT		2
#define RET_ERROR_OUTPUT	3
#define RET_ERROR_COMPRESSION	4

xzcompress::xzcompress(void)
{
}

xzcompress::~xzcompress(void)
{
	
}


int xzcompress::compress(char *pBufferIn, int iBufferIn, char* pBufferOut, int& iBufferOut)
{
	if ((pBufferIn == NULL) || (pBufferOut == NULL)) {
		return -1;
	}

	uint32_t preset = COMPRESSION_LEVEL | (COMPRESSION_EXTREME ? LZMA_PRESET_EXTREME : 0);
	lzma_check check = INTEGRITY_CHECK;
	lzma_stream strm = LZMA_STREAM_INIT; /* alloc and init lzma_stream struct */
	uint8_t in_buf [IN_BUF_MAX];
	uint8_t out_buf [OUT_BUF_MAX];
	size_t in_len;	/* length of useful data in in_buf */
	size_t out_len;	/* length of useful data in out_buf */

	unsigned long in_buf_pos = 0;
	unsigned long out_buf_pos = 0;

	bool in_finished = false;
	bool out_finished = false;
	lzma_action action;
	lzma_ret ret_xz;
	int ret;

	ret = RET_OK;

	/* initialize xz encoder */
	ret_xz = lzma_easy_encoder (&strm, preset, check);
	if (ret_xz != LZMA_OK) {
		fprintf (stderr, "lzma_easy_encoder error: %d\n", (int) ret_xz);
		return RET_ERROR_INIT;
	}

	while ((! in_finished) && (! out_finished)) {
		/* read incoming data */
#if 0
		in_len = fread (in_buf, 1, IN_BUF_MAX, in_file);
	
		if (feof (in_file)) {
			in_finished = true;
		}
		if (ferror (in_file)) {
			in_finished = true;
			ret = RET_ERROR_INPUT;
		}
#else
		if ( (IN_BUF_MAX + in_buf_pos)<= iBufferIn) {
			memcpy(in_buf, pBufferIn + in_buf_pos, IN_BUF_MAX);
			in_len = IN_BUF_MAX;
			in_buf_pos += IN_BUF_MAX;
		}
		else {
			memcpy(in_buf, pBufferIn + in_buf_pos, (iBufferIn - in_buf_pos));
			in_len = iBufferIn - in_buf_pos;
			in_buf_pos = iBufferIn;
		}

		if (in_buf_pos == iBufferIn) {
			in_finished = true;
			//ret = RET_ERROR_INPUT;
		}

#endif
		strm.next_in = in_buf;
		strm.avail_in = in_len;

		/* if no more data from in_buf, flushes the
		   internal xz buffers and closes the xz data
		   with LZMA_FINISH */
		action = in_finished ? LZMA_FINISH : LZMA_RUN;

		/* loop until there's no pending compressed output */
		do {
			/* out_buf is clean at this point */
			strm.next_out = out_buf;
			strm.avail_out = OUT_BUF_MAX;

			/* compress data */
			ret_xz = lzma_code (&strm, action);

			if ((ret_xz != LZMA_OK) && (ret_xz != LZMA_STREAM_END)) {
				fprintf (stderr, "lzma_code error: %d\n", (int) ret_xz);
				out_finished = true;
				ret = RET_ERROR_COMPRESSION;
			} else {
				/* write compressed data */
				out_len = OUT_BUF_MAX - strm.avail_out;
#if 0
				fwrite (out_buf, 1, out_len, out_file);
				if (ferror (out_file)) {
					out_finished = true;
					ret = RET_ERROR_OUTPUT;
				}
#else
				//if (out_buf_pos <= 2*1024*1024) 
				{
					printf("out_buf_pos = %d, out_len = %d\n", out_buf_pos, out_len);
					memcpy(pBufferOut + out_buf_pos, out_buf, out_len);
					out_buf_pos += out_len;
					iBufferOut = out_buf_pos;
				}
#endif
			}
		} while (strm.avail_out == 0);
	}

	lzma_end (&strm);
	return ret;

}
