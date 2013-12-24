/*****************************************************************************
 * lockcond.cpp :
 *****************************************************************************
 * Copyright (C) 2010 Broad Intelligence Technologies
 *
 * Author:
 *
 * Notes:
 *****************************************************************************/
#include <assert.h>
#ifdef WIN32
#include <errno.h>
#endif

#include "lockcond.h"

#ifdef WIN32
typedef __int64 mtime_t;
#else
typedef int64_t mtime_t;
#endif

/*****************************************************************************
 * bit_mutex_init: initialize a mutex
 *****************************************************************************/
int bit_mutex_init( bit_mutex_t *p_mutex )
{
#if defined( BIT_USE_PTHREAD )
    pthread_mutexattr_t attr;
    int                 i_result;

    pthread_mutexattr_init( &attr );

# ifndef NDEBUG
    /* Create error-checking mutex to detect problems more easily. */
#  if defined (__GLIBC__) && (__GLIBC_MINOR__ < 6)
    pthread_mutexattr_setkind_np( &attr, PTHREAD_MUTEX_ERRORCHECK_NP );
#  else
    pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_ERRORCHECK );
#  endif
# endif
    i_result = pthread_mutex_init( p_mutex, &attr );
    pthread_mutexattr_destroy( &attr );
    return i_result;

#elif defined( WIN32 )
    /* This creates a recursive mutex. This is OK as fast mutexes have
     * no defined behavior in case of recursive locking. */
    InitializeCriticalSection (&p_mutex->mutex);
    p_mutex->initialized = 1;
    return 0;

#endif
}

/*****************************************************************************
 * vlc_mutex_init: initialize a recursive mutex (Do not use)
 *****************************************************************************/
int bit_mutex_init_recursive( bit_mutex_t *p_mutex )
{
#if defined( BIT_USE_PTHREAD )
    pthread_mutexattr_t attr;
    int                 i_result;

    pthread_mutexattr_init( &attr );
#  if defined (__GLIBC__) && (__GLIBC_MINOR__ < 6)
    pthread_mutexattr_setkind_np( &attr, PTHREAD_MUTEX_RECURSIVE_NP );
#  else
    pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE );
#  endif
    i_result = pthread_mutex_init( p_mutex, &attr );
    pthread_mutexattr_destroy( &attr );
    return( i_result );

#elif defined( WIN32 )
    InitializeCriticalSection( &p_mutex->mutex );
    p_mutex->initialized = 1;
    return 0;

#endif
}


/**
 * Destroys a mutex. The mutex must not be locked.
 *
 * @param p_mutex mutex to destroy
 * @return always succeeds
 */
void bit_mutex_destroy (bit_mutex_t *p_mutex)
{
#if defined( BIT_USE_PTHREAD )
    int val = pthread_mutex_destroy( p_mutex );
    DBG ("destroying mutex");

#elif defined( WIN32 )
    assert (InterlockedExchange (&p_mutex->initialized, -1) == 1);
    DeleteCriticalSection (&p_mutex->mutex);

#endif
}

/**
 * Acquires a mutex.
 *
 * @param p_mutex mutex initialized with bit_mutex_init() or
 *                bit_mutex_init_recursive()
 */
void bit_mutex_lock (bit_mutex_t *p_mutex)
{
#if defined(BIT_USE_PTHREAD)
    int val = pthread_mutex_lock( p_mutex );
    DBG ("locking mutex");

#elif defined( WIN32 )
    assert (InterlockedExchange (&p_mutex->initialized, 1) == 1);
	EnterCriticalSection (&p_mutex->mutex);
#endif
}

/**
 * Acquires a mutex if and only if it is not currently held by another thread.
 * This function never sleeps and can be used in delay-critical code paths.
 * This function is not a cancellation-point.
 *
 * @param p_mutex mutex initialized with vlc_mutex_init() or
 *                vlc_mutex_init_recursive()
 * @return 0 if the mutex could be acquired, an error code otherwise.
 */
int bit_mutex_trylock (bit_mutex_t *p_mutex)
{
#if defined(BIT_USE_PTHREAD)
    int val = pthread_mutex_trylock( p_mutex );

    if (val != EBUSY)
        DBG ("locking mutex");
    return val;

#elif defined( WIN32 )
    assert (InterlockedExchange (&p_mutex->initialized, 1) == 1);
	return TryEnterCriticalSection (&p_mutex->mutex) ? 0 : EBUSY;
#endif
}

/**
 * Releases a mutex (or crashes if the mutex is not locked by the caller).
 * @param p_mutex mutex locked with vlc_mutex_lock().
 */
void bit_mutex_unlock (bit_mutex_t *p_mutex)
{
#if defined(BIT_USE_PTHREAD)
    int val = pthread_mutex_unlock( p_mutex );
    DBG ("unlocking mutex");

#elif defined( WIN32 )
    assert (InterlockedExchange (&p_mutex->initialized, 1) == 1);
    LeaveCriticalSection (&p_mutex->mutex);

#endif
}

/*****************************************************************************
 * bit_cond_init: initialize a condition variable
 *****************************************************************************/
int bit_cond_init( bit_cond_t *p_condvar )
{
#if defined( BIT_USE_PTHREAD )
    pthread_condattr_t attr;
    int ret;

    ret = pthread_condattr_init (&attr);
    if (ret)
        return ret;

# if !defined (_POSIX_CLOCK_SELECTION)
   /* Fairly outdated POSIX support (that was defined in 2001) */
#  define _POSIX_CLOCK_SELECTION (-1)
# endif
# if (_POSIX_CLOCK_SELECTION >= 0)
    /* NOTE: This must be the same clock as the one in mtime.c */
    pthread_condattr_setclock (&attr, CLOCK_MONOTONIC);
# endif

    ret = pthread_cond_init (p_condvar, &attr);
    pthread_condattr_destroy (&attr);
    return ret;

#elif defined( WIN32 )
    /* Create a manual-reset event (manual reset is needed for broadcast). */
    *p_condvar = CreateEvent( NULL, TRUE, FALSE, NULL );
    return *p_condvar ? 0 : ENOMEM;

#endif
}

