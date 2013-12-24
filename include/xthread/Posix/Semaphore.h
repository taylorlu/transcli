/////////////////////////////////////////////////////////////////////
//  Written by Phillip Sitbon
//  Copyright 2003
//
//  Posix/Semaphore.h
//    - Resource counting mechanism
//
/////////////////////////////////////////////////////////////////////

#ifndef _Semaphore_Posix_
#define _Semaphore_Posix_

#include <semaphore.h>

class CBISemaphore
{
  sem_t sem;

  public:
  CBISemaphore( int init = 0 )
  { 
	  sem_init(&sem,0,init); 
  }

  virtual ~CBISemaphore()
  { 
	  sem_destroy(&sem); 
  }

  void Wait() const
  { 
	  sem_wait((sem_t *)&sem); 
  }

  int Wait_Try() const
  { 
	  return (sem_trywait((sem_t *)&sem)?errno:0);
  }

  int Post() const
  { 
	  return (sem_post((sem_t *)&sem)?errno:0);
  }

  int Value() const
  { 
	  int val = -1; 
	  sem_getvalue((sem_t *)&sem,&val);
	  return val;
  }

  void Reset( int init = 0 )
  { 
	  sem_destroy(&sem);
	  sem_init(&sem,0,init);
  }
};

#endif // !_Semaphore_Posix_
