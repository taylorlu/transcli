/////////////////////////////////////////////////////////////////////
//  Written by Phillip Sitbon
//  Copyright 2003
//
//  Posix/Thread.h
//    - Posix thread
//
/////////////////////////////////////////////////////////////////////

#ifndef _Thread_Posix_
#define _Thread_Posix_

#include <pthread.h>
#include <stdio.h>

#define InvalidHandle 0


#define THREAD_PRIORITY_LOWEST          10
#define THREAD_PRIORITY_BELOW_NORMAL    30
#define THREAD_PRIORITY_NORMAL          50
#define THREAD_PRIORITY_HIGHEST         90
#define THREAD_PRIORITY_ABOVE_NORMAL    70
#define THREAD_PRIORITY_ERROR_RETURN    (0x7fffffff)
#define THREAD_PRIORITY_TIME_CRITICAL   99
#define THREAD_PRIORITY_IDLE            1
#define CLOSE_THREAD_HANDLE	

#define THREAD_RET_T unsigned long
/////////////////////////////////////////////////////////////////////

class CBIThread
{
public:
	struct sched_param {
		int __sched_priority;
	};

	typedef pthread_t Handle;
	typedef THREAD_RET_T (*ThreadFunc)(void*);
	typedef void* (*ThreadFuncEx)(void*);

	static int Create(
		Handle              &tHandle,
		ThreadFunc          tFunc,
		void				*param,
		bool				createDetached  = false,
		unsigned int		stackSize       = 0,
		bool                cancelEnable    = false,   // UNUSED
		bool                cancelAsync     = false    // UNUSED
	)
    {
      int ret;

      pthread_attr_t attr;
      pthread_attr_init(&attr);
	  // Change policy
	//  pthread_attr_setschedpolicy(&attr,   SCHED_RR);
      if ( createDetached )
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

      if ( stackSize )
        pthread_attr_setstacksize(&attr, stackSize);

      ret = pthread_create((pthread_t *)&tHandle, &attr, (ThreadFuncEx)tFunc, (void *)param);
      pthread_attr_destroy(&attr);

      return ret;
    }

	static void Exit()
	{ 
		pthread_exit(0);
	}

    static int Join( Handle tHandle, THREAD_RET_T* retCode = NULL, int timeout = 0 /*NOT USED*/ )
    { 
		if(retCode == NULL) return pthread_join(tHandle, NULL);
		return pthread_join(tHandle, ((void**)&retCode));
	}

	static int SetPriority(Handle tHandle, int priority)
	{
#ifdef HAVE_LINUX
	/*	int ret = pthread_setschedprio(tHandle, priority);
		switch(ret) {
		case 0:
			printf("Set thread priority to %d successfully!\n", priority);
			break;
		case EINVAL:
			printf("The value of priority %d is invalid for the scheduling policy of the specified thread.\n", priority);
			break;
		case ENOTSUP:
			printf("An attempt was made to set the priority to an unsupported value.\n");
			break;
		case EPERM:
			printf("The implementation does not allow the application to modify the priority to the value specified.\n");
			break;
		case ESRCH:
			printf("No thread could be found corresponding to thread.\n");
			break;
		}*/
		return 0;
#else
    //TODO: SetPriority implentment for MAC OSX
    return 0;
#endif
	}

    static int Kill( Handle tHandle )
    { 
		return pthread_cancel(tHandle);
	}

    static int Detach( Handle tHandle )
    { 
		return pthread_detach(tHandle); 
	}

private:
   CBIThread() {}
};

#endif // !_Thread_Posix_
