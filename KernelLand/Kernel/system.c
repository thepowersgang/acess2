/*
 * Acess 2 Kernel
 * - By John Hodge (thePowersGang)
 * system.c
 * - Architecture Independent System Init
 */
#define DEBUG	0
#include <acess.h>
#include <hal_proc.h>

// === IMPORTS ===
extern void	Arch_LoadBootModules(void);
extern int	Modules_LoadBuiltins(void);
extern void	Modules_SetBuiltinParams(char *Name, char *ArgString);
extern void	Debug_SetKTerminal(const char *File);
extern void	Timer_CallbackThread(void *);

// === PROTOTYPES ===
void	System_Init(char *Commandline);
void	System_ParseCommandLine(char *ArgString);
void	System_ExecuteCommandLine(void);
void	System_ParseVFS(char *Arg);
void	System_ParseModuleArgs(char *Arg);
void	System_ParseSetting(char *Arg);

// === GLOBALS ===
const char	*gsInitBinary = "/Acess/SBin/init";
char	*argv[32];
 int	argc;

// === CODE ===
void System_Init(char *CommandLine)
{
	Proc_SpawnWorker(Timer_CallbackThread, NULL);

	// Parse Kernel's Command Line
	System_ParseCommandLine(CommandLine);
	
	// Initialise modules
	Log_Log("Config", "Initialising builtin modules...");
	Modules_LoadBuiltins();
	Arch_LoadBootModules();
	
	System_ExecuteCommandLine();
	
	// - Execute the Config Script
	Log_Log("Config", "Spawning init '%s'", gsInitBinary);
	if(Proc_Clone(CLONE_VM|CLONE_NOUSER) == 0)
	{
		const char	*args[] = {gsInitBinary, 0};
		Proc_Execve(gsInitBinary, args, &args[1], 0);
		Log_KernelPanic("System", "Unable to spawn init '%s'", gsInitBinary);
	}
	
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
	for( i = 1; i < argc; i++ )
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
	if(argc > 0)
		LOG("Invocation '%s'", argv[0]);
	for( i = 1; i < argc; i++ )
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
//		Log_Log("Config", "Symbolic link '%s' pointing to '%s'", Arg, value);
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
//			Log_Log("Config", "Creating directory '%s'", Arg, value);
			VFS_MkDir( Arg );
		} else {
			VFS_Close(fd);
		}
		// Mount
//		Log_Log("Config", "Mounting '%s' to '%s' ('%s')", dev, Arg, value);
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
		
		if(strcmp(Arg, "INIT") == 0) {
			Log_Log("Config", "Init binary: '%s'", value);
			if(strlen(value) == 0)
				gsInitBinary = NULL;
			else
				gsInitBinary = value;
		}
		else {
			Log_Warning("Config", "Kernel config setting '%s' is not recognised", Arg);
		}
		
	}
}

