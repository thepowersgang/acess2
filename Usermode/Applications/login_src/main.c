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
	 int	pid, uid;
	 int	status = 0;
	tUserInfo	*uinfo;
	
	putchar('\n');
	for(;;)
	{
		// Validate User
		do {
			sUsername = GetUsername();
			sPassword = GetPassword();
		} while( (uid = ValidateUser(sUsername, sPassword)) == -1 );
		putchar('\n');
		
		// Get user information
		uinfo = GetUserInfo(uid);
		
		// Create child process
		pid = clone(CLONE_VM, 0);
		// Error check
		if(pid == -1) {
			fprintf(stderr, "Unable to fork the login process!\n");
			return -1;
		}
		
		// Spawn shell in a child process
		if(pid == 0)
		{
			char	*argv[2] = {uinfo->Shell, 0};
			char	**envp = NULL;
			setgid(uinfo->GID);
			setuid(uid);
			
			execve(uinfo->Shell, argv, envp);
			exit(-1);
		}
		
		// Wait for child to terminate
		waittid(pid, &status);
	}
	
	return 0;
}

/**
 * \fn char *GetUsername()
 */
char *GetUsername()
{
	char	ret[BUFLEN];
	 int	pos = 0;
	char	ch;
	
	// Prompt the user
	printf("Username: ");
	
	// Read in text
	while( (ch = fgetc(stdin)) != -1 && ch != '\n' )
	{
		if(ch == '\b') {
			pos --;
			ret[pos] = '\0';
		}
		else
			ret[pos++] = ch;
		
		// Echo out to the screen
		fputc(ch, stdout);
		
		if(pos == BUFLEN-1)	break;
	}
	
	// Finish String
	ret[pos] = '\0';
	
	printf("\n");
	return strdup(ret);
}

/**
 * \fn char *GetPassword()
 */
char *GetPassword()
{
	char	ret[BUFLEN];
	 int	pos = 0;
	char	ch;
	
	// Prompt the user
	printf("Password: ");
	
	// Read in text
	while( (ch = fgetc(stdin)) != -1 && ch != '\n' )
	{
		if(ch == '\b') {
			pos --;
			ret[pos] = '\0';
		}
		else
			ret[pos++] = ch;
		
		// Don't echo out to the screen
		//fputc(stdout, ch);
		
		if(pos == BUFLEN-1)	break;
	}
	
	ret[pos] = '\0';
	
	printf("\n");
	return strdup(ret);
}
