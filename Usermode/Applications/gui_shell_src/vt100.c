/*
 * Acess GUI Terminal
 * - By John Hodge (thePowersGang)
 *
 * main.c
 * - Core
 */
#include <string.h>
#include "include/vt100.h"
#include "include/display.h"

int Term_HandleVT100(int Len, const char *Buf)
{
	const int	max_length = 16;
	static char	inc_buf[max_length]
	static int	inc_len = 0;

	if( inc_len > 0	|| *Buf == '\x1b' )
	{
		memcpy(inc_buf + inc_len, Buf, min(max_length - inc_len, Len));
		// Handle VT100 (like) escape sequence
		
		inc_len = 0;
		return 1;
	}

	switch( *Buf )
	{
	case '\b':
		// TODO: Backspace
		return 1;
	case '\t':
		// TODO: tab
		return 1;
	case '\n':
		Display_Newline(1);
		return 1;
	case '\r':
		// TODO: Carriage return
		return ;
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
