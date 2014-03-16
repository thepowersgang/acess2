/*
 */
#include <nettest.h>
#include <stdio.h>
#include <string.h>

// === CODE ===
void NetTest_Suite_Cmdline(void)
{
	char line[BUFSIZ];
	while( fgets(line, sizeof(line)-1, stdin) )
	{
		const char *cmd = strtok(line, " ");
		if(!cmd)
			continue;
		if( strcmp(cmd, "exit") == 0 ) {
			return ;
		}
		else if( strcmp(cmd, "request") == 0 ) {
			//const char *addr = strtok(NULL, " ");
			
		}
		else {
			fprintf(stderr, "ERROR: Unknown command '%s'\n", cmd);
		}
	}
}
