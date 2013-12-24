#ifndef _BI_Thread_
#define _BI_Thread_

#include <errno.h>

#ifdef WIN32
	#include "xthread/Win32/Thread.h"
	#include "xthread/Win32/Mutex.h"
	#include "xthread/Win32/Semaphore.h"
#else
	#include "xthread/Posix/Thread.h"
	#include "xthread/Posix/Mutex.h"
	#include "xthread/Posix/Semaphore.h"
#endif

#endif // !_Thread_
