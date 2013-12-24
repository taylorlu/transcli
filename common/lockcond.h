/*****************************************************************************
 * lockconf.h :
 *****************************************************************************
 * Copyright (C) 2010 Broad Intelligence Technologies
 *
 * Author:
 *
 * Notes:
 *****************************************************************************/

#ifndef __LOCK_COND_H__
#define __LOCK_COND_H__

#ifdef WIN32
#include <Windows.h>
#undef BIT_USE_PTHREAD
#else
#define BIT_USE_PTHREAD
#endif

/*****************************************************************************
 * Type definitions
 *****************************************************************************/

#if defined (BIT_USE_PTHREAD)
typedef pthread_t       bit_thread_t;
typedef pthread_mutex_t bit_mutex_t;
#define BIT_STATIC_MUTEX PTHREAD_MUTEX_INITIALIZER
typedef pthread_cond_t  bit_cond_t;
typedef pthread_key_t   bit_threadvar_t;

#elif defined( WIN32 )
typedef struct
{
    HANDLE handle;
    void  *(*entry) (void *);
    void  *data;
#if defined( UNDER_CE )
    HANDLE cancel_event;
#endif
} *bit_thread_t;

typedef struct
{
    LONG initialized;
    CRITICAL_SECTION mutex;
} bit_mutex_t;
#define BIT_STATIC_MUTEX { 0, }

typedef HANDLE  bit_cond_t;
typedef DWORD   bit_threadvar_t;
#endif

#if defined( WIN32 ) && !defined ETIMEDOUT
#  define ETIMEDOUT 10060 /* This is the value in winsock.h. */
#endif
#endif

int bit_mutex_init( bit_mutex_t *p_mutex );
int bit_mutex_init_recursive( bit_mutex_t *p_mutex );
void bit_mutex_destroy (bit_mutex_t *p_mutex);
void bit_mutex_lock (bit_mutex_t *p_mutex);
int bit_mutex_trylock (bit_mutex_t *p_mutex);
void bit_mutex_unlock (bit_mutex_t *p_mutex);

int bit_cond_init( bit_cond_t *p_condvar );
void bit_cond_destroy (bit_cond_t *p_condvar);
void bit_cond_signal (bit_cond_t *p_condvar);
void bit_cond_broadcast (bit_cond_t *p_condvar);
void bit_cond_wait (bit_cond_t *p_condvar, bit_mutex_t *p_mutex);
