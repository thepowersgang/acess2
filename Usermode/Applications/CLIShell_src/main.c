/*
 * AcessOS Shell Version 3
 */
#define USE_READLINE	1
#include <acess/sys.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "header.h"
#include <readline.h>

#define _stdin	0
#define _stdout	1
#define _stderr	2

// ==== PROTOTYPES ====
 int	Parse_Args(const char *str, char **dest);
void	CallCommand(char **Args);
void	Command_Logout(int argc, char **argv);
void	Command_Clear(int argc, char **argv);
void	Command_Help(int argc, char **argv);
void	Command_Cd(int argc, char **argv);
void	Command_Dir(int argc, char **argv);

// ==== CONSTANT GLOBALS ====
struct	{
	char	*name;
	void	(*fcn)(int argc, char **argv);
}	cBUILTINS[] = {
	{"exit", Command_Logout},	{"logout", Command_Logout},
	{"help", Command_Help}, {"clear", Command_Clear},
	{"cd", Command_Cd}, {"dir", Command_Dir}
};
static char	*cDEFAULT_PATH[] = {"/Acess/Bin"};
#define	BUILTIN_COUNT	(sizeof(cBUILTINS)/sizeof(cBUILTINS[0]))

// ==== LOCAL VARIABLES ====
 int	giNumPathDirs = 1;
char	**gasPathDirs = cDEFAULT_PATH;
char	**gasEnvironment;
char	gsCommandBuffer[1024];
char	*gsCurrentDirectory = NULL;
char	**gasCommandHistory;
 int	giLastCommand = 0;
 int	giCommandSpace = 0;

// ==== CODE ====
int main(int argc, char *argv[], char **envp)
{
	char	*sCommandStr;
	char	*saArgs[32] = {0};
	 int	i;
	 int	iArgCount = 0;
	tReadline	*readline_state = Readline_Init(1);
	
	gasEnvironment = envp;
	
	Command_Clear(0, NULL);
	
	{
		char	*tmp = getenv("CWD");
		if(tmp) {
			gsCurrentDirectory = malloc(strlen(tmp)+1);
			strcpy(gsCurrentDirectory, tmp);
		} else {
			gsCurrentDirectory = malloc(2);
			strcpy(gsCurrentDirectory, "/");
		}
	}	
	
	printf("Acess Shell Version 3\n\n");
	for(;;)
	{
		// Free last command & arguments
		if(saArgs[0])	free(saArgs[0]);
		
		printf("%s$ ", gsCurrentDirectory);
		
		// Read Command line
		sCommandStr = Readline( readline_state );
		printf("\n");
		
		// Parse Command Line into arguments
		iArgCount = Parse_Args(sCommandStr, saArgs);
		
		// Silently Ignore all empty commands
		if(saArgs[1][0] == '\0')	continue;
		
		// Check Built-In Commands
		for( i = 0; i < BUILTIN_COUNT; i++ )
		{
			if( strcmp(saArgs[1], cBUILTINS[i].name) == 0 )
			{
				cBUILTINS[i].fcn(iArgCount-1, &saArgs[1]);
				break;
			}
		}
		if(i != BUILTIN_COUNT)	continue;
		
		// Shall we?
		CallCommand( &saArgs[1] );
		
		free( sCommandStr );
	}
}

/**
 * \fn int Parse_Args(const char *str, char **dest)
 * \brief Parse a string into an argument array
 */
int Parse_Args(const char *str, char **dest)
{
	 int	i = 1;
	char	*buf = malloc( strlen(str) + 1 );
	
	if(buf == NULL) {
		dest[0] = NULL;
		printf("Parse_Args: Out of heap space!\n");
		return 0;
	}
	
	strcpy(buf, str);
	dest[0] = buf;
	
	// Trim leading whitespace
	while(*buf == ' ' && *buf)	buf++;
	
	for(;;)
	{
		dest[i] = buf;	// Save start of string
		i++;
		while(*buf && *buf != ' ')
		{
			if(*buf++ == '"')
			{
				while(*buf && !(*buf == '"' && buf[-1] != '\\'))
					buf++;
			}
		}
		if(*buf == '\0')	break;
		*buf = '\0';
		while(*++buf == ' ' && *buf);
		if(*buf == '\0')	break;
	}
	dest[i] = NULL;
	
	return i;
}

