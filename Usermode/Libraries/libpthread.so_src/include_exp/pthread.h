/*
 * Acess2 libpthread
 * - By John Hodge (thePowersGang)
 *
 * pthread.h
 * - Core POSIX threads header
 */
#ifndef _LIBPTHREAT_PTHREAD_H_
#define _LIBPTHREAT_PTHREAD_H_

// XXX: Hack - libgcc doesn't seem to be auto-detecting the presence of this header
#include "sched.h"

//! \name pthread core
//! \{
typedef struct pthread_attr_s	pthread_attr_t;

struct pthread_s
{
	void*	retval;
	unsigned int	kernel_handle;
};
typedef struct pthread_s	pthread_t;

extern int	pthread_create(pthread_t *threadptr, const pthread_attr_t * attrs, void* (*fcn)(void*), void* arg);
extern int	pthread_detach(pthread_t thread);
extern int	pthread_join(pthread_t thread, void **retvalptr);
extern int	pthread_cancel(pthread_t thread);
extern int	pthread_equal(pthread_t t1, pthread_t t2);
extern pthread_t	pthread_self(void);
extern void	pthread_exit(void* retval);
//! }


//! \name pthread_once
//! \{
struct pthread_once_s
{
	int	flag;
};
#define PTHREAD_ONCE_INIT	((struct pthread_once_s){.flag=0})
typedef struct pthread_once_s	pthread_once_t;
extern int pthread_once(pthread_once_t *once_contol, void (*init_routine)(void));
//! \}

//! \name pthread mutexes
//! \{
#define PTHREAD_MUTEX_NORMAL	0
#define PTHREAD_MUTEX_RECURSIVE	1
struct pthread_mutexattr_s
{
	int	type;
};
typedef struct pthread_mutexattr_s	pthread_mutexattr_t;
extern int pthread_mutexattr_init(pthread_mutexattr_t *attrs);
extern int pthread_mutexattr_settype(pthread_mutexattr_t *attrs, int type);
extern int pthread_mutexattr_destroy(pthread_mutexattr_t *attrs);

struct pthread_mutex_s
{
	void*	futex;
};
#define PTHREAD_MUTEX_INITIALIZER	((struct pthread_mutex_s){NULL})
typedef struct pthread_mutex_s	pthread_mutex_t;
extern int pthread_mutex_init(pthread_mutex_t * mutex, const pthread_mutexattr_t *attrs);
extern int pthread_mutex_lock(pthread_mutex_t *lock);
extern int pthread_mutex_trylock(pthread_mutex_t *lock);
extern int pthread_mutex_unlock(pthread_mutex_t *lock);
extern int pthread_mutex_destroy(pthread_mutex_t *lock);
//! \}

//! \name pthread TLS keys
//! \{
//typedef struct pthread_key_s	pthread_key_t;
typedef unsigned int	pthread_key_t;
extern int pthread_key_create(pthread_key_t *keyptr, void (*fcn)(void*));
extern int pthread_key_delete(pthread_key_t key);
extern int pthread_setspecific(pthread_key_t key, const void* data);
extern void* pthread_getspecific(pthread_key_t key);
//! \}

//! \name pthread condvars
//! \{
typedef struct pthread_cond_s	pthread_cond_t;
typedef struct pthread_condattr_s	pthread_condattr_t;
extern int pthread_cond_init(pthread_cond_t *condptr, const pthread_condattr_t* attrs);
extern int pthread_cond_wait(pthread_cond_t *condptr, pthread_mutex_t *mutex);
extern int pthread_cond_timedwait(pthread_cond_t *condptr, pthread_mutex_t *mutex, const struct timespec *timeout);
extern int pthread_cond_signal(pthread_cond_t *condptr);
extern int pthread_cond_broadcast(pthread_cond_t *condptr);
extern int pthread_cond_destroy(pthread_cond_t *condptr);
//! \}

#endif

