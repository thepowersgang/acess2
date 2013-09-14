/*
 * Acess GUI Terminal
 * - By John Hodge (thePowersGang)
 *
 * vt100.c
 * - VT100/xterm Emulation
 */
#include <limits.h>
#include "include/vt100.h"
#include "include/display.h"
#include <ctype.h>	// isalpha
#ifdef KERNEL_VERSION
# define _SysDebug(v...)	Debug("VT100 "v)
#else
# include <acess/sys.h>	// _SysDebug
# include <string.h>
# include <assert.h>
#endif

const uint32_t	caVT100Colours[] = {
	// Black, Red, Green, Yellow, Blue, Purple, Cyan, Gray
	// Same again, but bright
	0x000000, 0x770000, 0x007700, 0x777700, 0x000077, 0x770077, 0x007777, 0xAAAAAAA,
	0xCCCCCC, 0xFF0000, 0x00FF00, 0xFFFF00, 0x0000FF, 0xFF00FF, 0x00FFFF, 0xFFFFFFF
};

 int	Term_HandleVT100_Long(tTerminal *Term, int Len, const char *Buf);

static inline int min(int a, int b)
{
	return a < b ? a : b;
}

int Term_HandleVT100(tTerminal *Term, int Len, const char *Buf)
{
	#define	MAX_VT100_ESCAPE_LEN	16
	static char	inc_buf[MAX_VT100_ESCAPE_LEN];
	static int	inc_len = 0;

	if( inc_len > 0	|| *Buf == '\x1b' )
	{
		// Handle VT100 (like) escape sequence
		 int	new_bytes = min(MAX_VT100_ESCAPE_LEN - inc_len, Len);
		 int	ret = 0;
		 int	old_inc_len = inc_len;
		memcpy(inc_buf + inc_len, Buf, new_bytes);

		if( new_bytes == 0 ) {
			inc_len = 0;
			return 0;
		}

		inc_len += new_bytes;
		//_SysDebug("inc_buf = %i '%.*s'", inc_len, inc_len, inc_buf);

		if( inc_len <= 1 )
			return 1;	// Skip 1 character (the '\x1b')

		switch(inc_buf[1])
		{
		case '[':	// Multibyte, funtime starts	
			ret = Term_HandleVT100_Long(Term, inc_len-2, inc_buf+2);
			if( ret > 0 ) {
				ret += 2;
			}
			break;
		case 'D':
			Display_ScrollDown(Term, 1);
			ret = 2;
			break;
		case 'M':
			Display_ScrollDown(Term, -1);
			ret = 2;
			break;
		default:
			ret = 2;
			break;
		}	

		if( ret != 0 ) {
			inc_len = 0;
			assert(ret > old_inc_len);
			//_SysDebug("%i bytes of escape code '%.*s' (return %i)",
			//	ret, ret, inc_buf, ret-old_inc_len);
			ret -= old_inc_len;	// counter cached bytes
		}
		else
			ret = new_bytes;
		return ret;
	}

	switch( *Buf )
	{
	// TODO: Need to handle \t and ^A-Z
	case '\b':
		Display_MoveCursor(Term, 0, -1);
		Display_AddText(Term, 1, " ");
		Display_MoveCursor(Term, 0, -1);
		return 1;
	case '\t':
		// TODO: tab (get current cursor pos, space until multiple of 8)
		return 1;
	case '\n':
		// TODO: Support disabling CR after NL
		Display_Newline(Term, 1);
		return 1;
	case '\r':
		if( Len >= 2 && Buf[1] == '\n' ) {
			Display_Newline(Term, 1);
			return 2;
		}
		else {
			Display_MoveCursor(Term, 0, INT_MIN);
			return 1;
		}
	}

	 int	ret = 0;
	while( ret < Len )
	{
		switch(*Buf)
		{
		case '\x1b':
		case '\b':
		case '\t':
		case '\n':
		case '\r':
			// Force an exit right now
			Len = ret;
			break;
		default:
			ret ++;
			Buf ++;
			break;
		}
	}
	return -ret;
}

int Term_HandleVT100_Long(tTerminal *Term, int Len, const char *Buffer)
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
		_SysDebug("Unexpected char 0x%x in VT100 escape code '\\e[%.*s'", c,
			Len, Buffer);
		return j;
	}

	if( bQuestionMark )
	{
		 int	set = 0;
		// Special commands
		switch( c )
		{
		case 'h':	// set
			set = 1;
		case 'l':	// unset
			switch(args[0])
			{
			case 25:	// Hide cursor
				_SysDebug("TODO: \\e[?25%c Show/Hide cursor", c);
				break;
			case 1047:	// Alternate buffer
				Display_ShowAltBuffer(Term, set);
				break;
			default:
				_SysDebug("TODO: \\e[?%i%c Unknow DEC private mode", args[0], c);
				break;
			}
			break;
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
		case 'H':
			if( argc != 2 ) {
			}
			else {
				Display_SetCursor(Term, args[0], args[1]);
			}
			break;
		case 'J':
			switch( args[0] )
			{
			case 0:
			case 1:
				_SysDebug("TODO: VT100 %i J", args[0]);
				break;
			case 2:	// Everything
				Display_ClearLines(Term, 0);
				break;
			default:
				_SysDebug("Unknown VT100 %i J", args[0]);
				break;
			}
			break;
		case 'K':
			switch( args[0] )
			{
			case 0:	// To EOL
			case 1:	// To SOL
				_SysDebug("TODO: VT100 %i K", args[0]);
				break;
			case 2:
				Display_ClearLine(Term, 0);
				break;
			default:
				_SysDebug("Unknown VT100 %i K", args[0]);
				break;
			}
		case 'T':	// Scroll down n=1
			Display_ScrollDown(Term, 1);
			break;
		case 'm':
			if( argc == 0 )
			{
				// Reset
				Display_ResetAttributes(Term);
			}
			else
			{
				for( int i = 0; i < argc; i ++ )
				{
					switch(args[i])
					{
					case 0:
						Display_ResetAttributes(Term);
						break;
					case 30 ... 37:
						// TODO: Bold/bright
						Display_SetForeground( Term, caVT100Colours[ args[i]-30 ] );
						break;
					case 40 ... 47:
						// TODO: Bold/bright
						Display_SetBackground( Term, caVT100Colours[ args[i]-30 ] );
						break;
					default:
						_SysDebug("TODO: VT100 \\e[%im", args[i]);
						break;
					} 
				}
			}
			break;
		// Set scrolling region
		case 'r':
			Display_SetScrollArea(Term, args[0], args[1] - args[0]);
			break;
		
		case 's':
			Display_SaveCursor(Term);
			break;
		case 'u':
			Display_RestoreCursor(Term);
			break;
		default:
			_SysDebug("Unknown VT100 long escape char 0x%x '%c'", c, c);
			break;
		}
	}
	return j;
}
