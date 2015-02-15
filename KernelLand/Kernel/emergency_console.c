/*
 * Acess 2 Kernel
 * - By John Hodge (thePowersGang)
 * 
 * emergency_console.c
 * - Kernel-land emergency console/shell
 */
#include <acess.h>
#include <stdarg.h>

#define STDIN	0
#define STDOUT	1
#define STDERR	2

// === PROTOTYPES ===
void EmergencyConsole(void);
// -- Commands
void Command_ls(const char* path);
void Command_hd(const char* path);
void Command_mount(const char *fs, const char *dev, const char *point);
// --
static char **split_args(char *line);
static char *read_line(int fd);
static int dprintf(int fd, const char *fmt, ...);// __attribute__((printf(2,3)));

// === CODE ===
void EmergencyConsole(void)
{
	for(;;)
	{
		dprintf(STDOUT, "(kernel)$ ");
		char *line = read_line(STDIN);
		
		// Explode line into args
		char **args = split_args(line);
		// Get command from first arg
		ASSERT(args);
		ASSERT(args[0]);
		if( strcmp(args[0], "help") == 0 ) {
			dprintf(STDOUT,
				"Commands:\n"
				"ls <path> - List the contents of a directory\n"
				"hd <path> - Dump a file in a 16 bytes/line hex format\n"
				"mount <fs> <device> <point> - Mount a filesystem\n"
				);
		}
		else if( strcmp(args[0], "ls") == 0 ) {
			Command_ls(args[1]); 
		}
		else if( strcmp(args[0], "hd") == 0 ) {
			Command_hd(args[1]);
		}
		else if( strcmp(args[0], "mount") == 0 ) {
			Command_mount(args[1], args[2], args[3]);
		}
		else
		{
			dprintf(STDERR, "Unknown command '%s'\n", args[0]);
		}
		// Free args
		free(args);
		free(line);
	}
}

void Command_ls(const char* path)
{
	dprintf(STDERR, "ls: TODO - Implement\n");
}
void Command_hd(const char* path)
{
	dprintf(STDERR, "hd: TODO - Implement\n");
}
void Command_mount(const char *fs, const char *dev, const char *point)
{
	dprintf(STDERR, "mount: TODO - Implement\n");
}

// Allocates return array (NUL terminated), but mangles input string
static int split_args_imp(char *line, char **buf)
{
	 int	argc = 0;
	 int	pos = 0;
	enum {
		MODE_SPACE,
		MODE_NORM,
		MODE_SQUOTE,
		MODE_DQUOTE,
	} mode = MODE_SPACE;
	for( char *chp = line; *chp; chp ++ )
	{
		if( *chp == ' ' ) {
			if( mode != MODE_SPACE ) {
				if(buf) buf[argc][pos] = '\0';
				argc ++;
			}
			mode = MODE_SPACE;
			continue ;
		}
		
		switch( mode )
		{
		case MODE_SPACE:
			if( buf )
				buf[argc] = chp;
			pos = 0;
		case MODE_NORM:
			switch( *chp )
			{
			case '"':
				mode = MODE_DQUOTE;
				break;
			case '\'':
				mode = MODE_SQUOTE;
				break;
			case '\\':
				chp ++;
				switch( *chp )
				{
				case '\0':
					dprintf(STDERR, "err: Trailing '\\'\n");
					break;
				case '\\':
				case '\'':
				case '"':
					if(buf)	buf[argc][pos++] = *chp;
					break;
				default:
					dprintf(STDERR, "err: Unkown escape '%c'\n", *chp);
					break;
				}
				break;
			default:
				if(buf)	buf[argc][pos++] = *chp;
				break;
			}
			break;
		case MODE_SQUOTE:
			switch( *chp )
			{
			case '\'':
				mode = MODE_NORM;
				break;
			case '\0':
				dprintf(STDERR, "err: Unterminated \' string\n");
				break;
			default:
				if(buf)	buf[argc][pos++] = *chp;
				break;
			}
			break;
		case MODE_DQUOTE:
			switch( *chp )
			{
			case '\"':
				mode = MODE_NORM;
				break;
			case '\\':
				chp ++;
				switch(*chp)
				{
				case '\0':
					dprintf(STDERR, "err: Trailing '\\' in \" string\n");
					break;
				case '\\':
				case '"':
					if(buf)	buf[argc][pos++] = *chp;
					break;
				default:
					dprintf(STDERR, "err: Unkown escape '%c'\n", *chp);
					break;
				}
				break;
			case '\0':
				dprintf(STDERR, "err: Unterminated \" string\n");
				break;
			default:
				if(buf)	buf[argc][pos++] = *chp;
				break;
			}
			break;
		}
	}

	if(buf) buf[argc][pos++] = '\0';
	argc ++;
	return argc;
}
static char **split_args(char *line)
{
	// 1. Count
	int count = split_args_imp(line, NULL);
	char **ret = malloc( (count + 1) * sizeof(char*) );
	
	split_args_imp(line, ret);
	ret[count] = NULL;
	return ret;
}

static char *read_line(int fd)
{
	// Read line (or up to ~128 bytes)
	// - This assumes a default PTY state (i.e. line buffered, echo on)
	char *ret = malloc(128);
	size_t len = VFS_Read(STDIN, 128, ret);
	ret[len] = 0;
	return ret;
}

static int dprintf(int fd, const char *fmt, ...)
{
	va_list	args;
	va_start(args, fmt);
	size_t len = vsnprintf(NULL, 0, fmt, args);
	va_end(args);
	
	char buf[len+1];
	va_start(args, fmt);
	vsnprintf(buf, len+1, fmt, args);
	va_end(args);
	
	VFS_Write(fd, len, buf);
	
	return len;
}

