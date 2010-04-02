/*
 * Acess2 Library Suite
 * - Readline
 * 
 * Text mode entry with history
 */
#include <readline.h>
#include <acess/sys.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define STDIN_FD	0
#define STDOUT_FD	1

// === PROTOTYPES ===
 int	SoMain();
tReadline	*Readline_CreateInstance(int bUseHistory);
char	*Readline(tReadline *Info);

// === GLOBALS ===

// === CODE ===
int SoMain()
{
	return 0;
}

char *Readline(tReadline *Info)
{
	char	*ret;
	char	*orig;
	 int	len, pos, space = 1023-8-8;	// optimised for the heap manager
	char	ch;
	 int	scrollbackPos = Info->NumHistory;
	 
	// Preset Variables
	ret = malloc( space+1 );
	if(!ret)	return NULL;
	len = 0;	pos = 0;
	
	orig = ret;
	
	// Read In Command Line
	do {
		read(STDIN_FD, 1, &ch);	// Read Character from stdin (read is a blocking call)
		
		if(ch == '\n')	break;
		
		switch(ch)
		{
		// Control characters
		case '\x1B':
			read(STDIN_FD, 1, &ch);	// Read control character
			switch(ch)
			{
			//case 'D':	if(pos)	pos--;	break;
			//case 'C':	if(pos<len)	pos++;	break;
			case '[':
				read(STDIN_FD, 1, &ch);	// Read control character
				switch(ch)
				{
				case 'A':	// Up
					{
						 int	oldLen = len;
						if( scrollbackPos <= 0 )	break;
						
						if(ret != orig)	free(ret);
						ret = strdup( Info->History[--scrollbackPos] );
						
						space = len = strlen(ret);
						while(pos-->1)	write(STDOUT_FD, 3, "\x1B[D");
						write(STDOUT_FD, len, ret);	pos = len;
						while(pos++ < oldLen)	write(STDOUT_FD, 1, " ");
					}
					break;
				case 'B':	// Down
					{
						 int	oldLen = len;
						if( scrollbackPos >= Info->NumHistory )	break;
						
						if(ret != orig)	free(ret);
						ret = strdup( Info->History[scrollbackPos++] );
						
						space = len = strlen(ret);
						while(pos-->1)	write(STDOUT_FD, 3, "\x1B[D");
						write(STDOUT_FD, len, ret);	pos = len;
						while(pos++ < oldLen)	write(STDOUT_FD, 1, " ");
					}
					break;
				case 'D':	// Left
					if(pos == 0)	break;
					pos --;
					write(STDOUT_FD, 3, "\x1B[D");
					break;
				case 'C':	// Right
					if(pos == len)	break;
					pos++;
					write(STDOUT_FD, 3, "\x1B[C");
					break;
				}
			}
			break;
		
		// Backspace
		case '\b':
			if(len <= 0)		break;	// Protect against underflows
			write(STDOUT_FD, 1, &ch);
			if(pos == len) {	// Simple case of end of string
				len --;
				pos--;
			}
			else {
				char	buf[7] = "\x1B[000D";
				buf[2] += ((len-pos+1)/100) % 10;
				buf[3] += ((len-pos+1)/10) % 10;
				buf[4] += (len-pos+1) % 10;
				write(STDOUT_FD, len-pos, &ret[pos]);	// Move Text
				ch = ' ';	write(STDOUT_FD, 1, &ch);	ch = '\b';	// Clear deleted character
				write(STDOUT_FD, 7, buf);	// Update Cursor
				// Alter Buffer
				memmove(&ret[pos-1], &ret[pos], len-pos);
				pos --;
				len --;
			}
			break;
		
		// Tab
		case '\t':
			//TODO: Implement Tab-Completion
			//Currently just ignore tabs
			break;
		
		default:		
			// Expand Buffer
			if(len+1 > space) {
				space += 256;
				if(ret == orig) {
					orig = ret = realloc(ret, space+1);
				}
				else {
					ret = realloc(ret, space+1);
				}
				if(!ret)	return NULL;
			}
			
			// Editing inside the buffer
			if(pos != len) {
				char	buf[7] = "\x1B[000D";
				buf[2] += ((len-pos)/100) % 10;
				buf[3] += ((len-pos)/10) % 10;
				buf[4] += (len-pos) % 10;
				write(STDOUT_FD, 1, &ch);	// Print new character
				write(STDOUT_FD, len-pos, &ret[pos]);	// Move Text
				write(STDOUT_FD, 7, buf);	// Update Cursor
				memmove( &ret[pos+1], &ret[pos], len-pos );
			}
			else {
				write(STDOUT_FD, 1, &ch);
			}
			ret[pos++] = ch;
			len ++;
			break;
		}
	} while(ch != '\n');
	
	// Cap String
	ret[len] = '\0';
	printf("\n");
	
	// Return length
	//if(Length)	*Length = len;
	
	// Add to history
	if( Info->UseHistory )
	{
		if( !Info->History || strcmp( Info->History[ Info->NumHistory-1 ], ret) != 0 )
		{
			void	*tmp;
			Info->NumHistory ++;
			tmp = realloc( Info->History, Info->NumHistory * sizeof(char*) );
			if(tmp != NULL)
			{
				Info->History = tmp;
				Info->History[ Info->NumHistory-1 ] = strdup(ret);
			}
		}
	}
	
	if(ret != orig)	free(orig);
	
	return ret;
}
