/////////////////////////////////////////////////////////////////////
//  Written by Phillip Sitbon
//  Copyright 2003
//
//  Win32/Mutex.h
//    - Resource locking mechanism using Critical Sections
//
/////////////////////////////////////////////////////////////////////

#ifndef _Mutex_Win32_
#define _Mutex_Win32_

#include "Win32.h"

class CBIMutex
{
  mutable CRITICAL_SECTION C;
  void operator=(CBIMutex &M) {}
  CBIMutex( const CBIMutex &M ) {}

  public:

  CBIMutex()
  { InitializeCriticalSection(&C); }

  virtual ~CBIMutex()
  { DeleteCriticalSection(&C); }

  int Lock() const
  { EnterCriticalSection(&C); return 0; }

#if(_WIN32_WINNT >= 0x0400)
  int Lock_Try() const
  { return (TryEnterCriticalSection(&C)?0:EBUSY); }
#endif

  int Unlock() const
  { LeaveCriticalSection(&C); return 0; }
};

#endif // !_Mutex_Win32_
