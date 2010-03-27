#ifndef EDI_PTHREADS

/* Copyright (c)  2006  Eli Gottlieb.
 * Permission is granted to copy, distribute and/or modify this document
 * under the terms of the GNU Free Documentation License, Version 1.2
 * or any later version published by the Free Software Foundation;
 * with no Invariant Sections, no Front-Cover Texts, and no Back-Cover
 * Texts.  A copy of the license is included in the file entitled "COPYING". */

#define EDI_PTHREADS
/*!\file edi_pthreads.h
 * \brief A basic subset of POSIX Threads functionality, providing threading and thread synchronization.
 *
 * A very basic POSIX Threads interface.  Note that pthreads are not a class, because none of these calls really gels with
 * object-oriented programming.  Also, if drivers aren't processes or threads under the implementing operating system a small
 * threading system must be implemented in-runtime just to multiplex the pthreads of EDI drivers.  Sorry about that.
 *
 * Data structures and algorithms this header represents:
 *
 * 	ALGORITHM AND DATA STRUCTURE: POSIX Threading - The runtime must provide enough of a POSIX threading interface to implement
 * the calls described here.  The actual multithreading must be performed by the runtime, and the runtime can implement that
 * multithreading however it likes as long as the given POSIX Threads subset works.  There is, however, a caveat: since the runtime
 * calls the driver like it would a library, the driver must perceive all calls made to it by the runtime as running under one thread.
 * From this thread the driver can create others.  Such behavior is a quirk of EDI, and does not come from the POSIX standard.
 * However, it is necessary to provide the driver with a thread for its own main codepaths.  For further details on a given POSIX
 * Threading routine, consult its Unix manual page. */

#include "edi_objects.h"

/* Placeholder type definitions.  Users of the PThreads interface only ever need to define pointers to these types. */
/*!\brief Opaque POSIX Threading thread attribute type. */
typedef void pthread_attr_t;
/*!\brief Opaque POSIX Threading mutex (mutual exclusion semaphore) type. */
typedef void pthread_mutex_t;
/*!\brief Opaque POSIX Threading mutex attribute type. */
typedef void pthread_mutex_attr_t;

/*!\struct sched_param
 * \brief POSIX Threading scheduler parameters for a thread. */
typedef struct {
	/*!\brief The priority of the thread. */
	int32_t sched_priority;
} sched_param;

/*!\brief POSIX Threading thread identifier. */
typedef uint32_t pthread_t;
/*!\brief POSIX Threading thread function type.
 *
 * A function pointer to a thread function, with the required signature of a thread function.  A thread function takes one untyped
 * pointer as an argument and returns an untyped pointer.  Such a function is a thread's main routine: it's started with the thread,
 * and the thread exits if it returns. */
typedef void *(*pthread_function_t)(void*);

/*!\brief Insufficient resources. */
#define EAGAIN -1
/*!\brief Invalid parameter. */
#define EINVAL -2
/*!\brief Permission denied. */
#define EPERM -3
/*!\brief Operation not supported. */
#define ENOTSUP -4
/*!\brief Priority scheduling for POSIX/multiple schedulers is not implemented. */
#define ENOSYS -5
/*!\brief Out of memory. */
#define ENOMEM -6
/*!\brief Deadlock.  Crap. */
#define EDEADLK -7
/*!\brief Busy.  Mutex already locked. */
#define EBUSY -8

/*!\brief Scheduling policy for regular, non-realtime scheduling.  The default. */
#define SCHED_OTHER 0
/*!\brief Real-time, first-in first-out scheduling policy.  Requires special (superuser, where such a thing exists) permissions. */
#define SCHED_FIFO 1
/*!\brief Real-time, round-robin scheduling policy.  Requires special (superuser, where such a thing exists) permissions. */
#define SCHED_RR 0

/*!\brief Creates a new thread with the given attributes, thread function and arguments, giving back the thread ID of the new
 * thread.
 *
 * pthread_create() creates a new thread of control that executes concurrently with the calling thread.  The new thread applies the
 * function start_routine, passing it arg as its argument.  The attr argument specifies thread attributes to apply to the new thread;
 * it can also be NULL for the default thread attributes (joinable with default scheduling policy).  On success this function returns
 * 0 and places the identifier of the new thread into thread_id.  On an error, pthread_create() can return EAGAIN if insufficient
 * runtime resources are available to create the requested thread, EINVAL a value specified by attributes is invalid, or EPERM if the
 * caller doesn't have permissions to set the given attributes.
 *
 * For further information: man 3 pthread_create */
int32_t pthread_create(pthread_t *thread_id, const pthread_attr_t *attributes, pthread_function_t thread_function, void *arguments);
/*!\brief Terminates the execution of the calling thread.  The thread's exit code with by status, and this routine never returns. */
void pthread_exit(void *status);
/*!\brief Returns the thread identifier of the calling thread. */
pthread_t pthread_self();
/*!\brief Compares two thread identifiers.
 *
 * Determines of the given two thread identifiers refer to the same thread.  If so, returns non-zero.  Otherwise, 0 is returned. */
int32_t pthread_equal(pthread_t thread1, pthread_t thread2);
/*!\brief Used by the calling thread to relinquish use of the processor.  The thread then waits in the run queue to be scheduled
 * again. */
void pthread_yield();

