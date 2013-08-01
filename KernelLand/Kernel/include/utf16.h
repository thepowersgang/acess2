/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * utf16.h
 * - UTF-16 <-> UTF-8/UCS32 translation
 */
#ifndef _UTF16_H_
#define _UTF16_H_

extern int	ReadUTF16(const Uint16 *Str16, Uint32 *Codepoint);
extern size_t	UTF16_ConvertToUTF8(size_t DestLen, char *Dest, size_t SrcLen, const Uint16 *Source);
extern int	UTF16_CompareWithUTF8Ex(size_t Str16Len, const Uint16 *Str16, const char *Str8, int bIgnoreCase);
extern int	UTF16_CompareWithUTF8(size_t Str16Len, const Uint16 *Str16, const char *Str8);
extern int	UTF16_CompareWithUTF8CI(size_t Str16Len, const Uint16 *Str16, const char *Str8);

#endif

