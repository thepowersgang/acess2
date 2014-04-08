/*
 */
#include <nettest.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmdline.h"

// === CODE ===
void NetTest_Suite_Cmdline(void)
{
	char line[BUFSIZ];
	Cmdline_Backend_StartThread();
	while( fgets(line, sizeof(line)-1, stdin) )
	{
		const char *const sep = " \n\r";
		const char *cmd = strtok(line, sep);
		if(!cmd)
			continue;
		if( strcmp(cmd, "exit") == 0 ) {
			return ;
		}
		else if( strcmp(cmd, "tcp_echo_server") == 0 ) {
			const char *port_str = strtok(NULL, sep);
			char *end;
			int port = strtol(port_str, &end, 0);
			if(*end != '\0') {
				fprintf(stderr, "ERROR: Port number '%s' not valid\n", port_str);
				continue ;
			}
			
			Cmdline_Backend_StartEchoServer(port);
			// TODO: Allow stopping of the server?
		}
		else {
			fprintf(stderr, "ERROR: Unknown command '%s'\n", cmd);
		}
	}
	
	// TODO: Tear down backend?
}
