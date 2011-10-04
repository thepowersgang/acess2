/*
 * Acess 2
 * Architecture Independent System Init
 * system.c
 */
#define DEBUG	0
#include <acess.h>

#define N_VARIABLES	16
#define N_MAX_ARGS	BITS

// === TYPES ===
typedef struct
{
	 int	TrueLine;
	 int	nParts;
	char	**Parts;
}	tConfigLine;
typedef struct
{
	 int	nLines;
	tConfigLine	Lines[];
}	tConfigFile;
typedef struct
{
	const char	*Name;		// Name
	 int	MinArgs;	// Minimum number of arguments
	 int	MaxArgs;	// Maximum number of arguments
	Uint	IntArgs;	// Bitmap of arguments that should be treated as integers
	 int	Index;		// 
	const char	*OptDefaults[N_MAX_ARGS];	// Default values for optional arguments
}	tConfigCommand;

// === IMPORTS ===
extern void	Arch_LoadBootModules(void);
extern int	Modules_LoadBuiltins(void);
extern void	Modules_SetBuiltinParams(char *Name, char *ArgString);
extern void	Debug_SetKTerminal(const char *File);

// === PROTOTYPES ===
void	System_Init(char *Commandline);
void	System_ParseCommandLine(char *ArgString);
void	System_ExecuteCommandLine(void);
void	System_ParseVFS(char *Arg);
void	System_ParseModuleArgs(char *Arg);
void	System_ParseSetting(char *Arg);
void	System_ExecuteScript(void);
tConfigFile	*System_Int_ParseFile(char *File);

// === CONSTANTS ===
enum eConfigCommands {
	CC_LOADMODULE,
	CC_SPAWN,
	CC_MOUNT,
	CC_SYMLINK,
	CC_MKDIR,
	CC_OPEN,
	CC_CLOSE,
	CC_IOCTL
};
const tConfigCommand	caConfigCommands[] = {
	{"module",  1,2, 00, CC_LOADMODULE, {"",NULL}},	// Load a module from a file
	{"spawn",   1,1, 00, CC_SPAWN, {NULL}},		// Spawn a process
	// --- VFS ---
	{"mount",   3,4, 00, CC_MOUNT, {"",0}},		// Mount a device
	{"symlink", 2,2, 00, CC_SYMLINK, {0}},	// Create a Symbolic Link
	{"mkdir",   1,1, 00, CC_MKDIR, {0}},		// Create a Directory
	{"open",    1,2, 00, CC_OPEN,  {(void*)VFS_OPENFLAG_READ,0}},	// Open a file
	{"close",   1,1, 01, CC_CLOSE, {0}},	// Close an open file
	{"ioctl",   3,3, 03, CC_IOCTL, {0}},	// Call an IOCtl
	
	{"", 0,0, 0, 0, {0}}
};
#define NUM_CONFIG_COMMANDS	(sizeof(caConfigCommands)/sizeof(caConfigCommands[0]))

// === GLOBALS ===
const char	*gsConfigScript = "/Acess/Conf/BootConf.cfg";
char	*argv[32];
 int	argc;

// === CODE ===
void System_Init(char *CommandLine)
{
	// Parse Kernel's Command Line
	System_ParseCommandLine(CommandLine);
	
	// Initialise modules
	Log_Log("Config", "Initialising builtin modules...");
	Modules_LoadBuiltins();
	Arch_LoadBootModules();
	
	System_ExecuteCommandLine();
	
	// - Execute the Config Script
	Log_Log("Config", "Executing config script '%s'", gsConfigScript);
	System_ExecuteScript();
	
	// Set the debug to be echoed to the terminal
	Log_Log("Config", "Kernel now echoes to VT7 (Ctrl-Alt-F8)");
	Debug_SetKTerminal("/Devices/VTerm/7");
}

/**
 * \fn void System_ParseCommandLine(char *ArgString)
 * \brief Parses the kernel's command line and sets the environment
 */