/**
 * \fn void CallCommand(char **Args)
 */
void CallCommand(char **Args)
{
	t_sysFInfo	info;
	 int	pid = -1;
	 int	fd = 0;
	char	sTmpBuffer[1024];
	char	*exefile = Args[0];
	
	if(exefile[0] == '/'
	|| (exefile[0] == '.' && exefile[1] == '/')
	|| (exefile[0] == '.' && exefile[1] == '.' && exefile[2] == '/')
		)
	{
		GeneratePath(exefile, gsCurrentDirectory, sTmpBuffer);
		// Check file existence
		fd = open(sTmpBuffer, OPENFLAG_EXEC);
		if(fd == -1) {
			printf("Unknown Command: `%s'\n", Args[0]);	// Error Message
			return ;
		}
		
		// Get File info and close file
		finfo( fd, &info, 0 );
		close( fd );
		
		// Check if the file is a directory
		if(info.flags & FILEFLAG_DIRECTORY) {
			printf("`%s' is a directory.\n", sTmpBuffer);
			return ;
		}
	}
	else
	{
		 int	i;
		
		// Check all components of $PATH
		for( i = 0; i < giNumPathDirs; i++ )
		{
			GeneratePath(exefile, gasPathDirs[i], sTmpBuffer);
			fd = open(sTmpBuffer, OPENFLAG_EXEC);
			if(fd == -1)	continue;
			finfo( fd, &info, 0 );
			close( fd );
			if(info.flags & FILEFLAG_DIRECTORY)	continue;
			// Woohoo! We found a valid command
			break;
		}
		
		// Exhausted path directories
		if( i == giNumPathDirs ) {
			printf("Unknown Command: `%s'\n", exefile);
			return ;
		}
	}
	
	// Create new process
	pid = clone(CLONE_VM, 0);
	// Start Task
	if(pid == 0) {
		execve(sTmpBuffer, Args, gasEnvironment);
		printf("Execve returned, ... oops\n");
		exit(-1);
	}
	if(pid <= 0) {
		printf("Unable to create process: `%s'\n", sTmpBuffer);	// Error Message
	}
	else {
		 int	status;
		waittid(pid, &status);
	}
}

/**
 * \fn void Command_Logout(int argc, char **argv)
 * \brief Exit the shell, logging the user out
 */
void Command_Logout(int argc, char **argv)
{
	exit(0);
}

/**
 * \fn void Command_Colour(int argc, char **argv)
 * \brief Displays the help screen
 */
void Command_Help(int argc, char **argv)
{
	printf( "Acess 2 Command Line Interface\n"
		" By John Hodge (thePowersGang / [TPG])\n"
		"\n"
		"Builtin Commands:\n"
		" logout: Return to the login prompt\n"
		" exit:   Same\n"
		" help:   Display this message\n"
		" clear:  Clear the screen\n"
		" cd:     Change the current directory\n"
		" dir:    Print the contents of the current directory\n");
	return;
}

/**
 * \fn void Command_Clear(int argc, char **argv)
 * \brief Clear the screen
 */
void Command_Clear(int argc, char **argv)
{
	write(_stdout, "\x1B[2J", 4);	//Clear Screen
}

/**
 * \fn void Command_Cd(int argc, char **argv)
 * \brief Change directory
 */
void Command_Cd(int argc, char **argv)
{
	char	tmpPath[1024];
	int		fp;
	t_sysFInfo	stats;
	
	if(argc < 2)
	{
		printf("%s\n", gsCurrentDirectory);
		return;
	}
	
	GeneratePath(argv[1], gsCurrentDirectory, tmpPath);
	
	fp = open(tmpPath, 0);
	if(fp == -1) {
		printf("Directory does not exist\n");
		return;
	}
	finfo(fp, &stats, 0);
	close(fp);
	
	if( !(stats.flags & FILEFLAG_DIRECTORY) ) {
		printf("Not a Directory\n");
		return;
	}
	
	free(gsCurrentDirectory);
	gsCurrentDirectory = malloc(strlen(tmpPath)+1);
	strcpy(gsCurrentDirectory, tmpPath);
	
	// Register change with kernel
	chdir( gsCurrentDirectory );
}

