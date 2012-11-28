/*
 * Acess GUI Terminal
 * - By John Hodge (thePowersGang)
 *
 * vt100.h
 * - VT100/xterm emulation
 */
#ifndef _VT100_H_
#define _VT100_H_

/**
 * Returns either a positive or negative byte count.
 * Positive means that many bytes were used as part of the escape sequence
 * and should not be sent to the display.
 * Negative means that there were that many bytes before the next escape
 * sequence (and hence those should be displayed).
 */
extern int	Term_HandleVT100(int Len, const char *Buf);


#endif

