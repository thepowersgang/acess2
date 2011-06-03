/*
 * Acess GUI (AxWin) Version 2
 * By John Hodge (thePowersGang)
 */
#include "common.h"
#include <string.h>

// === PROTOTYPES ===
void	ShowUsage(char *ProgName);
void	ShowHelp(char *ProgName);

// === CODE ===
void ParseCommandline(int argc, char *argv[])
{
	 int	i;
	char	*arg;
	
	for( i = 1; i < argc; i++ )
	{
		arg = argv[i];
		if(arg[0] == '-')
		{
			if( arg[1] == '-' )
			{
				if( strcmp(&arg[2], "help") == 0 ) {
					ShowHelp(argv[0]);
					exit(EXIT_SUCCESS);
				}
				else {
					ShowUsage(argv[0]);
					exit(EXIT_FAILURE);
				}
			}
			else
			{
				while( *++arg )
				{
					switch(*arg)
					{
					case 'h':
					case '?':
						ShowHelp(argv[0]);
						exit(EXIT_SUCCESS);
						break;
					default:
						break;
					}
				}
			}
		}
	}
}

void ShowUsage(char *ProgName)
{
	fprintf(stderr, "Usage: %s [-h|--help]\n", ProgName);
}

void ShowHelp(char *ProgName)
{
	ShowUsage(ProgName);
	fprintf(stderr, "\n");
	fprintf(stderr, "\t--help\tShow this message\n");
}
