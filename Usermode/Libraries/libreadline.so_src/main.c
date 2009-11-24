/*
 * Acess2 Library Suite
 * - Readline
 * 
 * Text mode entry with history
 */
#include <readline.h>
#include <stdlib.h>

// === GLOBALS ===

// === CODE ===
char *Readline(tReadline *Info)
{
	char	*ret;
	 int	len, pos, space = 1023-8-8;	// optimised for the heap manager
	char	ch;
	 int	scrollbackPos = Info->NumHistory;
	 
	// Preset Variables
	ret = malloc( space+1 );
	if(!ret)	return NULL;
	len = 0;	pos = 0;
		
	// Read In Command Line
	do {
		read(_stdin, 1, &ch);	// Read Character from stdin (read is a blocking call)
		
		if(ch == '\n')	break;
		
		switch(ch)
		{
		// Control characters
		case '\x1B':
			read(_stdin, 1, &ch);	// Read control character
			switch(ch)
			{
			//case 'D':	if(pos)	pos--;	break;
			//case 'C':	if(pos<len)	pos++;	break;
			case '[':
				read(_stdin, 1, &ch);	// Read control character
				switch(ch)
				{
				case 'A':	// Up
					{
						 int	oldLen = len;
						if( scrollbackPos <= 0 )	break;
						
						free(ret);
						ret = strdup( Info->History[--scrollbackPos] );
						
						len = strlen(ret);
						while(pos--)	write(_stdout, 3, "\x1B[D");
						write(_stdout, len, ret);	pos = len;
						while(pos++ < oldLen)	write(_stdout, 1, " ");
					}
					break;
				case 'B':	// Down
					{
						 int	oldLen = len;
						if( scrollbackPos >= Info->NumHistory )	break;
						
						free(ret);
						ret = strdup( Info->History[scrollbackPos++] );
						
						len = strlen(ret);
						while(pos--)	write(_stdout, 3, "\x1B[D");
						write(_stdout, len, ret);	pos = len;
						while(pos++ < oldLen)	write(_stdout, 1, " ");
					}
					break;
				case 'D':	// Left
					if(pos == 0)	break;
					pos --;
					write(_stdout, 3, "\x1B[D");
					break;
				case 'C':	// Right
					if(pos == len)	break;
					pos++;
					write(_stdout, 3, "\x1B[C");
					break;
				}
			}
			break;
		
		// Backspace
		case '\b':
			if(len <= 0)		break;	// Protect against underflows
			write(_stdout, 1, &ch);
			if(pos == len) {	// Simple case of end of string
				len --;
				pos--;
			}
			else {
				char	buf[7] = "\x1B[000D";
				buf[2] += ((len-pos+1)/100) % 10;
				buf[3] += ((len-pos+1)/10) % 10;
				buf[4] += (len-pos+1) % 10;
				write(_stdout, len-pos, &ret[pos]);	// Move Text
				ch = ' ';	write(_stdout, 1, &ch);	ch = '\b';	// Clear deleted character
				write(_stdout, 7, buf);	// Update Cursor
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
				ret = realloc(ret, space+1);
				if(!ret)	return NULL;
			}
			
			// Editing inside the buffer
			if(pos != len) {
				char	buf[7] = "\x1B[000D";
				buf[2] += ((len-pos)/100) % 10;
				buf[3] += ((len-pos)/10) % 10;
				buf[4] += (len-pos) % 10;
				write(_stdout, 1, &ch);	// Print new character
				write(_stdout, len-pos, &ret[pos]);	// Move Text
				write(_stdout, 7, buf);	// Update Cursor
				memmove( &ret[pos+1], &ret[pos], len-pos );
			}
			else {
				write(_stdout, 1, &ch);
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
		if( strcmp( Info->History[ Info->NumHistory-1 ], ret) != 0 )
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
	
	return ret;
}
