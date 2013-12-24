#ifndef WIN32
#include <errno.h>
#endif
#include "StreamOutput.h"

CStreamOutput::CStreamOutput():m_fd(-1)
{
}

CStreamOutput::~CStreamOutput()
{
}

CStreamOutput *getStreamOutput(int outType)
{
	switch (outType) {
		case (CStreamOutput::FILE_STREAM):
			return new CStreamFileOutput();
		case CStreamOutput::SOCKET_STREAM:
			return new CStreamSockOutput();
		default:
			return 0; /*NULL*/
	}
}

int CStreamSockOutput::Write( char *p_nal, int i_size )
{
	if (m_fd < 0) return -1;
	int ret = -1;
	fd_set fds;
	struct timeval timeout;

	FD_ZERO(&fds);
	FD_SET((SOCKET)m_fd, &fds);
	/* Set time limit. */
	timeout.tv_sec = 20;
	timeout.tv_usec = 0;

	int rc = select(m_fd+1, NULL, &fds, NULL, &timeout);
	if (rc > 0) {
		ret = send((SOCKET)m_fd, p_nal, i_size, 0);
	} else {
		printf("select failed rc=%d!\n", rc);
	}
	return ret;
}

void  destroyStreamOutput(CStreamOutput * outStream)
{
	if(outStream) {
		outStream->Close();
		delete outStream;
	}
};

