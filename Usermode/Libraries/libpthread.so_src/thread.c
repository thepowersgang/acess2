/*
 * Acess2 libpthread
 * - By John Hodge (thePowersGang)
 *
 * thread.c
 * - Thread management for pthreads
 */
#include <pthread.h>
#include <assert.h>

int pthread_create(pthread_t *threadptr, const pthread_attr_t * attrs, void* (*fcn)(void*), void* arg)
{
	assert(!"TODO: pthread_create");
	return 0;
}
int pthread_detach(pthread_t thread)
{
	assert(!"TODO: pthread_detach");
	return 0;
}
int pthread_join(pthread_t thread, void **retvalptr)
{
	assert(!"TODO: pthread_join");
	return 0;
}
int pthread_cancel(pthread_t thread)
{
	assert(!"TODO: pthread_cancel");
	return 0;
}
int pthread_equal(pthread_t t1, pthread_t t2)
{
	assert(!"TODO: pthread_equal");
	return 0;
}
pthread_t pthread_self(void)
{
	assert(!"TODO: pthread_self");
	return (pthread_t){0};
}
void pthread_exit(void* retval)
{
	assert(!"TODO: pthread_create");
}
