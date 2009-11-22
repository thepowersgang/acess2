/*
 * AcessOS Shell Version 3
 */
#include <acess/sys.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "header.h"

#define _stdin	0
#define _stdout	1
#define _stderr	2

// ==== PROTOTYPES ====
char	*ReadCommandLine(int *Length);
void	Parse_Args(char *str, char **dest);
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
int main(int argc, char *argv[], char *envp[])
{
	char	*sCommandStr;
	char	*saArgs[32] = {0};
	 int	length = 0;
	 int	i;
	 int	iArgCount = 0;
	 int	bCached = 1;
	
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
	
	write(_stdout, 22, "Acess Shell Version 3\n");
	write(_stdout,  2, "\n");
	for(;;)
	{
		// Free last command & arguments
		if(saArgs[0])	free(saArgs);
		if(!bCached)	free(sCommandStr);
		bCached = 0;
		
		write(_stdout, strlen(gsCurrentDirectory), gsCurrentDirectory);
		write(_stdout, 2, "$ ");
		
		// Read Command line
		sCommandStr = ReadCommandLine( &length );
		
		if(!sCommandStr) {
			write(_stdout, 25, "PANIC: Out of heap space\n");
			return -1;
		}
		
		// Check if the command should be cached
		if(gasCommandHistory == NULL || strcmp(sCommandStr, gasCommandHistory[giLastCommand]) != 0)
		{
			if(giLastCommand >= giCommandSpace) {
				giCommandSpace += 12;
				gasCommandHistory = realloc(gasCommandHistory, giCommandSpace*sizeof(char*));
			}
			giLastCommand ++;
			gasCommandHistory[ giLastCommand ] = sCommandStr;
			bCached = 1;
		}
		
		// Parse Command Line into arguments
		Parse_Args(sCommandStr, saArgs);
		
		// Count Arguments
		iArgCount = 0;
		while(saArgs[iArgCount])	iArgCount++;
		
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
	}
}

/**
 * \fn char *ReadCommandLine(int *Length)
 * \brief Read from the command line
 */
char *ReadCommandLine(int *Length)
{
	char	*ret;
	 int	len, pos, space = 1023;
	char	ch;
	#if 0
	 int	scrollbackPos = giLastCommand;
	#endif
	 
	// Preset Variables
	ret = malloc( space+1 );
	if(!ret)	return NULL;
	len = 0;	pos = 0;
		
	// Read In Command Line
	do {
		read(_stdin, 1, &ch);	// Read Character from stdin (read is a blocking call)
		
		if(ch == '\n')	break;
		
		switch(ch)
		{
		// Control characters
		case '\x1B':
			read(_stdin, 1, &ch);	// Read control character
			switch(ch)
			{
			//case 'D':	if(pos)	pos--;	break;
			//case 'C':	if(pos<len)	pos++;	break;
			case '[':
				read(_stdin, 1, &ch);	// Read control character
				switch(ch)
				{
				#if 0
				case 'A':	// Up
					{
						 int	oldLen = len;
						if( scrollbackPos > 0 )	break;
						
						free(ret);
						ret = strdup( gasCommandHistory[--scrollbackPos] );
						
						len = strlen(ret);
						while(pos--)	write(_stdout, 3, "\x1B[D");
						write(_stdout, len, ret);	pos = len;
						while(pos++ < oldLen)	write(_stdout, 1, " ");
					}
					break;
				case 'B':	// Down
					{
						 int	oldLen = len;
						if( scrollbackPos < giLastCommand-1 )	break;
						
						free(ret);
						ret = strdup( gasCommandHistory[++scrollbackPos] );
						
						len = strlen(ret);
						while(pos--)	write(_stdout, 3, "\x1B[D");
						write(_stdout, len, ret);	pos = len;
						while(pos++ < oldLen)	write(_stdout, 1, " ");
					}
					break;
				#endif
				case 'D':	// Left
					if(pos == 0)	break;
					pos --;
					write(_stdout, 3, "\x1B[D");
					break;
				case 'C':	// Right
					if(pos == len)	break;
					pos++;
					write(_stdout, 3, "\x1B[C");
					break;
				}
			}
			break;
		
		// Backspace
		case '\b':
			if(len <= 0)		break;	// Protect against underflows
			write(_stdout, 1, &ch);
			if(pos == len) {	// Simple case of end of string
				len --;
				pos--;
			}
			else {
				char	buf[7] = "\x1B[000D";
				buf[2] += ((len-pos+1)/100) % 10;
				buf[3] += ((len-pos+1)/10) % 10;
				buf[4] += (len-pos+1) % 10;
				write(_stdout, len-pos, &ret[pos]);	// Move Text
				ch = ' ';	write(_stdout, 1, &ch);	ch = '\b';	// Clear deleted character
				write(_stdout, 7, buf);	// Update Cursor
				// Alter Buffer
				memmove(&ret[pos-1], &ret[pos], len-pos);
				pos --;
				len --;
			}
			break;
		
		// Tab
		case '\t':
			//TODO: Implement Tab-Completion
			//Currently just ignore tabs
			break;
		
		default:		
			// Expand Buffer
			if(len+1 > space) {
				space += 256;
				ret = realloc(ret, space+1);
				if(!ret)	return NULL;
			}
			
			// Editing inside the buffer
			if(pos != len) {
				char	buf[7] = "\x1B[000D";
				buf[2] += ((len-pos)/100) % 10;
				buf[3] += ((len-pos)/10) % 10;
				buf[4] += (len-pos) % 10;
				write(_stdout, 1, &ch);	// Print new character
				write(_stdout, len-pos, &ret[pos]);	// Move Text
				write(_stdout, 7, buf);	// Update Cursor
				memmove( &ret[pos+1], &ret[pos], len-pos );
			}
			else {
				write(_stdout, 1, &ch);
			}
			ret[pos++] = ch;
			len ++;
			break;
		}
	} while(ch != '\n');
	
	// Cap String
	ret[len] = '\0';
	printf("\n");
	
	// Return length
	if(Length)	*Length = len;
	
	return ret;
}

