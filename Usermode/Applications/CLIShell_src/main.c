/*
 * AcessOS Shell Version 3
 */
#include <acess/sys.h>
#include <stdlib.h>
#include <stdio.h>
#include "header.h"

#define _stdin	0
#define _stdout	1
#define _stderr	2

// ==== PROTOTYPES ====
char	*ReadCommandLine(int *Length);
void	Parse_Args(char *str, char **dest);
void	Command_Colour(int argc, char **argv);
void	Command_Clear(int argc, char **argv);
void	Command_Cd(int argc, char **argv);
void	Command_Dir(int argc, char **argv);

// ==== CONSTANT GLOBALS ====
char	*cCOLOUR_NAMES[8] = {"black", "red", "green", "yellow", "blue", "magenta", "cyan", "white"};
struct	{
	char	*name;
	void	(*fcn)(int argc, char **argv);
}	cBUILTINS[] = {
	{"colour", Command_Colour}, {"clear", Command_Clear},
	{"cd", Command_Cd}, {"dir", Command_Dir}
};
#define	BUILTIN_COUNT	(sizeof(cBUILTINS)/sizeof(cBUILTINS[0]))

// ==== LOCAL VARIABLES ====
char	gsCommandBuffer[1024];
char	*gsCurrentDirectory = NULL;
char	gsTmpBuffer[1024];
char	**gasCommandHistory;
 int	giLastCommand = 0;
 int	giCommandSpace = 0;