/*!\brief Gets the scheduling policy of the given attributes.
 *
 * Places the scheduling policy for attributes into policy.  Returns 0 on success, EINVAL if attributes was invalid, and ENOSYS if
 * priority scheduling/multiple scheduler support is not implemented. */
int32_t pthread_attr_getschedpolicy(const pthread_attr_t *attributes, int32_t *policy);
/*!\brief Sets the scheduling policy of the given attributes.
 *
 * Requests a switch of scheduling policy to policy for the given attributes.  Can return 0 for success, EINVAL if the given policy
 * is not one of SCHED_OTHER, SCHED_FIFO or SCHED_RR or ENOTSUP if policy is either SCHED_FIFO or SCHED_RR and the driver is not
 * running with correct privileges. */
int32_t pthread_attr_setschedpolicy(pthread_attr_t *attributes, int32_t policy);

/*!\brief Gets the scheduling paramaters (priority) from the given attributes.
 *
 * On success, stores scheduling parameters in param from attributes, and returns 0.  Otherwise, returns non-zero error code, such
 * as EINVAL if the attributes object is invalid. */
int32_t pthread_attr_getschedparam(const pthread_attr_t *attributes, sched_param *param);
/*!\brief Sets the scheduling parameters (priority) of the given attributes.
 *
 * Requests that the runtime set the scheduling parameters (priority) of attributes from param. Returns 0 for success, EINVAL for an
 * invalid attributes object, ENOSYS when multiple schedulers/priority scheduling is not implemented, and ENOTSUP when the value of
 * param isn't supported/allowed. */
int32_t pthread_attr_setschedparam(pthread_attr_t *attributes, const sched_param *param);

/*!\brief The thread obtains its scheduling properties explicitly from its attributes structure. */
#define PTHREAD_EXPLICIT_SCHED 1
/*!\brief The thread inherits its scheduling properties from its parent thread. */
#define PTHREAD_INHERIT_SCHED 0

/*!\brief Returns the inheritsched attribute of the given attributes.
 *
 * On success, returns 0 and places the inheritsched attribute from attributes into inherit.  This attribute specifies where the
 * thread's scheduling properites shall come from, and can be set to PTHREAD_EXPLICIT_SCHED or PTHREAD_INHERIT_SCHED.  On failure it
 * returns EINVAL if attributes was invalid or ENOSYS if multiple schedulers/priority scheduling isn't implemented. */
int32_t pthread_attr_getinheritsched(const pthread_attr_t *attributes, int32_t *inherit);
/*!\brief Sets the inheritsched attribute of the given attributes.
 *
 * On success, places inherit into the inheritsched attribute of attributes and returns 0.  inherit must either contain
 * PTHREAD_EXPLICIT_SCHED or PTHREAD_INHERIT_SCHED.  On failure, this routine returns EINVAL if attributes is invalid, ENOSYS when
 * multiple schedulers/priority scheduling isn't implemented, and ENOSUP if the inheritsched attribute isn't supported. */
int32_t pthread_attr_setinheritsched(pthread_attr_t *attributes, int32_t inherit);

/*!\brief Creates a new POSIX Threads mutex, which will initially be unlocked.
 *
 * Creates a new mutex with the given attributes.  If attributes is NULL, the default attributes will be used.  The mutex starts out
 * unlocked.  On success, the new mutex resides in the mutex structure pointed to by mutex, and this routine routines 0.  On failure,
 * it returns EAGAIN if the system lacked sufficient non-memory resources to initialize the mutex, EBUSY if the given mutex is
 * already initialized and in use, EINVAL if either parameter is invalid, and ENOMEM if the system lacks the memory for a new
 * mutex.  Note: All EDI mutexes are created with the default attributes, and are of type PTHREAD_MUTEX_ERRORCHECK.  This means
 * undefined behavior can never result from an badly placed function call. */
int32_t pthread_mutex_init (pthread_mutex_t *mutex, const pthread_mutex_attr_t *attributes);
/*!\brief Locks the given mutex.
 *
 * Locks the given mutex.  If the mutex is already locked, blocks the calling thread until it can acquire the lock.  When this
 * routine returns successfully, it will return 0 and the calling thread will own the lock of the mutex.  If the call fails, it can
 * return EINVAL when mutex is invalid or EDEADLK if the calling thread already owns the mutex. */
int32_t pthread_mutex_lock(pthread_mutex_t *mutex);
/*!\brief Unlocks the given mutex.
 *
 * Unlocks the given mutex, returning 0 on success.  On failure, it can return EINVAL when mutex is invalid or EPERM when the
 * calling thread doesn't own the mutex. */
int32_t pthread_mutex_unlock(pthread_mutex_t *mutex);
/*!\brief Tries to lock the given mutex, returning immediately even if the mutex is already locked.
 *
 * Attempts to lock the given mutex, but returns immediately if it can't acquire a lock.  Returns 0 when it has acquired a lock,
 * EBUSY if the mutex is already locked, or EINVAL if mutex is invalid. */
int32_t pthread_mutex_trylock(pthread_mutex_t *mutex);
/*!\brief Destroys the given mutex, or at least the internal structure of it. 
 *
 * Deletes the given mutex, making mutex invalid until it should be initialized by pthread_mutex_init().  Returns 0 on success,
 * EINVAL when mutex is invalid, or EBUSY when mutex is locked or referenced by another thread. */
int32_t pthread_mutex_destroy (pthread_mutex_t *mutex);

#endif