/**
 * \fn void Parse_Args(char *str, char **dest)
 * \brief Parse a string into an argument array
 */
void Parse_Args(char *str, char **dest)
{
	 int	i = 1;
	char	*buf = malloc( strlen(str) + 1 );
	
	if(buf == NULL) {
		dest[0] = NULL;
		Print("Parse_Args: Out of heap space!\n");
		return ;
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
	if(i == 1) {
		free(buf);
		dest[0] = NULL;
	}
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
			Print("Unknown Command: `");Print(Args[0]);Print("'\n");	// Error Message
			return ;
		}
		
		// Get File info and close file
		finfo( fd, &info, 0 );
		close( fd );
		
		// Check if the file is a directory
		if(info.flags & FILEFLAG_DIRECTORY) {
			Print("`");Print(sTmpBuffer);	// Error Message
			Print("' is a directory.\n");
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
			Print("Unknown Command: `");Print(exefile);Print("'\n");
			return ;
		}
	}
	
	// Create new process
	pid = clone(CLONE_VM, 0);
	// Start Task
	if(pid == 0)
		execve(sTmpBuffer, Args, gasEnvironment);
	if(pid <= 0) {
		Print("Unablt to create process: `");Print(sTmpBuffer);Print("'\n");	// Error Message
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
	Print("Acess 2 Command Line Interface\n");
	Print(" By John Hodge (thePowersGang / [TPG])\n");
	Print("\n");
	Print("Builtin Commands:\n");
	Print(" logout: Return to the login prompt\n");
	Print(" exit:   Same\n");
	Print(" help:   Display this message\n");
	Print(" clear:  Clear the screen\n");
	Print(" cd:     Change the current directory\n");
	Print(" dir:    Print the contents of the current directory\n");
	//Print("\n");
	return;
}

/**
 * \fn void Command_Clear(int argc, char **argv)
 * \brief Clear the screen
 */
void Command_Clear(int argc, char **argv)
{
	write(_stdout, 4, "\x1B[2J");	//Clear Screen
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
		Print(gsCurrentDirectory);Print("\n");
		return;
	}
	
	GeneratePath(argv[1], gsCurrentDirectory, tmpPath);
	
	fp = open(tmpPath, 0);
	if(fp == -1) {
		write(_stdout, 26, "Directory does not exist\n");
		return;
	}
	finfo(fp, &stats, 0);
	close(fp);
	
	if( !(stats.flags & FILEFLAG_DIRECTORY) ) {
		write(_stdout, 17, "Not a Directory\n");
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
		write(_stdout, 34, "stat Failed, Bad File Descriptor\n");
		return;
	}
	// Check if it's a directory
	if(!(info.flags & FILEFLAG_DIRECTORY))
	{
		close(dp);
		write(_stdout, 27, "Unable to open directory `");
		write(_stdout, strlen(tmpPath)+1, tmpPath);
		write(_stdout, 20, "', Not a directory\n");
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
				write(_stdout, 42, "Invalid Permissions to traverse directory\n");
			break;
		}
		// Open File
		fp = open(tmpPath, 0);
		if(fp == -1)	continue;
		// Get File Stats
		finfo(fp, &info, 0);
		
		if(info.flags & FILEFLAG_DIRECTORY)
			write(_stdout, 1, "d");
		else
			write(_stdout, 1, "-");
		
		// Print Mode
		// - Owner
		acl.group = 0;	acl.id = info.uid;
		_SysGetACL(fp, &acl);
		if(acl.perms & 1)	modeStr[0] = 'r';	else	modeStr[0] = '-';
		if(acl.perms & 2)	modeStr[1] = 'w';	else	modeStr[1] = '-';
		if(acl.perms & 8)	modeStr[2] = 'x';	else	modeStr[2] = '-';
		// - Group
		acl.group = 1;	acl.id = info.gid;
		_SysGetACL(fp, &acl);
		if(acl.perms & 1)	modeStr[3] = 'r';	else	modeStr[3] = '-';
		if(acl.perms & 2)	modeStr[4] = 'w';	else	modeStr[4] = '-';
		if(acl.perms & 8)	modeStr[5] = 'x';	else	modeStr[5] = '-';
		// - World
		acl.group = 1;	acl.id = -1;
		_SysGetACL(fp, &acl);
		if(acl.perms & 1)	modeStr[6] = 'r';	else	modeStr[6] = '-';
		if(acl.perms & 2)	modeStr[7] = 'w';	else	modeStr[7] = '-';
		if(acl.perms & 8)	modeStr[8] = 'x';	else	modeStr[8] = '-';
		write(_stdout, 10, modeStr);
		close(fp);
		
		// Colour Code
		if(info.flags & FILEFLAG_DIRECTORY)	// Directory: Green
			write(_stdout, 6, "\x1B[32m");
		// Default: White
		
		// Print Name
		write(_stdout, strlen(fileName), fileName);
		
		// Print slash if applicable
		if(info.flags & FILEFLAG_DIRECTORY)
			write(_stdout, 1, "/");
		
		// Revert Colour
		write(_stdout, 6, "\x1B[37m");
		
		// Newline!
		write(_stdout, 1, "\n");
	}
	// Close Directory
	close(dp);
}