/**
 * \fn void Command_Dir(int argc, char **argv)
 * \brief Print the contents of a directory
 */
void Command_Dir(int argc, char **argv)
{
	 int	dp, fp, dirLen;
	char	modeStr[11] = "RWXrwxRWX ";
	char	tmpPath[1024];
	char	*fileName;
	t_sysFInfo	info;
	t_sysACL	acl;
	
	// Generate Directory Path
	if(argc > 1)
		dirLen = GeneratePath(argv[1], gsCurrentDirectory, tmpPath);
	else
	{
		strcpy(tmpPath, gsCurrentDirectory);
	}
	dirLen = strlen(tmpPath);
	
	// Open Directory
	dp = open(tmpPath, OPENFLAG_READ);
	// Check if file opened
	if(dp == -1)
	{
		printf("Unable to open directory `%s', File cannot be found\n", tmpPath);
		return;
	}
	// Get File Stats
	if( finfo(dp, &info, 0) == -1 )
	{
		close(dp);
		printf("stat Failed, Bad File Descriptor\n");
		return;
	}
	// Check if it's a directory
	if(!(info.flags & FILEFLAG_DIRECTORY))
	{
		close(dp);
		printf("Unable to open directory `%s', Not a directory\n", tmpPath);
		return;
	}
	
	// Append Shash for file paths
	if(tmpPath[dirLen-1] != '/')
	{
		tmpPath[dirLen++] = '/';
		tmpPath[dirLen] = '\0';
	}
	
	fileName = (char*)(tmpPath+dirLen);
	// Read Directory Content
	while( (fp = readdir(dp, fileName)) )
	{
		if(fp < 0)
		{
			if(fp == -3)
				printf("Invalid Permissions to traverse directory\n");
			break;
		}
		// Open File
		fp = open(tmpPath, 0);
		if(fp == -1)	continue;
		// Get File Stats
		finfo(fp, &info, 0);
		
		if(info.flags & FILEFLAG_DIRECTORY)
			printf("d");
		else
			printf("-");
		
		// Print Mode
		// - Owner
		acl.object = info.uid;
		_SysGetACL(fp, &acl);
		if(acl.perms & 1)	modeStr[0] = 'r';	else	modeStr[0] = '-';
		if(acl.perms & 2)	modeStr[1] = 'w';	else	modeStr[1] = '-';
		if(acl.perms & 8)	modeStr[2] = 'x';	else	modeStr[2] = '-';
		// - Group
		acl.object = info.gid | 0x80000000;
		_SysGetACL(fp, &acl);
		if(acl.perms & 1)	modeStr[3] = 'r';	else	modeStr[3] = '-';
		if(acl.perms & 2)	modeStr[4] = 'w';	else	modeStr[4] = '-';
		if(acl.perms & 8)	modeStr[5] = 'x';	else	modeStr[5] = '-';
		// - World
		acl.object = 0xFFFFFFFF;
		_SysGetACL(fp, &acl);
		if(acl.perms & 1)	modeStr[6] = 'r';	else	modeStr[6] = '-';
		if(acl.perms & 2)	modeStr[7] = 'w';	else	modeStr[7] = '-';
		if(acl.perms & 8)	modeStr[8] = 'x';	else	modeStr[8] = '-';
		printf(modeStr);
		close(fp);
		
		// Colour Code
		if(info.flags & FILEFLAG_DIRECTORY)	// Directory: Green
			printf("\x1B[32m");
		// Default: White
		
		// Print Name
		printf("%s", fileName);
		
		// Print slash if applicable
		if(info.flags & FILEFLAG_DIRECTORY)
			printf("/");
		
		// Revert Colour
		printf("\x1B[37m");
		
		// Newline!
		printf("\n");
	}
	// Close Directory
	close(dp);
}
