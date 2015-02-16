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

# define ASSERTC(a, r, b)	assert(a r b)

static inline int MIN(int a, int b)
{
	return a < b ? a : b;
}
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

#define	MAX_VT100_ESCAPE_LEN	32
typedef struct {
	uint32_t	Flags;
	int	CurFG, CurBG;

	char	cache[MAX_VT100_ESCAPE_LEN];
	 int	cache_len;	

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

/**
 * \brief Detect and handle VT100/ANSI/xterm escape sequences
 * \param Term	Terminal handle (opaque)
 * \param Len	Number of avaliable bytes
 * \param Buf	Input buffer (\a Len bytes long)
 * \return -ve   : Number of bytes that should be sent to screen
 * \return +ve/0 : Number of bytes consumed by this function
 */
int Term_HandleVT100(tTerminal *Term, int Len, const char *Buf)
{
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

	if( st->cache_len > 0	|| *Buf == '\x1b' )
	{
		// Handle VT100 (like) escape sequence
		 int	new_bytes = MIN(MAX_VT100_ESCAPE_LEN - st->cache_len, Len);
		 int	ret = 0;
		 int	old_inc_len = st->cache_len;
		
		memcpy(st->cache + st->cache_len, Buf, new_bytes);

		if( new_bytes == 0 ) {
			_SysDebug("Term_HandleVT100: Hit max? (Len=%i) Flushing cache", Len);
			st->cache_len = 0;
			return 0;
		}

		st->cache_len += new_bytes;
		assert(st->cache_len > old_inc_len);

		if( st->cache_len <= 1 )
			return 1;	// Skip 1 character (the '\x1b')

		ret = Term_HandleVT100_Short(Term, st->cache_len, st->cache);

		if( ret != 0 ) {
			// Check that we actually used the new data (as should have happened)
			if( ret <= old_inc_len ) {
				_SysDebug("Term_HandleVT100: ret(%i) <= old_inc_len(%i), inc_len=%i, '%*C'",
					ret, old_inc_len, st->cache_len, st->cache_len, st->cache);
				ASSERTC(ret, >, old_inc_len);
			}
			st->cache_len = 0;
			//_SysDebug("%i bytes of escape code '%.*s' (return %i)",
			//	ret, ret, inc_buf, ret-old_inc_len);
			ret -= old_inc_len;	// counter cached bytes
		}
		else {
			ret = new_bytes;
			_SysDebug("Term_HandleVT100: Caching %i bytes '%*C'", ret, ret, st->cache);
		}
		return ret;
	}

	switch( *Buf )
	{
	case '\a':
		// Alarm, aka bell
		//Display_SoundBell(Term);
		break;
	case '\b':
		// backspace is aprarently just supposed to cursor left (if possible)
		Display_MoveCursor(Term, 0, -1);
		return 1;
	case '\t':
		// TODO: tab (get current cursor pos, space until multiple of 8)
		_SysDebug("TODO: VT100 Support \\t tab");
		Display_AddText(Term, 1, "\t");	// pass the buck for now
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

/**
 * \brief Handle an escape code beginning with '\x1b'
 * \return 0 : Insufficient data in buffer, wait for more
 * \return +ve : Number of bytes in escape sequence
 */
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
	
	case '#':
		if( Len == 2 )
			return 0;
		switch(Buf[2])
		{
		case 8:
			_SysDebug("TODO \\e#%c DECALN - Fill screen with 'E'", Buf[2]);
			break;
		default:
			_SysDebug("Unknown \\e#%c", Buf[2]);
			break;
		}
		return 3;
		
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
	case '7':
		// Save cursor
		Display_SaveCursor(Term);
		return 2;
	case '8':
		// Restore cursor
		Display_RestoreCursor(Term);
		return 2;
	case 'D':
		// Cursor down, if at bottom scroll
		Display_MoveCursor(Term, 1, 0);
		// TODO: Scroll if at bottom (impl in _MoveCursor)
		return 2;
	case 'E':
		Display_MoveCursor(Term, 1, INT_MIN);
		// TODO: Scroll if at bottom (impl in _MoveCursor)
		return 2;
	case 'M':
		// Cursor up, scroll if at top
		Display_MoveCursor(Term, -1, 0);
		return 2;
	default:
		_SysDebug("Unknown VT100 \\e%c", Buf[1]);
		return 2;
	}
}

/**
 * \brief Handle CSI escape sequences '\x1b['
 * \return 0   : insufficient data
 * \return +ve : Number of bytes consumed
 */
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
	if(c == '?')
	{
		bQuestionMark = 1;
		if(j == Len)	return 0;
		c = Buffer[j++];
	}
	if( ('0' <= c && c <= '9') || c == ';' )
	{
		if(c == ';')
			argc ++;
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
			case  3:	// 132 Column mode
				_SysDebug("TODO: \\e[?3%c 132 Column mode", c);
				break;
			case  4:	// Smooth (Slow) Scroll
				_SysDebug("TODO: \\e[?4%c Smooth (Slow) Scroll", c);
				break;
			case  5:	// Reverse Video
				_SysDebug("TODO: \\e[?5%c Reverse Video", c);
				break;
			case  6:	// Origin Mode
				_SysDebug("TODO: \\e[?6%c Origin Mode", c);
				break;
			case 12:
				//_SysDebug("TODO: \\e[?25%c Start/Stop blinking cursor", c);
				//Display_SolidCursor(Term, !set);
				break;
			case 25:	// Hide cursor
				//_SysDebug("TODO: \\e[?25%c Show/Hide cursor", c);
				//Display_ShowCursor(Term, set);
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
			Display_MoveCursor(Term, -(args[0] != 0 ? args[0] : 1), 0);
			break;
		case 'B':
			Display_MoveCursor(Term, (args[0] != 0 ? args[0] : 1), 0);
			break;
		case 'C':
			Display_MoveCursor(Term, 0, (args[0] != 0 ? args[0] : 1));
			break;
		case 'D':
			Display_MoveCursor(Term, 0, -(args[0] != 0 ? args[0] : 1));
			break;
		case 'H':
			if( argc == 0 ) {
				Display_SetCursor(Term, 0, 0);
			}
			else if( argc == 1 ) {
				Display_SetCursor(Term, args[0]-1, 0);
			}
			else if( argc == 2 ) {
				// Adjust 1-based cursor position to 0-based
				Display_SetCursor(Term, args[0]-1, args[1]-1);
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
			break;
		case 'S':	// Scroll text up n=1 (expose bottom)
			Display_ScrollDown(Term, -(argc >= 1 ? args[0] : 1));
			break;
		case 'T':	// Scroll text down n=1 (expose top)
			Display_ScrollDown(Term, (argc >= 1 ? args[0] : 1));
			break;
		case 'c':	// Send Device Attributes
			switch(args[0])
			{
			case 0:	// Request attributes from terminal
				// "VT100 with Advanced Video Option" (same as screen returns)
				Display_SendInput(Term, "\x1b[?1;2c");
				break;
			default:
				_SysDebug("TODO: Request device attributes \\e[%ic", args[0]);
				break;
			}
			break;
		case 'f':
			if( argc != 2 ) {
				Display_SetCursor(Term, 0, 0);
			}
			else {
				// Adjust 1-based cursor position to 0-based
				Display_SetCursor(Term, args[0]-1, args[1]-1);
			}
			break;
		case 'h':
		case 'l':
			for( int i = 0; i < argc; i ++ )
			{
				switch(args[i])
				{
				default:
					_SysDebug("Unknown VT100 mode \e[%i%c",
						args[i], c);
					break;
				}
			}
			break;
		case 'm':
			if( argc == 0 )
			{
				// Reset
				Display_ResetAttributes(Term);
			}
			else if( args[0] == 48 )
			{
				// ISO-8613-3 Background
				if( args[1] == 2 ) {
					uint32_t	col = 0;
					col |= (uint32_t)args[2] << 16;
					col |= (uint32_t)args[3] << 8;
					col |= (uint32_t)args[4] << 0;
					Display_SetBackground(Term, col);
				}
				else if( args[1] == 5 ) {
					_SysDebug("TODO: Support xterm palette BG %i", args[2]);
				}
				else {
					_SysDebug("VT100 Unknown mode set \e[48;%im", args[1]);
				}
			}
			else if( args[0] == 38 )
			{
				// ISO-8613-3 Foreground
				if( args[1] == 2 ) {
					uint32_t	col = 0;
					col |= (uint32_t)args[2] << 16;
					col |= (uint32_t)args[3] << 8;
					col |= (uint32_t)args[4] << 0;
					Display_SetForeground(Term, col);
				}
				else if( args[1] == 5 ) {
					_SysDebug("TODO: Support xterm palette FG %i", args[2]);
				}
				else {
					_SysDebug("VT100 Unknown mode set \e[38;%im", args[1]);
				}
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
						_SysDebug("TODO: \\e[2m - Reverse");
						break;
					case 4:
						_SysDebug("TODO: \\e[4m - Underscore");
						break;
					//case 5:
					//	_SysDebug("TODO: \\e[5m - Blink/bold");
					//	break;
					case 7:
						_SysDebug("TODO: \\e[7m - Reverse");
						break;
					case 24:	// Not underlined
					case 27:	// Not inverse
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
					case 90 ... 97:
						st->CurFG = args[i]-90 + 8;
						Display_SetForeground( Term, caVT100Colours[ st->CurBG ] );
						break;;
					case 100 ... 107:
						st->CurBG = args[i]-100 + 8;
						Display_SetBackground( Term, caVT100Colours[ st->CurBG ] );
						break;;
					default:
						_SysDebug("Unknown mode set \\e[%im", args[i]);
						break;
					} 
				}
			}
			break;
		// Device Status Report
		case 'n':
			break;
		// Set scrolling region
		case 'r':
			Display_SetScrollArea(Term, args[0]-1, (args[1] - args[0])+1);
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

