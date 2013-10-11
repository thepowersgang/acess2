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
#include <string.h>	// strrchr
#include <assert.h>
#include "include/common.h"
#include "include/build.h"
#include "include/inifile.h"
#include "include/udiprops.h"

#define CONFIG_FILENAME	"udibuild.ini"
#ifdef __ACESS__
#define RUNTIME_DIR	"/Acess/Conf/UDI"
#else
#define RUNTIME_DIR	"/etc/udi"
#endif

// === PROTOTYPES ===
 int	main(int argc, char *argv[]);
 int	ParseArguments(int argc, char *argv[]);
void	Usage(const char *progname);

// === GLOBALS ===
const char *gsRuntimeDir = RUNTIME_DIR;
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
	// 1. Check CWD
	if( !gsOpt_ConfigFile ) {
		//if( file_exists("./udibuild.ini") )
		//{
		//	gsOpt_ConfigFile = "udibuild.ini";
		//}
	}
	// 2. Check program dir (if not invoked from PATH)
	if( !gsOpt_ConfigFile && (argv[0][0] == '.' || argv[0][0] == '/') ) {
		char *last_slash = strrchr(argv[0], '/');
		if( last_slash ) {
			gsOpt_ConfigFile = mkstr("%.*s/%s",
				last_slash-argv[0], argv[0], CONFIG_FILENAME);
		}
		//if( !file_exists(gsOpt_ConfigFile) ) {
		//	free(gsOpt_ConfigFile);
		//	gsOpt_ConfigFile = NULL;
		//}
	}
	// 3. Check ~/.config/udi/udibuild.ini
	// 4. Check RUNTIME_DIR/udibuild.ini

	// #. Oh well	
	if( !gsOpt_ConfigFile ) {
		fprintf(stderr, "Can't locate "CONFIG_FILENAME" file, please specify using '-c'\n");
		exit(2);
	}
	
	// Load udibuild.ini
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
	for( int i = 0; i < gpUdipropsBuild->nSourceFiles; i ++ )
	{
		int rv = Build_CompileFile(gpOptions, gsOpt_ABIName, gpUdipropsBuild,
			gpUdipropsBuild->SourceFiles[i]);
		if( rv ) {
			fprintf(stderr, "*** Exit status: %i\n", rv);
			return rv;
		}
	}
	// Create file with `.udiprops` section
	// - udimkpkg's job
	//Build_CreateUdiprops(gpOptions, gsOpt_ABIName, gpUdipropsBuild);
	// Link
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

char *mkstr(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	size_t len = vsnprintf(NULL, 0, fmt, args);
	va_end(args);
	va_start(args, fmt);
	char *ret = malloc(len+1);
	vsnprintf(ret, len+1, fmt, args);
	va_end(args);
	return ret;
}

