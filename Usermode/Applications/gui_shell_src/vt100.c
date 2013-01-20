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
		memcpy(inc_buf + inc_len, Buf, min(MAX_VT100_ESCAPE_LEN - inc_len, Len));
		// Handle VT100 (like) escape sequence
		
		inc_len = 0;
		return 1;
	}

	switch( *Buf )
	{
	case '\b':
		// TODO: Backspace
		Display_MoveCursor(-1, 0);
		return 1;
	case '\t':
		// TODO: tab (get current cursor pos, space until multiple of 8)
		return 1;
	case '\n':
		Display_Newline(1);
		return 1;
	case '\r':
		// TODO: Carriage return
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
