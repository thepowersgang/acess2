/*
 * Acess2 System Init Task
 */
#include <acess/sys.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include <ctype.h>

// === CONSTANTS ===
#define DEFAULT_SHELL	"/Acess/SBin/login"
#define INITTAB_FILE	"/Acess/Conf/inittab"

#define ARRAY_SIZE(x)	((sizeof(x))/(sizeof((x)[0])))

// === PROTOTYPES ===
 int	ProcessInittab(const char *Path);
char	*ReadQuotedString(FILE *FP);
char	**ReadCommand(FILE *FP);
void	FreeCommand(char **Command);

tInitProgram	*AllocateProgram(char **Command, enum eProgType Type, size_t ExtraSpace);
void	RespawnProgam(tInitProgram *Program);

 int	AddKTerminal(int TerminalSpec, char **Command);
 int	AddSerialTerminal(const char *DevPathSegment, const char *ModeStr, char **Command);
 int	AddDaemon(char *StdoutPath, char *StderrPath, char **Command);

 int	SpawnCommand(int c_stdin, int c_stdout, int c_stderr, char **ArgV);
 int	SpawnKTerm(tInitProgram *Program);
 int	SpawnSTerm(tInitProgram *Program);
 int	SpawnDaemon(tInitProgram *Program);

// === GLOBALS ===
const char	*gsInittabPath = INITTAB_FILE;
tInitProgram	*gpInitPrograms;

// === CODE ===
/**
 * \fn int main(int argc, char *argv[])
 * \brief Entrypoint
 */
int main(int argc, char *argv[])
{
	// Parse commandline args
	// TODO: cmdline

	// TODO: Behave differently if not invoked as first process?

	// Begin init process
	if( ProcessInittab(gsInittabPath) != 0 )
	{
		// No inittab file found, default to:
		_SysDebug("inittab '%s' is invalid, falling back to one VT", gsInittabPath);
		
		#if 0
		for( ;; )
		{
			int pid = SpawnCommand(0, 1, 1, (char *[]){DEFAULT_SHELL, NULL});
			// TODO: Detect errors
			_SysWaitTID(pid, NULL);
		}
		#else
		return 41;
		#endif
	}

	// TODO: Implement message watching
	for(;;)
	{
		 int	pid, status;
		pid = _SysWaitTID(-1, &status);
		_SysDebug("PID %i died, looking for respawn entry", pid);
	}
	
	return 42;
}

char *ReadQuotedString(FILE *FP)
{
	char	ch;
	
	while( isblank(ch = fgetc(FP)) )
		;

	if( ch == '\n' ) {
		return NULL;
	}

	char	*retstr = NULL;
	 int	mode = 0;
	 int	pos = 0, space = 0;
	for( ; !feof(FP); ch = fgetc(FP) )
	{
		 int	skip;
		skip = 1;
		if( mode & 4 ) {
			// Add escaped/special character
			skip = 0;
			mode &= 3;
		}
		else if( mode == 0 )
		{
			if( isspace(ch) ) {
				fseek(FP, -1, SEEK_CUR);
				break;
			}
			else if( ch == '\\' )
				mode |= 4;
			else if( ch == '"' )
				mode = 1;
			else if( ch == '\'')
				mode = 2;
			else {
				// Add character
				skip = 0;
			}
		}
		// Double-quoted string
		else if( mode == 1 )
		{
			if( ch == '"' )
				mode = 0;
			else if( ch == '\\' )
				mode |= 4;
			else {
				// Add character
				skip = 0;
			}
		}
		// Single-quoted string
		else if( mode == 2 )
		{
			if( ch == '\'' )
				mode = 0;
			else if( ch == '\\' )
				mode |= 4;
			else {
				// Add character
				skip = 0;
			}
		}
	
		if( !skip )
		{
			if( pos == space ) {
				space += 32;
				void *tmp = realloc(retstr, space+1);
				if( !tmp ) {
					_SysDebug("ReadQuotedString - realloc(%i) failure", space+1);
					free(retstr);
					return NULL;
				}
				retstr = tmp;
			}
			retstr[pos++] = ch;
		}
	}
	retstr[pos] = '\0';
	return retstr;
}

char **ReadCommand(FILE *FP)
{
	const int	space = 8;
	 int	pos = 0;
	char	**ret = malloc(space*sizeof(char*));
	char	*arg;
	do {
		arg = ReadQuotedString(FP);
		if(arg == NULL)
			break;
		if( pos == space - 1 ) {
			_SysDebug("Too many arguments %i", pos);
			ret[pos] = NULL;
			FreeCommand(ret);
			return NULL;
		}
		ret[pos++] = arg;
	} while(arg != NULL);
	ret[pos] = NULL;
	return ret;
}

