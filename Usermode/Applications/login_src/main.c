/*
 * Acess 2 Login
 */
#include "header.h"

// === CONSTANTS ===
#define BUFLEN	1024

// === PROTOTYPES ===
char	*GetUsername();
char	*GetPassword();

// === CODE ===
int main(int argc, char *argv[])
{
	char	*sUsername, *sPassword;
	 int	pid, uid = 0;
	 int	status = 0;
	tUserInfo	*uinfo;

	printf("\x1B[?25h");	// Re-enable the cursor	
//	printf("\x1B[2J");	// Clear Screen

	for(;;)
	{
		// Validate User
		for(;;)
		{
			sUsername = GetUsername();
			if(!sUsername)	continue;
			sPassword = GetPassword();
			if(!sPassword)	continue;
			if( (uid = ValidateUser(sUsername, sPassword)) == -1 )
			{
				printf("\nInvalid username or password\n");
				_SysDebug("Auth failure: '%s':'%s'", sUsername, sPassword);
				free(sUsername);
				free(sPassword);
			}
			else
				break;
		}
		printf("\n");

		uinfo = GetUserInfo(uid);
		struct s_sys_spawninfo	spawninfo;
		spawninfo.flags = 0;
		spawninfo.gid = uinfo->GID;
		spawninfo.uid = uinfo->UID;
		const char	*child_argv[2] = {"-", 0};
		const char	**child_envp = NULL;
		 int	fds[] = {0, 1, 2};
		pid = _SysSpawn(uinfo->Shell, child_argv, child_envp, 3, fds, &spawninfo);
		// Error check
		if(pid == -1) {
			fprintf(stderr, "Unable to fork the login process!\n");
			return -1;
		}
		
		// Wait for child to terminate
		_SysWaitTID(pid, &status);
	}
	
	return 0;
}

char *_GetString(int bEcho)
{
	char	ret[BUFLEN];
	 int	pos = 0;
	char	ch;
	
	// Read in text
	while( (ch = fgetc(stdin)) != -1 && ch != '\n' )
	{
		// Handle backspace
		if(ch == '\b') {
			if( pos <= 0 )	continue;
			pos --;
			ret[pos] = '\0';
		}
		// Ctrl-C : Cancel
		else if( ch == 'c'-'a'+1)
			pos = 0;
		// Ctrl-U : Clear
		else if( ch == 'u'-'a'+1)
			pos = 0;
		// Ignore \r
		else if( ch == '\r' )
			continue;
		else
			ret[pos++] = ch;
		
		// Don't echo out to the screen
		if( bEcho ) {
			fputc(ch, stdout);
			fflush(stdout);
		}
		
		if(pos == BUFLEN-1)	break;
	}
	
	ret[pos] = '\0';
	
	printf("\n");
	return strdup(ret);
}

/**
 * \fn char *GetUsername()
 */
char *GetUsername()
{
	char	ret[BUFLEN] = {0};
	 int	pos = 0;
	char	ch;
	
	// Prompt the user
	printf("Username: ");
	fflush(stdout);
	
	return _GetString(1);
}

/**
 * \fn char *GetPassword()
 */
char *GetPassword()
{
	// Prompt the user
	printf("Password: ");
	fflush(stdout);
	
	return _GetString(0);
}