void System_ParseCommandLine(char *ArgString)
{
	 int	i;
	char	*str;
	
	Log_Log("Config", "Kernel Invocation (%p) \"%s\"", ArgString, ArgString);
	
	// --- Get Arguments ---
	str = ArgString;
	for( argc = 0; argc < 32; argc++ )
	{
		// Eat Whitespace
		while(*str == ' ')	str++;
		// Check for the end of the string
		if(*str == '\0') {	argc--;	break;}	
		argv[argc] = str;
		if(*str == '"') {
			while(*str && !(*str == '"' && str[-1] != '\\'))
				str ++;
		}
		else {
			while(*str && *str != ' ')
				str++;
		}
		if(*str == '\0')	break;	// Check for EOS
		*str = '\0';	// Cap off argument string
		str ++;	// and increment the string pointer
	}
	if(argc < 32)
		argc ++;	// Count last argument
	
	// --- Parse Arguments (Pass 1) ---
	for( i = 0; i < argc; i++ )
	{
		switch(argv[i][0])
		{
		// --- VFS ---
		// Ignored on this pass
		case '/':
			break;
		
		// --- Module Paramaters ---
		// -VTerm:Width=640,Height=480,Scrollback=2
		case '-':
			System_ParseModuleArgs( argv[i] );
			break;
		// --- Config Options ---
		// SCRIPT=/Acess/Conf/BootConf.cfg
		default:
			System_ParseSetting( argv[i] );
			break;
		}
	}
}

void System_ExecuteCommandLine(void)
{
	 int	i;
	for( i = 0; i < argc; i++ )
	{
		LOG("argv[%i] = '%s'", i, argv[i]);
		switch(argv[i][0])
		{
		// --- VFS ---
		// Mount    /System=ext2:/Devices/ATA/A1
		// Symlink  /Acess=/System/Acess2
		case '/':
			System_ParseVFS( argv[i] );
			break;
		}
	}
}

/**
 * \fn void System_ParseVFS(char *Arg)
 */
void System_ParseVFS(char *Arg)
{
	char	*value;
	 int	fd;
	
	value = Arg;
	// Search for the '=' token
	while( *value && *value != '=' )
		value++;
	
	// Check if the equals was found
	if( *value == '\0' ) {
		Log_Warning("Config", "Expected '=' in the string '%s'", Arg);
		return ;
	}
	
	// Edit string
	*value = '\0';	value ++;
	
	// Check assignment type
	// - Symbolic Link <link>=<destination>
	if(value[0] == '/')
	{
		Log_Log("Config", "Symbolic link '%s' pointing to '%s'", Arg, value);
		VFS_Symlink(Arg, value);
	}
	// - Mount <mountpoint>=<fs>:<device>
	else
	{
		char	*dev = value;
		// Find colon
		while(*dev && *dev != ':')	dev++;
		if(*dev) {
			*dev = '\0';
			dev++;	// Eat ':'
		}
		// Create Mountpoint
		if( (fd = VFS_Open(Arg, 0)) == -1 ) {
			Log_Log("Config", "Creating directory '%s'", Arg, value);
			VFS_MkDir( Arg );
		} else {
			VFS_Close(fd);
		}
		// Mount
		Log_Log("Config", "Mounting '%s' to '%s' ('%s')", dev, Arg, value);
		VFS_Mount(dev, Arg, value, "");
	}
}

/**
 * \brief Parse a module argument string
 * \param Arg	Argument string
 */
void System_ParseModuleArgs(char *Arg)
{
	char	*name, *args;
	 int	i;
	
	// Remove '-'	
	name = Arg + 1;
	
	// Find the start of the args
	i = strpos(name, ':');
	if( i == -1 ) {
		Log_Warning("Config", "Module spec with no arguments");
		#if 1
		return ;
		#else
		i = strlen(name);
		args = name + i;
		#endif
	}
	else {
		name[i] = '\0';
		args = name + i + 1;
	}
	
	Log_Log("Config", "Setting boot parameters for '%s' to '%s'", name, args);
	Modules_SetBuiltinParams(name, args);
}

/**
 * \fn void System_ParseSetting(char *Arg)
 */
void System_ParseSetting(char *Arg)
{
	char	*value;
	value = Arg;

	// Search for the '=' token
	while( *value && *value != '=' )
		value++;
	
	// Check for boolean/flag (no '=')
	if(*value == '\0')
	{
		//if(strcmp(Arg, "") == 0) {
		//} else {
			Log_Warning("Config", "Kernel flag '%s' is not recognised", Arg);
		//}
	}
	else
	{
		*value = '\0';	// Remove '='
		value ++;	// and eat it's position
		
		if(strcmp(Arg, "SCRIPT") == 0) {
			Log_Log("Config", "Config Script: '%s'", value);
			if(strlen(value) == 0)
				gsConfigScript = NULL;
			else
				gsConfigScript = value;
		} else {
			Log_Warning("Config", "Kernel config setting '%s' is not recognised", Arg);
		}
		
	}
}