// ==== CODE ====
int main(int argc, char *argv[], char *envp[])
{
	char	*sCommandStr;
	char	*saArgs[32];
	 int	length = 0;
	 int	pid = -1;
	 int	iArgCount = 0;
	 int	bCached = 1;
	t_sysFInfo	info;
	
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
	
	write(_stdout, 1, "\n");
	write(_stdout, 22, "Acess Shell Version 3\n");
	write(_stdout,  2, "\n");
	for(;;)
	{
		// Free last command & arguments
		if(saArgs[0])	free(saArgs);
		if(!bCached)	free(sCommandStr);
		bCached = 0;
		write(_stdout, strlen(gsCurrentDirectory), gsCurrentDirectory);
		write(_stdout, 3, "$ ");
		
		// Read Command line
		sCommandStr = ReadCommandLine( &length );
		
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
		//  [HACK] Mem Usage - Use Length in place of `i'
		for(length=0;length<BUILTIN_COUNT;length++)
		{
			if(strcmp(saArgs[1], cBUILTINS[length].name) == 0)
			{
				cBUILTINS[length].fcn(iArgCount-1, &saArgs[1]);
				break;
			}
		}
		
		if(length != BUILTIN_COUNT)	continue;
		
		// - Calling a file
		GeneratePath(saArgs[1], gsCurrentDirectory, gsTmpBuffer);
		// Use length in place of fp
		length = open(gsTmpBuffer, 0);
		// Check file existence
		if(length == -1) {
			Print("Unknown Command: `");Print(saArgs[1]);Print("'\n");	// Error Message
			continue;
		}
		// Check if the file is a directory
		finfo( length, &info, 0 );
		close( length );
		if(info.flags & FILEFLAG_DIRECTORY) {
			Print("`");Print(saArgs[1]);	// Error Message
			Print("' is a directory.\n");
			continue;
		}
		// Load new executable
		pid = clone(CLONE_VM, 0);
		printf("pid = %i\n", pid);
		if(pid == 0)	execve(gsTmpBuffer, &saArgs[1], envp);
		if(pid <= 0) {
			Print("Unablt to create process: `");Print(gsTmpBuffer);Print("'\n");	// Error Message
			//SysDebug("pid = %i\n", pid);
		}
		else {
			 int	status;
			waittid(pid, &status);
		}
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
	 
	// Preset Variables
	ret = malloc( space+1 );
	if(!ret)	return NULL;
	len = 0;
	pos = 0;
		
	// Read In Command Line
	do {
		read(_stdin, 1, &ch);	// Read Character from stdin (read is a blocking call)
		// Ignore control characters
		if(ch == '\x1B') {
			read(_stdin, 1, &ch);	// Read control character
			switch(ch)
			{
			case 'D':	if(pos)	pos--;	break;
			case 'C':	if(pos<len)	pos++;	break;
			case '[':
				read(_stdin, 1, &ch);	// Read control character
				switch(ch)
				{
				case 'D':	if(pos)	pos--;	break;
				case 'C':	if(pos<len)	pos++;	break;
				}
			}
			continue;
		}
		// Backspace
		if(ch == '\b') {
			if(len <= 0)		continue;	// Protect against underflows
			if(pos == len) {	// Simple case of end of string
				len --;	pos--;
			} else {
				memmove(&ret[pos-1], &ret[pos], len-pos);
				pos --;
				len --;
			}
			write(_stdout, 1, &ch);
			continue;
		}
		// Expand Buffer
		if(len > space) {
			space += 256;
			ret = realloc(ret, space+1);
			if(!ret)	return NULL;
		}
		
		write(_stdout, 1, &ch);
		ret[pos++] = ch;
		len ++;
	} while(ch != '\n');
	
	// Remove newline
	pos --;
	ret[pos] = '\0';
	
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
 * \fn void Command_Colour(int argc, char **argv)
 * \brief 
 */
void Command_Colour(int argc, char **argv)
{
	int fg, bg;
	char	clrStr[6] = "\x1B[37m";
	
	// Verify Arg Count
	if(argc < 2)
	{
		Print("Please specify a colour\n");
		goto usage;
	}
	
	// Check Colour
	for(fg=0;fg<8;fg++)
		if(strcmp(cCOLOUR_NAMES[fg], argv[1]) == 0)
			break;

	// Foreground a valid colour
	if(fg == 8) {
		Print("Unknown Colour '");Print(argv[1]);Print("'\n");
		goto usage;
	}
	// Set Foreground
	clrStr[3] = '0' + fg;
	write(_stdout, 6, clrStr);
	
	// Need to Set Background?
	if(argc > 2)
	{
		for(bg=0;bg<8;bg++)
			if(strcmp(cCOLOUR_NAMES[bg], argv[2]) == 0)
				break;
	
		// Valid colour
		if(bg == 8)
		{
			Print("Unknown Colour '");Print(argv[2]);Print("'\n");
			goto usage;
		}
	
		clrStr[2] = '4';
		clrStr[3] = '0' + bg;
		write(_stdout, 6, clrStr);
	}
	// Return
	return;

	// Function Usage (Requested via a Goto (I know it's ugly))
usage:
	Print("Valid Colours are ");
	for(fg=0;fg<8;fg++)
	{
		Print(cCOLOUR_NAMES[fg]);
		write(_stdout, 3, ", ");
	}
	write(_stdout, 4, "\b\b\n");
	return;
}

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
		//printf("Unable to open directory `%s', File cannot be found\n", tmpPath);
		write(_stdout, 27, "Unable to open directory `");
		write(_stdout, strlen(tmpPath)+1, tmpPath);
		write(_stdout, 25, "', File cannot be found\n");
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
		acl.group = 0;	acl.id = info.uid;
		_SysGetACL(fp, &acl);
		if(acl.perms & 1)	modeStr[0] = 'r';	else	modeStr[0] = '-';
		if(acl.perms & 2)	modeStr[1] = 'w';	else	modeStr[1] = '-';
		if(acl.perms & 8)	modeStr[2] = 'x';	else	modeStr[2] = '-';
		acl.group = 1;	acl.id = info.gid;
		_SysGetACL(fp, &acl);
		if(acl.perms & 1)	modeStr[3] = 'r';	else	modeStr[3] = '-';
		if(acl.perms & 1)	modeStr[4] = 'w';	else	modeStr[4] = '-';
		if(acl.perms & 1)	modeStr[5] = 'x';	else	modeStr[5] = '-';
		acl.group = 1;	acl.id = -1;
		_SysGetACL(fp, &acl);
		if(acl.perms & 1)	modeStr[6] = 'r';	else	modeStr[6] = '-';
		if(acl.perms & 1)	modeStr[7] = 'w';	else	modeStr[7] = '-';
		if(acl.perms & 1)	modeStr[8] = 'x';	else	modeStr[8] = '-';
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
