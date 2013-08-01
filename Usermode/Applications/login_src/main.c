/*
 * Acess 2 Login
 * - By John Hodge (thePowersGang)
 */
#include "header.h"
#include <acess/devices/pty.h>	// Enable/disable echo

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
			if(!sUsername || !sUsername[0])	continue;
			sPassword = GetPassword();
			if(!sPassword) {
				free(sUsername);
				continue;
			}
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
	
		// Clear graphics mode	
		struct ptymode	mode = {.InputMode = PTYIMODE_ECHO|PTYIMODE_CANON,.OutputMode=0};
		_SysIOCtl(0, PTY_IOCTL_SETMODE, &mode);
		fprintf(stderr, "\x1b[R");
	}
	
	return 0;
}

char *_GetString(int bEcho)
{
	char	ret[BUFLEN];
	 int	pos = 0;
	char	ch;
	
	struct ptymode	mode;
	const int	is_pty = (_SysIOCtl(0, DRV_IOCTL_TYPE, NULL) == DRV_TYPE_TERMINAL);

	// Clear PTY echo
	if( !bEcho && is_pty ) {
		_SysIOCtl(0, PTY_IOCTL_GETMODE, &mode);
		mode.InputMode &= ~PTYIMODE_ECHO;
		_SysIOCtl(0, PTY_IOCTL_SETMODE, &mode);
	}
	
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
		
		if(pos == BUFLEN-1)	break;
	}
	
	ret[pos] = '\0';

	// Re-set echo
	if( !bEcho && is_pty ) {
		mode.InputMode |= PTYIMODE_ECHO;
		_SysIOCtl(0, PTY_IOCTL_SETMODE, &mode);
	}

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