/**
 * \fn void System_ExecuteScript()
 * \brief Reads and parses the boot configuration script
 */
void System_ExecuteScript(void)
{
	 int	fp;
	 int	fLen = 0;
	 int	i, j, k;
	 int	val;
	 int	result = 0;
	 int	variables[N_VARIABLES];
	 int	bReplaced[N_MAX_ARGS];
	char	*fData;
	char	*jmpTarget;
	tConfigFile	*file;
	tConfigLine	*line;
	
	ENTER("");

	// Open Script
	fp = VFS_Open(gsConfigScript, VFS_OPENFLAG_READ);
	if(fp == -1) {
		Log_Warning("Config", "Passed script '%s' does not exist", gsConfigScript);
		LEAVE('-');
		return;
	}
	
	// Get length
	VFS_Seek(fp, 0, SEEK_END);
	fLen = VFS_Tell(fp);
	LOG("VFS_Tell(0x%x) = %i", fp, fLen);
	VFS_Seek(fp, 0, SEEK_SET);
	// Read into memory buffer
	fData = malloc(fLen+1);
	VFS_Read(fp, fLen, fData);
	fData[fLen] = '\0';
	VFS_Close(fp);
	
	
	// Parse File
	file = System_Int_ParseFile(fData);
	
	// Parse each line
	for( i = 0; i < file->nLines; i++ )
	{
		line = &file->Lines[i];
		if( line->nParts == 0 )	continue;	// Skip blank
		
		if(line->Parts[0][0] == ':')	continue;	// Ignore labels
		
		// Prescan and eliminate variables
		for( j = 1; j < line->nParts; j++ )
		{
			LOG("Arg #%i is '%s'", j, line->Parts[j]);
			bReplaced[j] = 0;
			if( line->Parts[j][0] != '$' )	continue;
			if( line->Parts[j][1] == '?' ) {
				val = result;
			}
			else {
				val = atoi( &line->Parts[j][1] );
				if( val < 0 || val > N_VARIABLES )	continue;
				val = variables[ val ];
			}
			LOG("Replaced arg %i ('%s') with 0x%x", j, line->Parts[j], val);
			line->Parts[j] = malloc( BITS/8+2+1 );
			sprintf(line->Parts[j], "0x%x", val);
			bReplaced[j] = 1;
		}
		
		// Find the command name
		for( j = 0; j < NUM_CONFIG_COMMANDS; j++ )
		{
			const char	*args[N_MAX_ARGS];
			
			if(strcmp(line->Parts[0], caConfigCommands[j].Name) != 0)	continue;
			
			Log_Debug("Config", "Command '%s', %i args passed", line->Parts[0], line->nParts-1);
			
			// Check against minimum argument count
			if( line->nParts - 1 < caConfigCommands[j].MinArgs ) {
				Log_Warning("Config",
					"Configuration command '%s' requires at least %i arguments, %i given",
					caConfigCommands[j].Name, caConfigCommands[j].MinArgs, line->nParts-1
					);
				break;
			}
			
			// Check for extra arguments
			if( line->nParts - 1 > caConfigCommands[j].MaxArgs ) {
				Log_Warning("Config",
					"Configuration command '%s' takes at most %i arguments, %i given",
					caConfigCommands[j].Name, caConfigCommands[j].MaxArgs, line->nParts-1
					);
				break;
			}
			
			// Fill in defaults
			for( k = caConfigCommands[j].MaxArgs-1; k > line->nParts - 1; k-- ) {
				args[k] = caConfigCommands[j].OptDefaults[k];
			}
			
			// Convert arguments to integers
			for( k = line->nParts-1; k--; )
			{
				if( k < 32 && (caConfigCommands[j].IntArgs & (1 << k)) ) {
					args[k] = (const char *)(Uint)atoi(line->Parts[k+1]);
				}
				else {
					args[k] = (char *)line->Parts[k+1];
				}
				LOG("args[%i] = %p", k, args[k]);
			}
			switch( (enum eConfigCommands) caConfigCommands[j].Index )
			{
			case CC_LOADMODULE:
				result = Module_LoadFile( args[0], args[1] );
				break;
			case CC_SPAWN:
				result = Proc_Spawn( args[0] );
				break;
			case CC_MOUNT:
				result = VFS_Mount( args[0], args[1], args[2], args[3] );
				break;
			case CC_SYMLINK:
				result = VFS_Symlink( args[0], args[1] );
				break;
			case CC_OPEN:
				result = VFS_Open( args[0], (Uint)args[1] );
				break;
			case CC_CLOSE:
				VFS_Close( (Uint)args[0] );
				result = 0;
				break;
			case CC_MKDIR:
				result = VFS_MkDir( args[0] );
				break;
			case CC_IOCTL:
				result = VFS_IOCtl( (Uint)args[0], (Uint)args[1], (void *)args[2] );
				break;
			}
			LOG("Config", "result = %i", result);
			break;
		}
		if( j < NUM_CONFIG_COMMANDS )	continue;
			
		// --- State and Variables ---
		if(strcmp(line->Parts[0], "set") == 0)
		{
			 int	to, value;
			if( line->nParts-1 != 2 ) {
				Log_Warning("Config", "Configuration command 'set' requires 2 arguments, %i given",
					line->nParts-1);
				continue;
			}
			
			to = atoi(line->Parts[1]);
			value = atoi(line->Parts[2]);
			
			variables[to] = value;
			result = value;
		}
		// if <val1> <op> <val2> <dest>
		else if(strcmp(line->Parts[0], "if") == 0)
		{
			if( line->nParts-1 != 4 ) {
				Log_Warning("Config", "Configuration command 'if' requires 4 arguments, %i given",
					line->nParts-1);
			}
			
			result = atoi(line->Parts[1]);
			val = atoi(line->Parts[3]);
			
			jmpTarget = line->Parts[4];
			
			Log_Log("Config", "IF 0x%x %s 0x%x THEN GOTO %s",
				result, line->Parts[2], val, jmpTarget);
			
			if( strcmp(line->Parts[2], "<" ) == 0 ) {
				if( result < val )	goto jumpToLabel;
			}
			else if( strcmp(line->Parts[2], "<=") == 0 ) {
				if( result <= val )	goto jumpToLabel;
			}
			else if( strcmp(line->Parts[2], ">" ) == 0 ) {
				if (result > val )	goto jumpToLabel;
			}
			else if( strcmp(line->Parts[2], ">=") == 0 ) {
				if( result >= val )	goto jumpToLabel;
			}
			else if( strcmp(line->Parts[2],  "=") == 0 ) {
				if( result == val )	goto jumpToLabel;
			}
			else if( strcmp(line->Parts[2], "!=") == 0 ) {
				if( result != val )	goto jumpToLabel;
			}
			else {
				Log_Warning("Config", "Unknown comparision '%s' in `if`", line->Parts[2]);
			}
			
		}
		else if(strcmp(line->Parts[0], "goto") == 0) {
			if( line->nParts-1 != 1 ) {
				Log_Warning("Config", "Configuration command 'goto' requires 1 arguments, %i given",
					line->nParts-1);
			}
			jmpTarget = line->Parts[1];
		
		jumpToLabel:
			for( j = 0; j < file->nLines; j ++ )
			{
				if(file->Lines[j].nParts == 0)
					continue;
				if(file->Lines[j].Parts[0][0] != ':')
					continue;
				if( strcmp(file->Lines[j].Parts[0]+1, jmpTarget) == 0)
					break;
			}
			if( j == file->nLines )
				Log_Warning("Config", "Unable to find label '%s'", jmpTarget);
			else
				i = j;
		}
		else {
			Log_Warning("Config", "Unknown configuration command '%s' on line %i",
				line->Parts[0],
				line->TrueLine
				);
		}
	}
	
	// Clean up after ourselves
	for( i = 0; i < file->nLines; i++ ) {
		if( file->Lines[i].nParts == 0 )	continue;	// Skip blank
		for( j = 0; j < file->Lines[i].nParts; j++ ) {
			if(IsHeap(file->Lines[i].Parts[j]))
				free(file->Lines[i].Parts[j]);
		}
		free( file->Lines[i].Parts );
	}
	
	// Free data
	free( file );
	free( fData );
	
	LEAVE('-');
}

