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
# include <stdlib.h>	// malloc/free
#endif

enum eExcapeMode {
	MODE_NORMAL,
	MODE_IGNORE,
	MODE_STRING
};
enum eStringType {
	STRING_IGNORE,
	STRING_TITLE
};

#define FLAG_BOLD	0x01
#define FLAG_REVERSE	0x02

typedef struct {
	uint32_t	Flags;
	int	CurFG, CurBG;
	
	enum eExcapeMode	Mode;
	
	enum eStringType	StringType;
	size_t	StringLen;
	char	*StringCache;
} tVT100State;

const uint32_t	caVT100Colours[] = {
	// Black,      Red,    Green,   Yellow,     Blue,  Magenta,     Cyan,      Gray
	// Same again, but bright
	0x000000, 0x770000, 0x007700, 0x777700, 0x000077, 0x770077, 0x007777, 0xAAAAAA,
	0xCCCCCC, 0xFF0000, 0x00FF00, 0xFFFF00, 0x0000FF, 0xFF00FF, 0x00FFFF, 0xFFFFFF, 
};

 int	_locate_eos(size_t Len, const char *Buf);
 int	Term_HandleVT100_Short(tTerminal *Term, int Len, const char *Buf);
 int	Term_HandleVT100_Long(tTerminal *Term, int Len, const char *Buf);
 int	Term_HandleVT100_OSC(tTerminal *Term, int Len, const char *Buf);

static inline int min(int a, int b)
{
	return a < b ? a : b;
}

int _locate_eos(size_t Len, const char *Buf)
{
	for( size_t ret = 0; ret < Len; ret ++ )
	{
		if( Buf[ret] == '\007' )
			return ret;
		if( Buf[ret] == '\x9c' )
			return ret;
		if( ret+1 < Len && Buf[ret] == '\x1b' && Buf[ret+1] == '\\' )
			return ret;
	}
	return -1;
}

