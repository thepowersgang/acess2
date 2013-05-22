/*
 * Acess GUI Terminal
 * - By John Hodge (thePowersGang)
 *
 * vt100.c
 * - VT100/xterm Emulation
 */
#include <string.h>
#include <limits.h>
#include "include/vt100.h"
#include "include/display.h"
#include <ctype.h>	// isalpha
#include <acess/sys.h>	// _SysDebug

const uint32_t	caVT100Colours[] = {
	// Black, Red, Green, Yellow, Blue, Purple, Cyan, Gray
	// Same again, but bright
	0x000000, 0x770000, 0x007700, 0x777700, 0x000077, 0x770077, 0x007777, 0xAAAAAAA,
	0xCCCCCC, 0xFF0000, 0x00FF00, 0xFFFF00, 0x0000FF, 0xFF00FF, 0x00FFFF, 0xFFFFFFF
};

 int	Term_HandleVT100_Long(int Len, const char *Buf);

static inline int min(int a, int b)
{
	return a < b ? a : b;
}

int Term_HandleVT100(int Len, const char *Buf)
{
	#define	MAX_VT100_ESCAPE_LEN	16
	static char	inc_buf[MAX_VT100_ESCAPE_LEN];
	static int	inc_len = 0;

	if( inc_len > 0	|| *Buf == '\x1b' )
	{
		// Handle VT100 (like) escape sequence
		 int	new_bytes = min(MAX_VT100_ESCAPE_LEN - inc_len, Len);
		 int	ret = 0, old_inc_len = inc_len;
		memcpy(inc_buf + inc_len, Buf, new_bytes);

		inc_len += new_bytes;

		if( inc_len <= 1 )
			return 1;	// Skip 1 character (the '\x1b')

		switch(inc_buf[1])
		{
		case '[':	// Multibyte, funtime starts	
			ret = Term_HandleVT100_Long(inc_len-2, inc_buf+2);
			if( ret > 0 ) {
				ret += 2;
			}
			break;
		default:
			ret = 2;
			break;
		}	

		if( ret != 0 ) {
			inc_len = 0;
			ret -= old_inc_len;	// counter cached bytes
		}
		return ret;
	}

	switch( *Buf )
	{
	case '\b':
		Display_MoveCursor(-1, 0);
		Display_AddText(1, " ");
		Display_MoveCursor(-1, 0);
		// TODO: Need to handle \t and ^A-Z
		return 1;
	case '\t':
		// TODO: tab (get current cursor pos, space until multiple of 8)
		return 1;
	case '\n':
		Display_Newline(1);
		return 1;
	case '\r':
		Display_MoveCursor(INT_MIN, 0);
		return 1;
	}

	 int	ret = 0;
	while( ret < Len )
	{
		if( *Buf == '\n' )
			break;
		if( *Buf == '\x1b' )
			break;
		ret ++;
		Buf ++;
	}
	return -ret;
}

int Term_HandleVT100_Long(int Len, const char *Buffer)
{
	char	c;
	 int	argc = 0, j = 0;
	 int	args[6] = {0,0,0,0,0,0};
	 int	bQuestionMark = 0;
	
	// Get Arguments
	if(j == Len)	return 0;
	c = Buffer[j++];
	if(c == '?') {
		bQuestionMark = 1;
		if(j == Len)	return 0;
		c = Buffer[j++];
	}
	if( '0' <= c && c <= '9' )
	{
		do {
			if(c == ';') {
				if(j == Len)	return 0;
				c = Buffer[j++];
			}
			while('0' <= c && c <= '9') {
				args[argc] *= 10;
				args[argc] += c-'0';
				if(j == Len)	return 0;
				c = Buffer[j++];
			}
			argc ++;
		} while(c == ';');
	}
	
	// Get Command
	if( !isalpha(c) ) {
		// Bother.
		_SysDebug("Unexpected char 0x%x in VT100 escape code", c);
		return 1;
	}

	if( bQuestionMark )
	{
		// Special commands
		switch( c )
		{
		default:
			_SysDebug("Unknown VT100 extended escape char 0x%x", c);
			break;
		}
	}
	else
	{
		// Standard commands
		switch( c )
		{
		case 'J':
			if( argc == 0 )
				Display_ClearLine(0);
			else if( args[0] == 2 )
				Display_ClearLines(0);	// Entire screen!
			else
				_SysDebug("TODO: VT100 %i J", args[0]);
			break;
		case 'm':
			if( argc == 0 )
			{
				// Reset
			}
			else
			{
				int i;
				for( i = 0; i < argc; i ++ )
				{
					if( args[i] < 8 )
					{
						// TODO: Flags?
					}
					else if( 30 <= args[i] && args[i] <= 37 )
					{
						// TODO: Bold/bright
						Display_SetForeground( caVT100Colours[ args[i]-30 ] );
					} 
					else if( 40 <= args[i] && args[i] <= 47 )
					{
						// TODO: Bold/bright
						Display_SetBackground( caVT100Colours[ args[i]-30 ] );
					} 
				}
			}
			break;
		default:
			_SysDebug("Unknown VT100 escape char 0x%x", c);
			break;
		}
	}
	return j;
}
