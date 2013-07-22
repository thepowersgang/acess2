/*
 * AcessNative libacess-native
 * - By John Hodge (thePowersGang)
 *
 * heap.c
 * - Proxies for the C standard heap
 */
#include <stdlib.h>

void *acess_malloc(size_t bytes)
{
	return malloc(bytes);
}

void acess_free(void *ptr)
{
	free(ptr);
}

void *acess_calloc(size_t __nmemb, size_t __size)
{
	return calloc(__nmemb, __size);
}

void *acess_realloc(void *__ptr, size_t __size)
{
	return realloc(__ptr, __size);
}

