/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang) 
 *
 * utf16.c
 * - UTF-16 Translation/Manipulation
 */
#define DEBUG	0
#include <acess.h>
#include <utf16.h>
#include <ctype.h>

int ReadUTF16(const Uint16 *Str16, Uint32 *Codepoint)
{
	if( 0xD800 < *Str16 && *Str16 <= 0xDFFF )
	{
		// UTF-16 surrogate pair
		// > 0xDC00 is the second word
		if( Str16[0] > 0xDC00 ) {
			*Codepoint = 0;
			return 1;
		}
		if( Str16[1] < 0xD800 || Str16[1] >= 0xDC00 ) {
			*Codepoint = 0;
			return 2;
		}
		// 2^16 + 20-bit
		*Codepoint = 0x10000 + (((Str16[0] & 0x3FF) << 10) | (Str16[1] & 0x3FF));
		return 2;
	}
	else {
		*Codepoint = *Str16;
		return 1;
	}
}

size_t UTF16_ConvertToUTF8(size_t DestLen, char *Dest, size_t SrcLen, const Uint16 *Source)
{
	 int	len = 0;
	for( ; *Source && SrcLen --; Source ++ )
	{
		// TODO: Decode/Reencode
		if( Dest && len < DestLen )
			Dest[len] = *Source;
		len += 1;
	}
	if( Dest && len < DestLen )
		Dest[len] = 0;
	return len;
}

int UTF16_CompareWithUTF8Ex(size_t Str16Len, const Uint16 *Str16, const char *Str8, int bCaseInsensitive)
{
	 int	pos16 = 0, pos8 = 0;
	const Uint8	*str8 = (const Uint8 *)Str8;
	
	while( pos16 < Str16Len && Str16[pos16] && str8[pos8] )
	{
		Uint32	cp8, cp16;
		pos16 += ReadUTF16(Str16+pos16, &cp16);
		pos8 += ReadUTF8(str8 + pos8, &cp8);
		if( bCaseInsensitive ) {
			cp16 = toupper(cp16);
			cp8 = toupper(cp8);
		}
	
		LOG("cp16 = %x, cp8 = %x", cp16, cp8);
		if(cp16 == cp8)	continue ;
		
		if(cp16 < cp8)
			return -1;
		else
			return 1;
	}
	if( pos16 == Str16Len )
		return 0;
	if( Str16[pos16] && str8[pos8] )
		return 0;
	if( Str16[pos16] )
		return 1;
	else
		return -1;
}

int UTF16_CompareWithUTF8(size_t Str16Len, const Uint16 *Str16, const char *Str8)
{
	return UTF16_CompareWithUTF8Ex(Str16Len, Str16, Str8, 0);
}

int UTF16_CompareWithUTF8CI(size_t Str16Len, const Uint16 *Str16, const char *Str8)
{
	return UTF16_CompareWithUTF8Ex(Str16Len, Str16, Str8, 1);
}