int Term_HandleVT100(tTerminal *Term, int Len, const char *Buf)
{
	#define	MAX_VT100_ESCAPE_LEN	16
	static char	inc_buf[MAX_VT100_ESCAPE_LEN];
	static int	inc_len = 0;
	tVT100State	*st = Display_GetTermState(Term);
	
	if( st == NULL ) {
		st = malloc( sizeof(*st) );
		memset(st, 0, sizeof(*st));
		Display_SetTermState(Term, st);
	}

	if( st->Mode == MODE_IGNORE ) {
		st->Mode = MODE_NORMAL;
		// Used for multi-byte EOS
		_SysDebug("Ignore 1 '%c'", *Buf);
		return 1;
	}
	else if( st->Mode == MODE_STRING )
	{
		// We're in a string mode
		int pos = _locate_eos(Len, Buf);
		size_t bytes = (pos >= 0 ? pos : Len);
		char *tmp = realloc(st->StringCache, st->StringLen + bytes+1);
		if(!tmp)	return bytes;
		st->StringCache = tmp;
		memcpy(tmp+st->StringLen, Buf, bytes);
		tmp[st->StringLen+bytes] = 0;
		st->StringLen += bytes;
		
		_SysDebug("pos=%i", pos);
		_SysDebug("Buf[+%zi] = '%.*s'", bytes, bytes, Buf);
		// Only apply when we hit EOS at the start
		if( pos != 0 )
			return bytes;
		switch(st->StringType)
		{
		case STRING_TITLE:
			Display_SetTitle(Term, st->StringCache);
			break;
		case STRING_IGNORE:
			break;
		}
		free(st->StringCache);
		st->StringCache = 0;
		st->StringLen = 0;
		
		if( *Buf == '\x1b' ) {
			st->Mode = MODE_IGNORE;
			// skip the '\\'
		}
		else
			st->Mode = MODE_NORMAL;
		return 1;
	}
	else {
		// fall through
	}

	if( inc_len > 0	|| *Buf == '\x1b' )
	{
		// Handle VT100 (like) escape sequence
		 int	new_bytes = min(MAX_VT100_ESCAPE_LEN - inc_len, Len);
		 int	ret = 0;
		 int	old_inc_len = inc_len;
		memcpy(inc_buf + inc_len, Buf, new_bytes);

		if( new_bytes == 0 ) {
			_SysDebug("Term_HandleVT100: Hit max? (Len=%i) Flushing cache", Len);
			inc_len = 0;
			return 0;
		}

		inc_len += new_bytes;
		//_SysDebug("inc_buf = %i '%.*s'", inc_len, inc_len, inc_buf);

		if( inc_len <= 1 )
			return 1;	// Skip 1 character (the '\x1b')

		ret = Term_HandleVT100_Short(Term, inc_len, inc_buf);

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
		_SysDebug("TODO: VT100 Support \\t tab");
		return 1;
	case '\n':
		// TODO: Support disabling CR after NL
		Display_Newline(Term, 1);
		return 1;
	case '\r':
		if( Len >= 2 && Buf[1] == '\n' ) {
			// Fast case for \r\n
			Display_Newline(Term, 1);
			return 2;
		}
		Display_MoveCursor(Term, 0, INT_MIN);
		return 1;
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

int Term_HandleVT100_Short(tTerminal *Term, int Len, const char *Buf)
{
	 int	tmp;
	switch(Buf[1])
	{
	case '[':	// Multibyte, funtime starts	
		tmp = Term_HandleVT100_Long(Term, Len-2, Buf+2);
		assert(tmp >= 0);
		if( tmp == 0 )
			return 0;
		return tmp + 2;
	case ']':
		tmp = Term_HandleVT100_OSC(Term, Len-2, Buf+2);
		assert(tmp >= 0);
		if( tmp == 0 )
			return 0;
		return tmp + 2;
	case '=':
		_SysDebug("TODO: \\e= Application Keypad");
		return 2;
	case '>':
		_SysDebug("TODO: \\e= Normal Keypad");
		return 2;
	
	case '(':	// Update G0 charset
		tmp = 0; if(0)
	case ')':	// Update G1 charset
		tmp = 1; if(0)
	case '*':
		tmp = 2; if(0)
	case '+':
		tmp = 3;
		
		if( Len <= 2 )
			return 0;	// We need more
		switch(Buf[2])
		{
		case '0':	// DEC Special Character/Linedrawing set
		case 'A':	// UK
		case 'B':	// US ASCII
			break;
		}
		return 3;
	// xterm C1 \eD and \eM are 'Index' and 'Reverse Index'
	// - Aparently scroll?
	//case 'D':
	//	Display_ScrollDown(Term, 1);
	//	ret = 2;
	//	break;
	//case 'M':
	//	Display_ScrollDown(Term, -1);
	//	ret = 2;
	//	break;
	default:
		_SysDebug("Unknown VT100 \\e%c", Buf[1]);
		return 2;
	}
}

int Term_HandleVT100_Long(tTerminal *Term, int Len, const char *Buffer)
{
	tVT100State	*st = Display_GetTermState(Term);
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
			case  1:	// Aplication cursor keys
				_SysDebug("TODO: \\e[?1%c Application cursor keys", c);
				break;
			case 25:	// Hide cursor
				_SysDebug("TODO: \\e[?25%c Show/Hide cursor", c);
				break;
			case 1047:	// Alternate buffer
				Display_ShowAltBuffer(Term, set);
				break;
			case 1048:	// Save/restore cursor in DECSC
				_SysDebug("TODO: \\e[?1048%c Save/Restore cursor", c);
				break;
			case 1049:	// Save/restore cursor in DECSC and use alternate buffer
				_SysDebug("TODO: \\e[?1049%c Save/Restore cursor", c);
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
		case 'A':
			Display_MoveCursor(Term, -(argc >= 1 ? args[0] : 1), 0);
			break;
		case 'B':
			Display_MoveCursor(Term, (argc >= 1 ? args[0] : 1), 0);
			break;
		case 'C':
			Display_MoveCursor(Term, 0, (argc >= 1 ? args[0] : 1));
			break;
		case 'D':
			Display_MoveCursor(Term, 0, -(argc >= 1 ? args[0] : 1));
			break;
		case 'H':
			if( argc != 2 ) {
			}
			else {
				Display_SetCursor(Term, args[0], args[1]);
			}
			break;
		case 'J':	// Clear lines
			switch( args[0] )
			{
			case 0:
				Display_ClearLines(Term, 1);	// Down
				break;
			case 1:
				Display_ClearLines(Term, -1);	// Up
				break;
			case 2:
				Display_ClearLines(Term, 0);	// All
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
				Display_ClearLine(Term, 1);
				break;
			case 1:	// To SOL
				Display_ClearLine(Term, -1);
				break;
			case 2:
				Display_ClearLine(Term, 0);
				break;
			default:
				_SysDebug("Unknown VT100 %i K", args[0]);
				break;
			}
		case 'S':	// Scroll up n=1
			Display_ScrollDown(Term, -(argc >= 1 ? args[0] : 1));
			break;
		case 'T':	// Scroll down n=1
			Display_ScrollDown(Term, (argc >= 1 ? args[0] : 1));
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
						st->Flags = 0;
						Display_ResetAttributes(Term);
						break;
					case 1:
						st->Flags |= FLAG_BOLD;
						Display_SetForeground( Term, caVT100Colours[st->CurFG + 8] );
						break;
					case 2:
						_SysDebug("TODO: VT100 \\e[1m - Reverse");
						break;
					case 30 ... 37:
						st->CurFG = args[i]-30;
						if(0)
					case 39:
						st->CurFG = 7;
						Display_SetForeground( Term,
							caVT100Colours[ st->CurFG + (st->Flags&FLAG_BOLD?8:0) ] );
						break;
					case 40 ... 47:
						st->CurBG = args[i]-40;
						if(0)
					case 49:
						st->CurBG = 0;
						Display_SetBackground( Term, caVT100Colours[ st->CurBG ] );
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
			Display_SetScrollArea(Term, args[0], (args[1] - args[0]));
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

int Term_HandleVT100_OSC(tTerminal *Term, int Len, const char *Buf)
{
	tVT100State	*st = Display_GetTermState(Term);

	 int	ofs = 0;
	// OSC Ps ; Pt [ST/BEL]
	if(Len < 2)	return 0;	// Need moar

	 int	Ps = 0;
	while( ofs < Len && isdigit(Buf[ofs]) ) {
		Ps = Ps * 10 + (Buf[ofs] - '0');
		ofs ++;
	}

	if( ofs == Len )	return 0;
	if( Buf[ofs] != ';' ) {
		// Error
		st->Mode = MODE_STRING;
		st->StringType = STRING_IGNORE;
		return ofs;
	}
	ofs ++;

	switch(Ps)
	{
	case 0:	// Icon Name + Window Title
	case 1:	// Icon Name
	case 2:	// Window Title
		st->Mode = MODE_STRING;
		st->StringType = STRING_TITLE;
		break;
	case 3:	// Set X Property
		_SysDebug("TODO: \\e]3; Support X properties");
		st->Mode = MODE_STRING;
		st->StringType = STRING_IGNORE;
		break;
	case 4:	// Change colour number
	case 5:	// Change special colour number
		_SysDebug("TODO: \\e]%i;c; Support X properties", Ps);
		st->Mode = MODE_STRING;
		st->StringType = STRING_IGNORE;
		break;	
	case 10:	// Change foreground to Pt
	case 11:	// Change background to Pt
	case 52:
		// TODO: Can be Pt = [cps01234567]*;<str>
		// > Clipboard data in base-64, cut/primary/select buffers 0-7
		// > <str>='?' returns the clipbard in the same format
	default:
		_SysDebug("Unknown VT100 OSC \\e]%i;", Ps);
		st->Mode = MODE_STRING;
		st->StringType = STRING_IGNORE;
		break;
	}
	return ofs;
}

