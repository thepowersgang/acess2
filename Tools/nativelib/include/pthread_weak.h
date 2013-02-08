/*
 * Acess2 libnative (Kernel Simulation Library)
 * - By John Hodge (thePowersGang)
 *
 * pthread_weak.h
 * - Weakly linked copies of pthread_* and sem_*
 */
#ifndef _PTHREAD_WEAK_H
#define _PTHREAD_WEAK_H
#include <pthread.h>
#include <semaphore.h>

extern int pthread_create (pthread_t*, const pthread_attr_t*, void* (*)(void*), void*) __attribute__ ((weak));
extern int pthread_mutex_init (pthread_mutex_t*, const pthread_mutexattr_t*) __attribute__ ((weak));
extern int pthread_mutex_lock (pthread_mutex_t*) __attribute__ ((weak));
extern int pthread_mutex_unlock (pthread_mutex_t*) __attribute__ ((weak));
extern int pthread_mutex_destroy (pthread_mutex_t*) __attribute__ ((weak));

extern int sem_init(sem_t *sem, int pshared, unsigned int value) __attribute__ ((weak));
extern int sem_wait(sem_t *sem) __attribute__ ((weak));
extern int sem_trywait(sem_t *sem) __attribute__ ((weak));
extern int sem_post(sem_t *sem) __attribute__ ((weak));
extern int sem_getvalue(sem_t *sem, int *sval) __attribute__ ((weak));

#endif
