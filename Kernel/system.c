/*
 * Acess 2
 * Architecture Independent System Init
 * system.c
 */
#define DEBUG	1
#include <common.h>

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

// === IMPORTS ===
extern int	Modules_LoadBuiltins();
extern int	PCI_Install();
extern void	DMA_Install();
extern void	Debug_SetKTerminal(char *File);
extern void	StartupPrint(char *Str);

// === PROTOTYPES ===
void	System_Init(char *ArgString);
void	System_ParseCommandLine(char *ArgString);
void	System_ParseVFS(char *Arg);
void	System_ParseSetting(char *Arg);
void	System_ExecuteScript();
tConfigFile	*System_Int_ParseFile(char *File);

// === GLOBALS ===
char	*gsConfigScript = "/Acess/Conf/BootConf.cfg";

// === CODE ===
void System_Init(char *ArgString)
{
	// - Start Builtin Drivers & Filesystems
	StartupPrint("Scanning PCI Bus...");
	PCI_Install();
	StartupPrint("Loading DMA...");
	DMA_Install();
	StartupPrint("Loading staticly compiled modules...");
	Modules_LoadBuiltins();
	
	// Set the debug to be echoed to the terminal
	StartupPrint("Kernel now echoes to VT6 (Ctrl-Alt-F7)");
	Debug_SetKTerminal("/Devices/VTerm/6");
	
	// - Parse Kernel's Command Line
	System_ParseCommandLine(ArgString);
	
	// - Execute the Config Script
	Log("Executing config script...");
	System_ExecuteScript();
}

/**
 * \fn void System_ParseCommandLine(char *ArgString)
 * \brief Parses the kernel's command line and sets the environment
 */
void System_ParseCommandLine(char *ArgString)
{
	char	*argv[32];
	 int	argc;
	 int	i;
	char	*str;
	
	Log("Kernel Command Line: \"%s\"", ArgString);
	
	// --- Get Arguments ---
	str = ArgString;
	for( argc = 0; argc < 32; argc++ )
	{
		while(*str == ' ')	str++;	// Eat Whitespace
		if(*str == '\0') {	argc--;	break;}	// End of string
		argv[argc] = str;
		while(*str && *str != ' ')
		{
			/*if(*str == '"') {
				while(*str && !(*str == '"' && str[-1] != '\\'))
					str ++;
			}*/
			str++;
		}
		if(*str == '\0')	break;	// End of string
		*str = '\0';	// Cap off argument string
		str ++;	// and increment the string pointer
	}
	if(argc < 32)
		argc ++;	// Count last argument
	
	// --- Parse Arguments ---
	for( i = 1; i < argc; i++ )
	{
		if( argv[i][0] == '/' )
			System_ParseVFS( argv[i] );
		else
			System_ParseSetting( argv[i] );
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
		Warning("Expected '=' in the string '%s'", Arg);
		return ;
	}
	
	// Edit string
	*value = '\0';	value ++;
	
	// Check assignment type
	// - Symbolic Link <link>=<destination>
	if(value[0] == '/')
	{
		Log("Symbolic link '%s' pointing to '%s'", Arg, value);
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
			Log("Creating directory '%s'", Arg, value);
			VFS_MkDir( Arg );
		} else {
			VFS_Close(fd);
		}
		// Mount
		Log("Mounting '%s' to '%s' ('%s')", dev, Arg, value);
		VFS_Mount(dev, Arg, value, "");
	}
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
		if(strcmp(Arg, "") == 0) {
		} else {
			Warning("Kernel flag '%s' is not recognised", Arg);
		}
	}
	else
	{
		*value = '\0';	// Remove '='
		value ++;	// and eat it's position
		
		if(strcmp(Arg, "SCRIPT") == 0) {
			Log("Config Script: '%s'", value);
			gsConfigScript = value;
		} else {
			Warning("Kernel config setting '%s' is not recognised", Arg);
		}
		
	}
}

/**
 * \fn void System_ExecuteScript()
 */