/**
 * Destroys a condition variable. No threads shall be waiting or signaling the
 * condition.
 * @param p_condvar condition variable to destroy
 */
void bit_cond_destroy (bit_cond_t *p_condvar)
{
#if defined( BIT_USE_PTHREAD )
    int val = pthread_cond_destroy( p_condvar );
    VLC_THREAD_ASSERT ("destroying condition");

#elif defined( WIN32 )
    CloseHandle( *p_condvar );

#endif
}

/**
 * Wakes up one thread waiting on a condition variable, if any.
 * @param p_condvar condition variable
 */
void bit_cond_signal (bit_cond_t *p_condvar)
{
#if defined(BIT_USE_PTHREAD)
    int val = pthread_cond_signal( p_condvar );
    VLC_THREAD_ASSERT ("signaling condition variable");

#elif defined( WIN32 )
    /* NOTE: This will cause a broadcast, that is wrong.
     * This will also wake up the next waiting thread if no thread are yet
     * waiting, which is also wrong. However both of these issues are allowed
     * by the provision for spurious wakeups. Better have too many wakeups
     * than too few (= deadlocks). */
    SetEvent (*p_condvar);

#endif
}

/**
 * Wakes up all threads (if any) waiting on a condition variable.
 * @param p_cond condition variable
 */
void bit_cond_broadcast (bit_cond_t *p_condvar)
{
#if defined (BIT_USE_PTHREAD)
    pthread_cond_broadcast (p_condvar);

#elif defined (WIN32)
    SetEvent (*p_condvar);

#endif
}

/**
 * Waits for a condition variable. The calling thread will be suspended until
 * another thread calls bit_cond_signal() or bit_cond_broadcast() on the same
 * condition variable.
 *
 * A mutex is needed to wait on a condition variable. It must <b>not</b> be
 * a recursive mutex. Although it is possible to use the same mutex for
 * multiple condition, it is not valid to use different mutexes for the same
 * condition variable at the same time from different threads.
 *
 * @param p_condvar condition variable to wait on
 * @param p_mutex mutex which is unlocked while waiting,
 *                then locked again when waking up.
 * @param deadline <b>absolute</b> timeout
 *
 * @return 0 if the condition was signaled, an error code in case of timeout.
 */
void bit_cond_wait (bit_cond_t *p_condvar, bit_mutex_t *p_mutex)
{
#if defined(BIT_USE_PTHREAD)
    int val = pthread_cond_wait( p_condvar, p_mutex );
    DBG ("waiting on condition");

#elif defined( WIN32 )
    DWORD result;

    do
    {
        LeaveCriticalSection (&p_mutex->mutex);
        result = WaitForSingleObjectEx (*p_condvar, INFINITE, TRUE);
        EnterCriticalSection (&p_mutex->mutex);
    }
    while (result == WAIT_IO_COMPLETION);

    assert (result != WAIT_ABANDONED); /* another thread failed to cleanup! */
    assert (result != WAIT_FAILED);
    ResetEvent (*p_condvar);

#endif
}

#if 0 //TODO
/**
 * Waits for a condition variable up to a certain date.
 * This works like vlc_cond_wait(), except for the additional timeout.
 *
 * @param p_condvar condition variable to wait on
 * @param p_mutex mutex which is unlocked while waiting,
 *                then locked again when waking up.
 * @param deadline <b>absolute</b> timeout
 *
 * @return 0 if the condition was signaled, an error code in case of timeout.
 */
int bit_cond_timedwait (bit_cond_t *p_condvar, bit_mutex_t *p_mutex,
                        mtime_t deadline)
{
#if defined(BIT_USE_PTHREAD)
#if defined(__APPLE__) && !defined(__powerpc__) && !defined( __ppc__ ) && !defined( __ppc64__ )
    /* mdate() is mac_absolute_time on osx, which we must convert to do
     * the same base than gettimeofday() on which pthread_cond_timedwait
     * counts on. */
    mtime_t oldbase = mdate();
    struct timeval tv;
    gettimeofday(&tv, NULL);
    mtime_t newbase = (mtime_t)tv.tv_sec * 1000000 + (mtime_t) tv.tv_usec;
    deadline = deadline - oldbase + newbase;
#endif
    lldiv_t d = lldiv( deadline, CLOCK_FREQ );
    struct timespec ts = { d.quot, d.rem * (1000000000 / CLOCK_FREQ) };

    int val = pthread_cond_timedwait (p_condvar, p_mutex, &ts);
    if (val != ETIMEDOUT)
        VLC_THREAD_ASSERT ("timed-waiting on condition");
    return val;

#elif defined( WIN32 )
    DWORD result;

    do
    {
        mtime_t total = (deadline - mdate ())/1000;
        if( total < 0 )
            total = 0;

        DWORD delay = (total > 0x7fffffff) ? 0x7fffffff : total;
        LeaveCriticalSection (&p_mutex->mutex);
        result = WaitForSingleObjectEx (*p_condvar, delay, TRUE);
        EnterCriticalSection (&p_mutex->mutex);
    }
    while (result == WAIT_IO_COMPLETION);

    assert (result != WAIT_ABANDONED);
    assert (result != WAIT_FAILED);
    ResetEvent (*p_condvar);

    return (result == WAIT_OBJECT_0) ? 0 : ETIMEDOUT;

#endif
}
#endif
