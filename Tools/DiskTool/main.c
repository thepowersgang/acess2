/*
 */
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <disktool_common.h>

#define ACESS_VERSION	"0.15-pr"
#define VERSION_STR	"Acess2 DiskTool v"ACESS_VERSION

 int	main(int argc, const char *argv[]);
void	PrintUsage(void);
 int	RunCommand(int argc, const char *argv[]);
void	RunScript(const char *Filename);

// === CODE ===
int main(int argc, const char *argv[])
{
	// Parse arguments
	for( int i = 1; i < argc; i ++ )
	{
		const char *arg = argv[i];
		if( arg[0] != '-' )
		{
			int rv = RunCommand(argc-i, argv+i);
			if( rv == 0 ) {
				PrintUsage();
				return 1;
			}
			else if( rv < 0 ) {
				return 0;
			}
			else {
			}
			i += rv;
		}
		else if( arg[1] != '-' )
		{
			switch( *++arg )
			{
			case 's':
				if( i+1 >= argc ) {
					fprintf(stderr, "Option '-s' requires an argument\n");
					return 1;
				}
				RunScript( argv[++i] );
				break;
			default:
				fprintf(stderr, "Unkown option '-%c', try --help\n", *arg);
				return 1;
			}
		}
		else
		{
			if( strcmp(arg, "--help") == 0 ) {
				PrintUsage();
				return 0;
			}
			else if( strcmp(arg, "--version") == 0 ) {
				fprintf(stderr, VERSION_STR);
				return 0;
			}
			else {
				fprintf(stderr, "Unknown option '%s', try --help\n", arg);
				return 1;
			}
		}
	}
	
	DiskTool_Cleanup();
	
	return 0;
}

void PrintUsage(void)
{
	fprintf(stderr,
		"Usage:\n"
	       	" disktool <commands...>\n"
		"\n"
		"Commands:\n"
		" lvm <image> <ident>\n"
		" - Register an image with LVM\n"
		"  e.g.\n"
		"   `lvm ../AcessHDD.img HDD`\n"
		" mount <image> <mountname>\n"
		" - Bind an image to a name.\n"
		"  e.g.\n"
	       	"   `mount ../AcessFDD.img FDD`\n"
		"   `mount :HDD/0 hda1`\n"
		" ls <dir>\n"
		" - List a directory\n"
		"  e.g.\n"
		"   `ls ../`\n"
		"   `ls FDD:/`\n"
		"\n"
		);
}

int RunCommand(int argc, const char *argv[])
{
	if( argc < 1 )	return 0;
	
	const char *name = argv[0];
	if( strcmp("mount", name) == 0 )
	{
		// Mount an image
		if( argc < 3 ) {
			fprintf(stderr, "mount takes 2 arguments (image and mountpoint)\n");
			return 0;
		}

		if( DiskTool_MountImage(argv[2], argv[1]) ) {
			fprintf(stderr, "Unable to mount '%s' as '%s'\n", argv[1], argv[2]);
			return -1;
		}

		return 2;
	}
	else if( strcmp("lvm", name) == 0 )
	{
		// Bind a "file" to LVM
		if( argc < 3 ) {
			fprintf(stderr, "lvm takes 2 arguments (iamge and ident)\n");
			return 0;
		}

		if( DiskTool_RegisterLVM(argv[2], argv[1]) ) {
			fprintf(stderr, "Unable to register '%s' as LVM '%s'\n", argv[1], argv[2]);
			return -1;
		}
		
		return 2;
	}
	else if( strcmp("ls", name) == 0 )
	{
		if( argc < 2 ) {
			fprintf(stderr, "ls takes 1 argument (path)\n");
			return 0;
		}

		DiskTool_ListDirectory(argv[1]);
		return 1;
	}
	else if( strcmp("cp", name) == 0 )
	{
		
		if( argc < 3 ) {
			fprintf(stderr, "cp takes 2 arguments (source and destination)\n");
			return 0;
		}

		DiskTool_Copy(argv[1], argv[2]);

		return 2;
	}
	else if( strcmp("cat", name) == 0 ) {

		if( argc < 2 ) {
			fprintf(stderr, "cat takes 1 argument (path)\n");
			return 0;
		}

		DiskTool_Cat(argv[1]);

		return 1;
	}
	else {
		fprintf(stderr, "Unknown command '%s'\n", name);
		return 0;
	}
}

int tokenise(const char **ptrs, int max_ptrs, char *buffer)
{
	 int	idx = 0;
	while( *buffer )
	{
		// Eat leading whitespace
		while( *buffer && isspace(*buffer) )
			buffer ++;
		if( *buffer == '"' ) {
			// Double-quoted string
			buffer ++;
			ptrs[idx++] = buffer;
			while( *buffer && *buffer != '"' )
			{
				if( *buffer == '\\' && buffer[1] == '"' ) {
					char *tmp = buffer;
					while( tmp[1] ) {
						tmp[0] = tmp[1];
						tmp ++;
					}
				}
				buffer ++;
			}
			if( *buffer )
				*buffer++ = '\0';
		}
		else {
			// whitespace delimited string
			ptrs[idx++] = buffer;
			while( *buffer && !isspace(*buffer) )
				buffer ++;
			if( *buffer )
				*buffer++ = '\0';
		}
	}
	return idx;
}

void RunScript(const char *Filename)
{
	FILE	*fp = fopen(Filename, "r");
	if( !fp ) {
		fprintf(stderr, "Unable to open script '%s': %s\n", Filename, strerror(errno));
		exit(1);
	}

	 int	 line = 0;	
	char	buf[128];
	while( NULL != fgets(buf, sizeof(buf), fp) )
	{
		line ++;
		const int max_tokens = 4;
		const char *tokens[max_tokens];
		int ntok = tokenise(tokens, max_tokens, buf);
		
		if( ntok > max_tokens ) {
			// ... 
			break ;
		}
		
		int rv = RunCommand(ntok, tokens);
		assert(rv + 1 <= ntok);
		if( rv == 0 ) {
			// Oops, bad command
			break;
		}
		else if( rv == -1 ) {
			// Internal error
			break;
		}
		else if( rv + 1 != ntok ) {
			// Too many arguments
			break;
		}
	}
	
	fclose(fp);
}

// NOTE: This is in a native compiled file because it needs access to the real errno macro
int *Threads_GetErrno(void)
{
	return &errno;
}

size_t _fwrite_stdout(size_t bytes, const void *data)
{
	return fwrite(data, bytes, 1, stdout);
}
