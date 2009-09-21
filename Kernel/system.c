/*
 * Acess 2
 * Architecture Independent System Init
 * system.c
 */
#include <common.h>

// === IMPORTS ===
extern int	Modules_LoadBuiltins();
extern int	PCI_Install();

// === PROTOTYPES ===
void	System_Init(char *ArgString);
void	System_ParseCommandLine(char *ArgString);
void	System_ParseVFS(char *Arg);
void	System_ParseSetting(char *Arg);
void	System_ExecuteScript();
 int	System_Int_GetString(char *Str, char **Dest);

// === GLOBALS ===
char	*gsInitPath = "/Acess/Bin/init";
char	*gsConfigScript = "/Acess/BootConf.cfg";

// === CODE ===
void System_Init(char *ArgString)
{
	// - Start Builtin Drivers & Filesystems
	PCI_Install();
	//ATA_Install();
	Modules_LoadBuiltins();
	
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
	 int	i = 0, lineStart;
	char	*sArg1, *sArg2, *sArg3;
	char	*fData;
	
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
	
	// Read Script
	while(i < fLen)
	{
		sArg1 = sArg2 = sArg3 = NULL;
		
		lineStart = i;
		// Clear leading whitespace and find empty lines
		while(i < fLen && (fData[i] == ' ' || fData[i]=='\t'))	i ++;
		if(i == fLen)	break;
		if(fData[i] == '\n') {
			i++;
			continue;
		}
		
		// Comment
		if(fData[i] == ';' || fData[i] == '#') {
			while(i < fLen && fData[i] != '\n')	i ++;
			i ++;
			continue;
		}
		
		// Commands
		// - Mount
		if(strncmp("mount ", fData+i, 6) == 0) {
			i += 6;
			i += System_Int_GetString(fData+i, &sArg1);
			if(!sArg1)	goto read2eol;
			i += System_Int_GetString(fData+i, &sArg2);
			if(!sArg2)	goto read2eol;
			i += System_Int_GetString(fData+i, &sArg3);
			if(!sArg3)	goto read2eol;
			//Log("[CFG ] Mount '%s' to '%s' (%s)\n", sArg1, sArg2, sArg3);
			VFS_Mount(sArg1, sArg2, sArg3, "");
		}
		// - Load Module
		else if(strncmp("module ", fData+i, 6) == 0) {
			i += 7;
			i += System_Int_GetString(fData+i, &sArg1);
			if(!sArg1)	goto read2eol;
			//Log("[CFG ] Load Module '%s'\n", sArg1);
			Module_LoadFile(sArg1, "");	//!\todo Use the rest of the line as the argument string
		}
		// - Load Module
		else if(strncmp("edimod ", fData+i, 6) == 0) {
			i += 7;
			i += System_Int_GetString(fData+i, &sArg1);
			if(!sArg1)	goto read2eol;
			Log("[CFG ] Load EDI Module '%s'\n", sArg1);
			Module_LoadFile(sArg1, "");
		}
		// - Symlink
		else if(strncmp("symlink ", fData+i, 7) == 0) {
			i += 8;
			i += System_Int_GetString(fData+i, &sArg1);
			if(!sArg1)	goto read2eol;
			i += System_Int_GetString(fData+i, &sArg2);
			if(!sArg2)	goto read2eol;
			Log("[CFG ] Symlink '%s' pointing to '%s'\n", sArg1, sArg2);
			VFS_Symlink(sArg1, sArg2);
		}
		// - New Directory
		else if(strncmp("mkdir ", fData+i, 5) == 0) {
			i += 6;
			i += System_Int_GetString(fData+i, &sArg1);
			if(!sArg1)	goto read2eol;
			Log("[CFG ] New Directory '%s'\n", sArg1);
			VFS_MkDir(sArg1);
		}
		// - Spawn a task
		else if(strncmp("spawn ", fData+i, 5) == 0) {
			i += 6;
			i += System_Int_GetString(fData+i, &sArg1);
			if(!sArg1)	goto read2eol;
			Log("[CFG ] Starting '%s' as a new task\n", sArg1);
			Proc_Spawn(sArg1);
		}
		else {
			Warning("Unknown configuration command, Line: '%s'", fData+i);
			goto read2eol;
		}
	read2eol:
		if(sArg1)	free(sArg1);
		if(sArg2)	free(sArg2);
		if(sArg3)	free(sArg3);
		// Skip to EOL
		while(i < fLen && fData[i] != '\n')	i++;
		i ++;	// Skip \n
	}
	free(fData);
}

/**
 * \fn int System_Int_GetString(char *Str, char **Dest)
 * \brief Gets a string from another
 * \note Destructive
 * \param Str	Input String
 * \param Dest	Pointer to output pointer
 * \return Characters eaten from input
 */
int System_Int_GetString(char *Str, char **Dest)
{
	 int	pos = 0;
	 int	start = 0;
	 int	len;
	 
	//LogF("GetString: (Str='%s', Dest=0x%x)\n", Str, Dest);
	 
	while(Str[pos] == ' ' || Str[pos] == '\t')	pos++;
	if(Str[pos] == '\n' || Str[pos] == '\0') {
		*Dest = NULL;
		return pos;
	}
	
	// Quoted String
	if(Str[pos] == '"')
	{
		pos ++;
		start = pos;
		while(Str[pos] != '"')	pos++;
		
		len = pos - start;
		*Dest = malloc( len + 1 );
		memcpy( *Dest, Str+start, len );
		(*Dest)[len] = '\0';
		
		//LogF("GetString: RETURN *Dest = '%s'\n", *Dest);
		
		pos++;
		return pos;
	}
	
	// Non-Quoted String - Whitespace deliminated
	start = pos;
	while(Str[pos] != ' ' && Str[pos] != '\t' && Str[pos] != '\n')	pos++;
	
	len = pos - start;
	//LogF(" GetString: len = %i\n", len);
	*Dest = malloc( len + 1 );
	memcpy( *Dest, Str+start, len );
	(*Dest)[len] = '\0';
	
	//LogF("GetString: RETURN *Dest = '%s'\n", *Dest);
	
	return pos;
}
