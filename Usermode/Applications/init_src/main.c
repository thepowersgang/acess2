/*
 * Acess2 System Init Task
 */
#include <acess/sys.h>
#include <stdlib.h>
#include <stdio.h>
#include <spiderscript.h>
//#include "common.h"

// === CONSTANTS ===
#define NULL	((void*)0)
#define NUM_TERMS	4
#define	DEFAULT_TERMINAL	"/Devices/VTerm/0"
#define DEFAULT_SHELL	"/Acess/SBin/login"
#define DEFAULT_SCRIPT	"/Acess/Conf/BootConf.isc"

#define ARRAY_SIZE(x)	((sizeof(x))/(sizeof((x)[0])))

// === PROTOTYPES ===
tSpiderVariable	*Script_System_IO_Open(tSpiderScript *, int, tSpiderVariable *);

// === GLOBALS ===
tSpiderFunction	gaScriptNS_IO_Fcns[] = {
	{"Open", Script_System_IO_Open}
};
tSpiderNamespace	gaScriptNS_System[] = {
	{
		"IO",
		0, NULL,
		ARRAY_SIZE(gaScriptNS_IO_Fcns), gaScriptNS_IO_Fcns,
		0, NULL
	}
};

tSpiderNamespace	gaScriptNamespaces[] = {
	{
		"System",
		ARRAY_SIZE(gaScriptNS_System), gaScriptNS_System,
		0, NULL,
		0, NULL
	}
};

tSpiderVariant	gScriptVariant = {
	"init", 0,
	ARRAY_SIZE(gaScriptNamespaces), gaScriptNamespaces
};

// === CODE ===
/**
 * \fn int main(int argc, char *argv[])
 * \brief Entrypoint
 */
int main(int argc, char *argv[])
{
	 int	tid;
	 int	i;
	char	termpath[sizeof(DEFAULT_TERMINAL)] = DEFAULT_TERMINAL;
	char	*child_argv[2] = {DEFAULT_SHELL, 0};
	
	// - Parse init script
	
	// - Start virtual terminals
	for( i = 0; i < NUM_TERMS; i++ )
	{		
		tid = clone(CLONE_VM, 0);
		if(tid == 0)
		{
			termpath[sizeof(DEFAULT_TERMINAL)-2] = '0' + i;
			
			open(termpath, OPENFLAG_READ);	// Stdin
			open(termpath, OPENFLAG_WRITE);	// Stdout
			open(termpath, OPENFLAG_WRITE);	// Stderr
			execve(DEFAULT_SHELL, child_argv, NULL);
			for(;;)	sleep();
		}
	}
	
	// TODO: Implement message watching
	for(;;)	sleep();
	
	return 42;
}

/**
 * \brief Reads and parses the boot configuration script
 * \param Filename	File to parse and execute
 */
void ExecuteScript(const char *Filename)
{
	tSpiderScript	*script;
	script = SpiderScript_ParseFile(&gScriptVariant, Filename);
	SpiderScript_ExecuteMethod(script, "");
	SpiderScript_Free(script);
}

/**
 * \brief Open a file
 */
tSpiderVariable	*Script_System_IO_Open(tSpiderScript *Script, int NArgs, tSpiderVariable *Args)
{
	return NULL;
}