void System_ExecuteScript()
{
	 int	fp;
	 int	fLen = 0;
	 int	i;
	char	*fData;
	tConfigFile	*file;
	tConfigLine	*line;
	
	// Open Script
	fp = VFS_Open(gsConfigScript, VFS_OPENFLAG_READ);
	if(fp == -1) {
		Warning("[CFG] Passed script '%s' does not exist", gsConfigScript);
		return;
	}
	
	// Read into memory buffer
	VFS_Seek(fp, 0, SEEK_END);
	fLen = VFS_Tell(fp);
	VFS_Seek(fp, 0, SEEK_SET);
	fData = malloc(fLen+1);
	VFS_Read(fp, fLen, fData);
	fData[fLen] = '\0';
	VFS_Close(fp);
	
	
	
	// Parse File
	file = System_Int_ParseFile(fData);
	
	// Loop lines
	for( i = 0; i < file->nLines; i++ )
	{
		line = &file->Lines[i];
		if( line->nParts == 0 )	continue;	// Skip blank
		
		// Mount
		if( strcmp(line->Parts[0], "mount") == 0 ) {
			if( line->nParts != 4 ) {
				Warning("Configuration command 'mount' requires 3 arguments, %i given",
					line->nParts-1);
				continue;
			}
			//Log("[CFG ] Mount '%s' to '%s' (%s)",
			//	line->Parts[1], line->Parts[2], line->Parts[3]);
			//! \todo Use an optional 4th argument for the options string
			VFS_Mount(line->Parts[1], line->Parts[2], line->Parts[3], "");
		}
		// Module
		else if(strcmp(line->Parts[0], "module") == 0) {
			if( line->nParts < 2 || line->nParts > 3 ) {
				Warning("Configuration command 'module' requires 1 or 2 arguments, %i given",
					line->nParts-1);
				continue;
			}
			if( line->nParts == 3 )
				Module_LoadFile(line->Parts[1], line->Parts[2]);
			else
				Module_LoadFile(line->Parts[1], "");
		}
		// UDI Module
		else if(strcmp(line->Parts[0], "udimod") == 0) {
			if( line->nParts != 2 ) {
				Warning("Configuration command 'udimod' requires 1 argument, %i given",
					line->nParts-1);
				continue;
			}
			Log("[CFG  ] Load UDI Module '%s'", line->Parts[1]);
			Module_LoadFile(line->Parts[1], "");
		}
		// EDI Module
		else if(strcmp(line->Parts[0], "edimod") == 0) {
			if( line->nParts != 2 ) {
				Warning("Configuration command 'edimod' requires 1 argument, %i given",
					line->nParts-1);
				continue;
			}
			Log("[CFG  ] Load EDI Module '%s'", line->Parts[1]);
			Module_LoadFile(line->Parts[1], "");
		}
		// Symbolic Link
		else if(strcmp(line->Parts[0], "symlink") == 0) {
			if( line->nParts != 3 ) {
				Warning("Configuration command 'symlink' requires 2 arguments, %i given",
					line->nParts-1);
				continue;
			}
			Log("[CFG  ] Symlink '%s' pointing to '%s'",
				line->Parts[1], line->Parts[2]);
			VFS_Symlink(line->Parts[1], line->Parts[2]);
		}
		// Create Directory
		else if(strcmp(line->Parts[0], "mkdir") == 0) {
			if( line->nParts != 2 ) {
				Warning("Configuration command 'mkdir' requires 1 argument, %i given",
					line->nParts-1);
				continue;
			}
			Log("[CFG  ] New Directory '%s'", line->Parts[1]);
			VFS_MkDir(line->Parts[1]);
		}
		// Spawn a process
		else if(strcmp(line->Parts[0], "spawn") == 0) {
			if( line->nParts != 2 ) {
				Warning("Configuration command 'spawn' requires 1 argument, %i given",
					line->nParts-1);
				continue;
			}
			Log("[CFG  ] Starting '%s' as a new task", line->Parts[1]);
			Proc_Spawn(line->Parts[1]);
		}
		else {
			Warning("Unknown configuration command '%s' on line %i",
				line->Parts[0],
				line->TrueLine
				);
		}
	}
	
	// Clean up after ourselves
	for( i = 0; i < file->nLines; i++ ) {
		if( file->Lines[i].nParts == 0 )	continue;	// Skip blank
		free( file->Lines[i].Parts );
	}
	free( file );
	free( fData );
}

/**
 * \brief Parses a config file
 * \param FileData	Read/Write buffer containing the config file data
 *                  (will be modified)
 * \return ::tConfigFile structure that represents the original contents
 *         of \a FileData
 */
tConfigFile	*System_Int_ParseFile(char *FileData)
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
	
	LEAVE('p', ret);
	return ret;
}
