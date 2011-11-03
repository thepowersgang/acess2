/*
 * Acess C Library
 * - Environment Handler
*/
#include <stdlib.h>
#include <string.h>

// === GLOBALS ===
char **_envp = NULL;

// === CODE ===
char *getenv(const char *name)
{
	char	**env;
	char	*env_str;
	 int	len;
	
	if(!_envp)	return NULL;
	if(!name)	return NULL;
	
	len = strlen((char*)name);
	
	env = _envp;
	while(*env)
	{
		env_str = *env;
		if(strncmp(name, env_str, len) == 0 && env_str[len] == '=') {
			return env_str+len+1;
		}
		env ++;
	}
	
	return NULL;
}
