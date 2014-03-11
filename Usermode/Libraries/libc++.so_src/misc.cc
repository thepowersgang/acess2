/*
 * Acess2 C++ Library
 * - By John Hodge (thePowersGang)
 *
 * misc.cc
 * - Miscelanious functions
 */
#include <string.h>

extern "C" int SoMain()
{
	// nope
	return 0;
}

extern "C" void __cxa_pure_virtual()
{
	// dunno
}

extern "C" void __gxx_personality_v0()
{
	// TODO: Handle __gxx_personality_v0 somehow
}


// DSO Support 
#define MAX_ATEXIT	32
static struct {
	void (*destructor) (void *);
	void	*arg;
	void	*dso_handle;
} __cxa_atexit_funcs[MAX_ATEXIT];
static int	__cxa_atexit_func_count;

extern "C" int __cxa_atexit(void (*destructor) (void *), void *arg, void *dso_handle)
{
	if( __cxa_atexit_func_count == MAX_ATEXIT )
	{
		return 1;
	}
	
	__cxa_atexit_funcs[__cxa_atexit_func_count].destructor = destructor;
	__cxa_atexit_funcs[__cxa_atexit_func_count].arg = arg;
	__cxa_atexit_funcs[__cxa_atexit_func_count].dso_handle = dso_handle;
	__cxa_atexit_func_count ++;
	return 0;
}

extern "C" void __cxa_finalize(void *f)
{
	if( f == 0 )
	{
		for( int i = __cxa_atexit_func_count; i --; )
		{
			if( __cxa_atexit_funcs[i].dso_handle == f )
			{
				__cxa_atexit_funcs[i].destructor(__cxa_atexit_funcs[i].arg);
				memmove(
					&__cxa_atexit_funcs[i],
					&__cxa_atexit_funcs[i+1],
					(__cxa_atexit_func_count-i)*sizeof(__cxa_atexit_funcs[0])
					);
				i ++;
				__cxa_atexit_func_count --;
			}
		}
	}
	else
	{
		for( int i = __cxa_atexit_func_count; i --; )
		{
			__cxa_atexit_funcs[i].destructor(__cxa_atexit_funcs[i].arg);
		}
		__cxa_atexit_func_count = 0;
	}
}

