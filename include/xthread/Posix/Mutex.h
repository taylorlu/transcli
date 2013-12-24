/////////////////////////////////////////////////////////////////////
//  Written by Phillip Sitbon
//  Copyright 2003
//
//  Posix/CBIMutex.h
//    - Resource locking mechanism using Posix mutexes
//
/////////////////////////////////////////////////////////////////////

#ifndef _Mutex_Posix_
#define _Mutex_Posix_

#include <pthread.h>

class CBIMutex
{
public:

  CBIMutex(bool beRecursive = false)
  {
    if (beRecursive) {
      pthread_mutexattr_t attr;
      pthread_mutexattr_init(&attr);
      pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE);
      pthread_mutex_init(&mutex,&attr);
      pthread_mutexattr_destroy(&attr);
	}
	else {
		pthread_mutex_init(&mutex, NULL);
	}
  }

  virtual ~CBIMutex()
  { 
	  pthread_mutex_unlock(&mutex); 
	  pthread_mutex_destroy(&mutex); 
  }

  int Lock() const
  { 
	  return pthread_mutex_lock(&mutex); 
  }

  int Lock_Try() const
  { 
	  return pthread_mutex_trylock(&mutex); 
  }

  int Unlock() const
  { 
	  return pthread_mutex_unlock(&mutex); 
  }

private:
  mutable pthread_mutex_t mutex;
  void operator=(CBIMutex &mutex) {}
  CBIMutex( const CBIMutex &mutex ) {}
};

#endif // !_Mutex_Posix_
