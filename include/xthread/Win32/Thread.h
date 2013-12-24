/////////////////////////////////////////////////////////////////////
//  Written by Phillip Sitbon
//  Copyright 2003
//
//  Win32/Thread.h
//    - Windows thread
//
//  - From CreateThread Platform SDK Documentation:
//
//  "A thread that uses functions from the static C run-time 
//    libraries should use the beginthread and endthread C run-time
//    functions for thread management rather than CreateThread and
//    ExitThread. Failure to do so results in small memory leaks
//    when ExitThread is called. Note that this is not a problem
//    with the C run-time in a DLL."
//
//  With regards to this, I have decided to use the CreateThread
//  API, unless you define _CRT_ in which case there are two
//  possibilities:
//
//    1. Define _USE_BEGINTHREAD: Uses _beginthread/_endthread
//        (said to be *unreliable* in the SDK docs)
//
//    2. Don't - Uses _beginthreaded/_endthreadex
//
//  A note about _endthread:
//
//    It will call CloseHandle() on exit, and if it was already
//    closed then you will get an exception. To prevent this, I
//    removed the CloseHandle() functionality - this means that
//    a Join() WILL wait on a Detach()'ed thread.
//
/////////////////////////////////////////////////////////////////////
#ifndef _Thread_Win32_
#define _Thread_Win32_

#include "Win32.h"

#ifdef _CRT_
# include <process.h>
# ifdef _USE_BEGINTHREAD
#   define THREAD_CALL                __cdecl
#   define THREAD_HANDLE              uintptr_t
#   define THREAD_RET_T               void
#   define CREATE_THREAD_FAILED       (-1L)
#   define CREATE_THREAD_ERROR        (errno)
#   define CREATE_THREAD(_S,_F,_P)    ((Handle)_beginthread((void (__cdecl *)(void *))_F,_S,(void *)_P))
#   define EXIT_THREAD                _endthread()
#   define CLOSE_THREAD_HANDLE(x)            1
#   define THREAD_RETURN(x)           return
# else
#   define THREAD_CALL                WINAPI
#   define THREAD_HANDLE              HANDLE
#   define THREAD_RET_T               UINT
#   define CREATE_THREAD_FAILED       (0L)
#   define CREATE_THREAD_ERROR        (errno)
#   define CREATE_THREAD(_S,_F,_P)    ((Handle)_beginthreadex(0,_S,(UINT (WINAPI *)(void *))_F,(void *)_P,0,0))
#   define EXIT_THREAD                _endthreadex(0)
#   define CLOSE_THREAD_HANDLE(x)            CloseHandle(x)
#   define THREAD_RETURN(x)           return(x)
# endif
#else
# define THREAD_CALL                WINAPI
# define THREAD_HANDLE              HANDLE
# define THREAD_RET_T               DWORD
# define CREATE_THREAD_FAILED       (0L)
# define CREATE_THREAD_ERROR        GetLastError()
# define CREATE_THREAD(_S,_F,_P)    ((Handle)CreateThread(0,_S,(DWORD (WINAPI *)(void *))_F,(void *)_P,0,0))
# define EXIT_THREAD                ExitThread(0)
# define CLOSE_THREAD_HANDLE(x)     CloseHandle(x)
# define THREAD_RETURN(x)           return(x)
#endif

#define InvalidHandle 0


/////////////////////////////////////////////////////////////////////

class CBIThread
{
public:
	typedef THREAD_HANDLE Handle;
	typedef LPTHREAD_START_ROUTINE ThreadFunc;

    static int Create(
      Handle              &tHandle,
	  ThreadFunc          tFunc,
	  void				  *param,
      bool				  createDetached  = false,
      unsigned int		  stackSize       = 0,
      bool                cancelEnable    = false,   // UNUSED
      bool                cancelAsync     = false    // UNUSED
    )
    {
      tHandle = CREATE_THREAD(stackSize, tFunc, param);

      if ( tHandle == CREATE_THREAD_FAILED )
      {
        if ( tHandle ) tHandle = InvalidHandle;
        return CREATE_THREAD_ERROR;
      }

      if ( createDetached ) CLOSE_THREAD_HANDLE(tHandle);
      return 0;
    }

	static void Exit()
	{ 
		EXIT_THREAD; 
	}

    static int Join(Handle tHandle, THREAD_RET_T* retCode = NULL, int timeout = INFINITE)
    {
      DWORD R = WaitForSingleObject(tHandle, timeout);

      if ( (R == WAIT_OBJECT_0) || (R == WAIT_ABANDONED) )
      {
	    if(retCode != NULL) GetExitCodeThread(tHandle, retCode);
        CLOSE_THREAD_HANDLE(tHandle);
        return 0;
      }
	
	  if(retCode != NULL) *retCode = 1000;		// An invalid return code
      if ( R == WAIT_TIMEOUT ) return EAGAIN;
      return EINVAL;
    }

	static int SetPriority(Handle tHandle, int priority)
	{
		return SetThreadPriority(tHandle, priority);
	}

    static int Kill(Handle tHandle )
    { 
		return TerminateThread(tHandle,0) ? 0 : EINVAL; 
	}

    static int Detach(Handle tHandle)
    { 
		return (CLOSE_THREAD_HANDLE(tHandle)? 0 : EINVAL);
	}

private:
	CBIThread() {}
};

#endif // !_Thread_Win32_