void FreeCommand(char **Command)
{
	int pos = 0;
	while( Command[pos] )
	{
		free(Command[pos]);
		pos ++;
	}
	free(Command);
}

int ProcessInittab(const char *Path)
{
	FILE	*fp = fopen(Path, "r");

	if( !fp )
		return 1;	

	while(!feof(fp))
	{
		char cmdbuf[64+1];
		
		 int	rv;
		if( (rv = fscanf(fp, "%64s%*[ \t]", &cmdbuf)) != 1 ) {
			_SysDebug("fscanf rv %i != exp 1", rv);
			break;
		}
		
		// Clear comments
		if( cmdbuf[0] == '#' ) {
			while( !feof(fp) && fgetc(fp) != '\n' )
				;
			continue ;
		}
		if( cmdbuf[0] == '\0' ) {
			char	ch = fgetc(fp);
			if( ch != '\n' && ch != -1 ) {
				fclose(fp);
				_SysDebug("Unexpected char 0x%x, expecting EOL", ch);
				return 2;	// Unexpected character?
			}
			continue ;
		}

		// Check commands
		if( strcmp(cmdbuf, "ktty") == 0 ) {
			// ktty <ID> <command...>
			// - Spins off a console on the specified kernel TTY
			 int	id = 0;
			if( fscanf(fp, "%d ", &id) != 1 ) {
				_SysDebug("init[ktty] - TTY ID read failed");
				goto lineError;
			}
			char	**command = ReadCommand(fp);
			if( !command ) {
				_SysDebug("init[ktty] - Command read failure");
				goto lineError;
			}
			AddKTerminal(id, command);
			free(command);
		}
		else if(strcmp(cmdbuf, "stty") == 0 ) {
			// stty <devpath> [78][NOE][012][bB]<baud> <command...>
			char	path_seg[32+1];
			char	modespec[4+6+1];
			if( fscanf(fp, "%32s %6s ", &path_seg, &modespec) != 2 ) {
				goto lineError;
			}
			char **command = ReadCommand(fp);
			if( !command )
				goto lineError;
			AddSerialTerminal(path_seg, modespec, command);
		}
		else if(strcmp(cmdbuf, "daemon") == 0 ) {
			// daemon <stdout> <stderr> <command...>
			// - Runs a daemon (respawning) that logs to the specified files
			// - Will append a header whenever the daemon starts
			char	*stdout_path = ReadQuotedString(fp);
			char	*stderr_path = ReadQuotedString(fp);
			char	**command = ReadCommand(fp);
			
			AddDaemon(stdout_path, stderr_path, command);
		}
		else if(strcmp(cmdbuf, "exec") == 0 ) {
			// exec <command...>
			// - Runs a command and waits for it to complete before continuing
			// - NOTE: No other commands will respawn while this is running
			char **command = ReadCommand(fp);
			if(!command)
				goto lineError;

			int handles[] = {0, 1, 2};
			int pid = _SysSpawn(command[0], (const char **)command, NULL, 3, handles, NULL);
			int retstatus;
			_SysWaitTID(pid, &retstatus);
			_SysDebug("Command '%s' returned %i", command[0], retstatus);

			FreeCommand(command);
		}
		else {
			// Unknown command.
			_SysDebug("Unknown command '%s'", cmdbuf);
			goto lineError;
		}
		fscanf(fp, " ");
		continue;
	lineError:
		_SysDebug("label lineError: goto'd");
		while( !feof(fp) && fgetc(fp) != '\n' )
			;
		continue ;
	}

	fclose(fp);
	return 0;
}

tInitProgram *AllocateProgram(char **Command, enum eProgType Type, size_t ExtraSpace)
{
	tInitProgram	*ret;
	
	ret = malloc( sizeof(tInitProgram) - sizeof(union uProgTypes) + ExtraSpace );
	ret->Next = NULL;
	ret->CurrentPID = 0;
	ret->Type = Type;
	ret->Command = Command;
	
	// Append
	ret->Next = gpInitPrograms;
	gpInitPrograms = ret;
	
	return ret;
}

void RespawnProgam(tInitProgram *Program)
{
	 int 	rv = 0;
	switch(Program->Type)
	{
	case PT_KTERM:	rv = SpawnKTerm(Program);	break;
	case PT_STERM:	rv = SpawnSTerm(Program);	break;
	case PT_DAEMON:	rv = SpawnDaemon(Program);	break;
	default:
		_SysDebug("BUGCHECK - Program Type %i unknown", Program->Type);
		break;
	}
	if( !rv ) {
		_SysDebug("Respawn failure!");
		// TODO: Remove from list?
	}
}

