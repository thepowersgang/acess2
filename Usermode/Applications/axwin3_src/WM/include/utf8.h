/*
 * Acess2 GUI (AxWin) Version 3
 * - By John Hodge (thePowersGang)
 *
 * utf8.h
 * - UTF-8 Parsing header
 */
#ifndef _UTF8_H_
#define _UTF8_H_

#include <stdint.h>

extern int	ReadUTF8(const char *Input, uint32_t *Val);
extern int	ReadUTF8Rev(const char *Base, int Offset, uint32_t *Val);
extern int	WriteUTF8(char *buf, uint32_t Val);

#endif