/**
 * \brief Parses a config file
 * \param FileData	Read/Write buffer containing the config file data
 *                  (will be modified)
 * \return ::tConfigFile structure that represents the original contents
 *         of \a FileData
 */
tConfigFile *System_Int_ParseFile(char *FileData)
{
	char	*ptr;
	char	*start;
	 int	nLines = 1;
	 int	i, j;
	tConfigFile	*ret;
	
	ENTER("pFileData", FileData);
	
	// Prescan and count the number of lines
	for(ptr = FileData; *ptr; ptr++)
	{		
		if(*ptr != '\n')	continue;
		
		if(ptr == FileData) {
			nLines ++;
			continue;
		}
		
		// Escaped EOL
		if(ptr[-1] == '\\')	continue;
		
		nLines ++;
	}
	
	LOG("nLines = %i", nLines);
	
	// Ok so we have `nLines` lines, now to allocate our return
	ret = malloc( sizeof(tConfigFile) + sizeof(tConfigLine)*nLines );
	ret->nLines = nLines;
	
	// Read the file for real
	for(
		ptr = FileData, i = 0;
		*ptr;
		i++
		)
	{
		start = ptr;
		
		ret->Lines[i].nParts = 0;
		ret->Lines[i].Parts = NULL;
		
		// Count parts
		for(;;)
		{
			// Read leading whitespace
			while( *ptr == '\t' || *ptr == ' ' )	ptr++;
			
			// End of line/file
			if( *ptr == '\0' || *ptr == '\n' ) {
				if(*ptr == '\n')	ptr ++;
				break;
			}
			// Comment
			if( *ptr == '#' || *ptr == ';' ) {
				while( *ptr && *ptr != '\n' )	ptr ++;
				if(*ptr == '\n')	ptr ++;
				break;
			}
			
			ret->Lines[i].nParts ++;
			// Quoted
			if( *ptr == '"' ) {
				ptr ++;
				while( *ptr && !(*ptr == '"' && ptr[-1] == '\\') && *ptr != '\n' )
					ptr++;
				continue;
			}
			// Unquoted
			while( *ptr && !(*ptr == '\t' || *ptr == ' ') && *ptr != '\n' )
				ptr++;
		}
		
		LOG("ret->Lines[%i].nParts = %i", i, ret->Lines[i].nParts);
		
		if( ret->Lines[i].nParts == 0 ) {
			ret->Lines[i].Parts = NULL;
			continue;
		}
		
		// Allocate part list
		ret->Lines[i].Parts = malloc( sizeof(char*) * ret->Lines[i].nParts );
		
		// Fill list
		for( ptr = start, j = 0; ; j++ )
		{
			// Read leading whitespace
			while( *ptr == '\t' || *ptr == ' ' )	ptr++;
			
			// End of line/file
			if( *ptr == '\0' || *ptr == '\n' ) {
				if(*ptr == '\n')	ptr ++;
				break;
			}
			// Comment
			if( *ptr == '#' || *ptr == ';' ) {
				while( *ptr && *ptr != '\n' )	ptr ++;
				if(*ptr == '\n')	ptr ++;
				break;
			}
			
			ret->Lines[i].Parts[j] = ptr;
			
			// Quoted
			if( *ptr == '"' ) {
				ptr ++;
				ret->Lines[i].Parts[j] = ptr;
				while( *ptr && !(*ptr == '"' && ptr[-1] == '\\') && *ptr != '\n' )
					ptr++;
			}
			// Unquoted
			else {
				while( *ptr != '\t' && *ptr != ' ' && *ptr != '\n' )
					ptr++;
			}
			
			// Break if we have reached NULL
			if( *ptr == '\0' ) {
				LOG("ret->Lines[%i].Parts[%i] = '%s'", i, j, ret->Lines[i].Parts[j]);
				break;
			}
			if( *ptr == '\n' ) {
				*ptr = '\0';
				LOG("ret->Lines[%i].Parts[%i] = '%s'", i, j, ret->Lines[i].Parts[j]);
				ptr ++;
				break;
			}
			*ptr = '\0';	// Cap off string
			LOG("ret->Lines[%i].Parts[%i] = '%s'", i, j, ret->Lines[i].Parts[j]);
			ptr ++;	// And increment for the next round
		}
	}
	
	if( i < ret->nLines ) {
		ret->Lines[i].nParts = 0;
		ret->Lines[i].Parts = NULL;
		LOG("Cleaning up final empty line");
	}
	
	LEAVE('p', ret);
	return ret;
}
