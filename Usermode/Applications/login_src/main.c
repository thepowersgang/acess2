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
			sPassword = GetPassword();
			if( (uid = ValidateUser(sUsername, sPassword)) == -1 )
			{
				printf("\nInvalid username or password\n");
				free(sUsername);
				free(sPassword);
			}
			else
				break;
		}
		printf("\n");
		
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
			char	*child_argv[2] = {NULL, 0};
			char	**child_envp = NULL;
			
			// Get user information
			uinfo = GetUserInfo(uid);
			
			child_argv[0] = uinfo->Shell;
			// Set Environment
			setgid(uinfo->GID);
			//setuid(uid);
			setuid(uinfo->UID);
			
			execve(uinfo->Shell, child_argv, child_envp);
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
	char	ret[BUFLEN] = {0};
	 int	pos = 0;
	char	ch;
	
	// Prompt the user
	printf("Username: ");
	
	// Read in text
	while( (ch = fgetc(stdin)) != -1 && ch != '\n' )
	{
		if(ch == '\b') {
			if( pos <= 0 )	continue;
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
			if( pos <= 0 )	continue;
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
