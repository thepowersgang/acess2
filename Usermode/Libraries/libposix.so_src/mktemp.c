/*
 * Acess2 POSIX Emulation Layer
 * - By John Hodge
 * 
 * mkstemp.c
 * - mkstemp/mktemp
 */
#include <unistd.h>	// mktemp
#include <stdlib.h>	// mkstemp
#include <stdio.h>
#include <string.h>	// str*
#include <errno.h>

// === CODE ===
int mkstemp(char *template)
{
	size_t	tpl_len = strlen(template);
	if( tpl_len < 6 ) {
		errno = EINVAL;
		return -1;
	}
	if( strcmp(template+tpl_len-6, "XXXXXX") != 0 ) {
		errno = EINVAL;
		return -1;
	}
	
	for( int i = 0; i < 1000000; i ++ )
	{
		snprintf(template+tpl_len-6, 6+1, "%06d", i);
		int fd = open(template, O_EXCL|O_CREAT, 0600);
		if(fd == -1)	continue ;
	
		return fd;
	}
	
	errno = EEXIST;
	template[0] = '\0';
	return -1;
}

char *mktemp(char *template)
{
	 int fd = mkstemp(template);
	if( fd == -1 )
		return NULL;
	close(fd);
	return template;
}