int AddKTerminal(int TerminalID, char **Command)
{
	// TODO: Smarter validation
	if( TerminalID < 0 || TerminalID > 7 )
		return -1;
	
	tInitProgram	*ent = AllocateProgram(Command, PT_KTERM, sizeof(struct sKTerm));
	ent->TypeInfo.KTerm.ID = TerminalID;

	RespawnProgam(ent);
	return 0;
}

int AddSerialTerminal(const char *DevPathSegment, const char *ModeStr, char **Command)
{
	char	dbit, parity, sbit;
	 int	baud;

	// Parse mode string
	if( sscanf(ModeStr, "%1[78]%1[NOE]%1[012]%*1[bB]%d", &dbit, &parity, &sbit, &baud) != 5 ) {
		// Oops?
		return -1;
	}
	
	// Validate baud rate / build mode word
	// TODO: Does baud rate need validation?
	uint32_t	modeword = 0;
	modeword |= (dbit == '7' ? 1 : 0) << 0;
	modeword |= (parity == 'O' ? 1 : (parity == 'E' ? 2 : 0)) << 1;
	modeword |= (sbit - '0') << 3;
	
	// Create info
	const char DEVPREFIX[] = "/Devices/";
	int pathlen = sizeof(DEVPREFIX) + strlen(DevPathSegment);
	tInitProgram	*ent = AllocateProgram(Command, PT_STERM, sizeof(struct sSTerm)+pathlen);
	ent->TypeInfo.STerm.FormatBits = modeword;
	ent->TypeInfo.STerm.BaudRate = baud;
	strcpy(ent->TypeInfo.STerm.Path, DEVPREFIX);
	strcat(ent->TypeInfo.STerm.Path, DevPathSegment);

	RespawnProgam(ent);
	return 0;
}

int AddDaemon(char *StdoutPath, char *StderrPath, char **Command)
{
	tInitProgram	*ent = AllocateProgram(Command, PT_DAEMON, sizeof(struct sDaemon));
	ent->TypeInfo.Daemon.StdoutPath = StdoutPath;
	ent->TypeInfo.Daemon.StderrPath = StderrPath;
	
	RespawnProgam(ent);
	return 0;
}

int SpawnCommand(int c_stdin, int c_stdout, int c_stderr, char **ArgV)
{
	 int	handles[] = {c_stdin, c_stdout, c_stderr};

	int rv = _SysSpawn(ArgV[0], (const char **)ArgV, NULL, 3, handles, NULL);

	_SysClose(c_stdin);
	if( c_stdout != c_stdin )
		_SysClose(c_stdout);
	if( c_stderr != c_stdin && c_stderr != c_stdout )
		_SysClose(c_stderr);

	return rv;
}

int SpawnKTerm(tInitProgram *Program)
{
	const char fmt[] = "/Devices/VTerm/%i";
	char	path[sizeof(fmt)];
	
	snprintf(path, sizeof(path), fmt, Program->TypeInfo.KTerm.ID);
	
	 int	in = _SysOpen(path, OPENFLAG_READ);
	 int	out = _SysOpen(path, OPENFLAG_WRITE);
	
	return SpawnCommand(in, out, out, Program->Command);
}

int SpawnSTerm(tInitProgram *Program)
{
	 int	in = _SysOpen(Program->TypeInfo.STerm.Path, OPENFLAG_READ);
	 int	out = _SysOpen(Program->TypeInfo.STerm.Path, OPENFLAG_WRITE);

	#if 0
	if( _SysIOCtl(in, 0, NULL) != DRV_TYPE_SERIAL )
	{
		// Oops?
		return -2;
	}
	_SysIOCtl(in, SERIAL_IOCTL_GETSETBAUD, &Program->TypeInfo.STerm.BaudRate);
	_SysIOCtl(in, SERIAL_IOCTL_GETSETFORMAT, &Program->TypeInfo.STerm.FormatBits);
	#endif

	return SpawnCommand(in, out, out, Program->Command);
}

int SpawnDaemon(tInitProgram *Program)
{
	 int	in = _SysOpen("/Devices/null", OPENFLAG_READ);
	 int	out = _SysOpen(Program->TypeInfo.Daemon.StdoutPath, OPENFLAG_WRITE);
	 int	err = _SysOpen(Program->TypeInfo.Daemon.StderrPath, OPENFLAG_WRITE);
	
	if( in == -1 || out == -1 || err == -1 ) {
		_SysClose(in);
		_SysClose(out);
		_SysClose(err);
		return -2;
	}
	
	return SpawnCommand(in, out, err, Program->Command);
}

