/*
AcessOS C Library
- Environment Handler
*/
#include <stdlib.h>

// === GLOBALS ===
char **_envp = NULL;

// === CODE ===
char *getenv(const char *name)
{
	char	**env;
	char	*str;
	 int	len;
	
	if(!_envp)	return NULL;
	if(!name)	return NULL;
	
	
	len = strlen((char*)name);
	
	env = _envp;
	while(*env) {
		str = *env;
		if(str[len] == '=' && strncmp((char*)name, str, len) == 0) {
			return str+len+1;
		}
		env ++;
	}
	
	return NULL;
}
