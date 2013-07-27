/*
 * Acess C Library
 * - Environment Handler
*/
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// === GLOBALS ===
char	**environ = NULL;	
static char	**expected_environ = NULL;
static int	num_allocated_slots;

// === CODE ===
char *getenv(const char *name)
{
	char	*env_str;
	 int	len;
	
	if(!environ)	return NULL;
	if(!name)	return NULL;
	
	len = strlen((char*)name);
	
	for( char **env = environ; *env; env ++ )
	{
		env_str = *env;
		if(strncmp(name, env_str, len) == 0 && env_str[len] == '=') {
			return env_str+len+1;
		}
	}
	
	return NULL;
}


int putenv(char *string)
{
	char *eqpos = strchr(string, '=');
	if( !eqpos ) {
		errno = EINVAL;
		return -1;
	}
	size_t	namelen = eqpos - string;
	
	static const int	alloc_step = 10;
	if( expected_environ == NULL || expected_environ != environ )
	{
		if( expected_environ )
			free(expected_environ);
		
		 int	envc = 0;
		// Take a copy
		for( char **env = environ; *env; env ++ )
			envc ++;
		envc ++;	// NULL termination
		envc ++;	// assume we're adding a new value
		
		num_allocated_slots = (envc + alloc_step-1) / alloc_step * alloc_step;

		expected_environ = malloc(num_allocated_slots*sizeof(char*));
		if(!expected_environ)
			return 1;
		
		 int	idx = 0;
		for( char **env = environ; *env; env ++ )
			expected_environ[idx++] = *env;
		expected_environ[idx++] = NULL;
		
		environ = expected_environ;
	}
	
	 int	envc = 0;
	for( char **env = environ; *env; env ++, envc++ )
	{
		if( strncmp(*env, string, namelen) != 0 )
			continue ;
		if( *env[namelen] != '=' )
			continue ;
		*env = string;
		return 0;
	}
	if( num_allocated_slots >= envc+1 )
	{
		num_allocated_slots += alloc_step;
		expected_environ = realloc(expected_environ, num_allocated_slots*sizeof(char*));
		if(!expected_environ)
			return 1;
		environ = expected_environ;
	}
	environ[envc] = string;
	environ[envc+1] = NULL;
	
	return 0;
}

