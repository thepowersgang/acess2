/*
 * udibuild - UDI Compilation Utility
 * - By John Hodge (thePowersGang)
 *
 * main.c
 * - Core
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>	// getopt
#include <getopt.h>
#include <assert.h>
#include "include/build.h"
#include "include/inifile.h"
#include "include/udiprops.h"

#ifdef __ACESS__
#define CONFIG_FILE	"/Acess/Conf/UDI/udibuild.ini"
#else
#define CONFIG_FILE	"/etc/udi/udibuild.ini"
#endif

// === PROTOTYPES ===
 int	main(int argc, char *argv[]);
 int	ParseArguments(int argc, char *argv[]);
void	Usage(const char *progname);

// === GLOBALS ===
const char *gsOpt_ConfigFile;
const char *gsOpt_WorkingDir;
const char *gsOpt_UdipropsFile;
const char *gsOpt_ABIName = "ia32";
tIniFile	*gpOptions;
tUdiprops	*gpUdipropsBuild;

// === CODE ===
int main(int argc, char *argv[])
{
	if( ParseArguments(argc, argv) ) {
		return 1;
	}

	// Locate udibuild.ini
	if( NULL == gsOpt_ConfigFile )
	{
		// 1. Check CWD
		//if( file_exists("./udibuild.ini") )
		//{
		//	gsOpt_ConfigFile = "udibuild.ini";
		//}
		// 2. Check program dir (if not invoked from PATH)
		// 3. Check ~/.config/udi/udibuild.ini
		// 4. Check CONFIGNAME
		
		exit(2);
	}
	gpOptions = IniFile_Load(gsOpt_ConfigFile);
	assert(gpOptions);

	// Change to working directory (-C <dir>)
	if( gsOpt_WorkingDir )
	{
		chdir(gsOpt_WorkingDir);
	}

	// Load udiprops
	gpUdipropsBuild = Udiprops_LoadBuild( gsOpt_UdipropsFile ? gsOpt_UdipropsFile : "udiprops.txt" );
	assert(gpUdipropsBuild);
	assert(gpUdipropsBuild->SourceFiles);

	// Do build
	for( int i = 0; gpUdipropsBuild->SourceFiles[i]; i ++ )
	{
		int rv = Build_CompileFile(gpOptions, gsOpt_ABIName, gpUdipropsBuild,
			gpUdipropsBuild->SourceFiles[i]);
		if( rv ) {
			fprintf(stderr, "*** Exit status: %i\n", rv);
			return rv;
		}
	}
	Build_LinkObjects(gpOptions, gsOpt_ABIName, gpUdipropsBuild);

	return 0;
}

int ParseArguments(int argc, char *argv[])
{
	 int	opt;
	while( (opt = getopt(argc, argv, "hC:c:f:a:")) != -1 )
	{
		switch(opt)
		{
		case 'h':
			Usage(argv[0]);
			exit(0);
		case 'C':
			gsOpt_WorkingDir = optarg;
			break;
		case 'c':
			gsOpt_ConfigFile = optarg;
			break;
		case 'f':
			gsOpt_UdipropsFile = optarg;
			break;
		case 'a':
			gsOpt_ABIName = optarg;
			break;
		case '?':
			Usage(argv[0]);
			return 1;
		default:
			fprintf(stderr, "BUG: Unhandled optarg %i '%c'\n", opt, opt);
			break;
		}
	}
	return 0;
}

void Usage(const char *progname)
{
	fprintf(stderr, "Usage: %s [-C workingdir] [-c udibuild.ini] [-f udiprops.txt] [-a abiname]\n",
		progname);
	fprintf(stderr, "\n"
		"-C workingdir   : Change to the specified directory before looking for udiprops.txt\n"
		"-c udibuild.ini : Override the default udibuild config file\n"
		"-f udiprops.txt : Override the default udiprops file\n"
		"-a abiname      : Select a different ABI\n"
		"\n");
}

