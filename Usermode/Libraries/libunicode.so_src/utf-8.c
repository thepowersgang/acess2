/*
 * Acess2 "libunicode" UTF Parser
 * - By John Hodge (thePowersGang)
 *
 * utf-8.c
 * - UTF-8 Parsing code
 */
#include <stdint.h>
#include <unicode.h>

/**
 * \brief Read a UTF-8 character from a string
 * \param Input	Source UTF-8 encoded string
 * \param Val	Destination for read codepoint
 * \return Number of bytes read/used
 */
int ReadUTF8(const char *Input, uint32_t *Val)
{
	const uint8_t	*str = (const uint8_t *)Input;
	*Val = 0xFFFD;	// Assume invalid character
	
	// ASCII
	if( !(*str & 0x80) ) {
		*Val = *str;
		return 1;
	}
	
	// Middle of a sequence
	if( (*str & 0xC0) == 0x80 ) {
		return 1;
	}
	
	// Two Byte
	if( (*str & 0xE0) == 0xC0 ) {
		*Val = (*str & 0x1F) << 6;	// Upper 6 Bits
		str ++;
		if( (*str & 0xC0) != 0x80)	return -1;	// Validity check
		*Val |= (*str & 0x3F);	// Lower 6 Bits
		return 2;
	}
	
	// Three Byte
	if( (*str & 0xF0) == 0xE0 ) {
		*Val = (*str & 0x0F) << 12;	// Upper 4 Bits
		str ++;
		if( (*str & 0xC0) != 0x80)	return -1;	// Validity check
		*Val |= (*str & 0x3F) << 6;	// Middle 6 Bits
		str ++;
		if( (*str & 0xC0) != 0x80)	return -1;	// Validity check
		*Val |= (*str & 0x3F);	// Lower 6 Bits
		return 3;
	}
	
	// Four Byte
	if( (*str & 0xF8) == 0xF0 ) {
		*Val = (*str & 0x07) << 18;	// Upper 3 Bits
		str ++;
		if( (*str & 0xC0) != 0x80)	return -1;	// Validity check
		*Val |= (*str & 0x3F) << 12;	// Middle-upper 6 Bits
		str ++;
		if( (*str & 0xC0) != 0x80)	return -1;	// Validity check
		*Val |= (*str & 0x3F) << 6;	// Middle-lower 6 Bits
		str ++;
		if( (*str & 0xC0) != 0x80)	return -1;	// Validity check
		*Val |= (*str & 0x3F);	// Lower 6 Bits
		return 4;
	}
	
	// UTF-8 Doesn't support more than four bytes
	return 4;
}

/**
 * \brief Get the UTF-8 character before the 
 * \
 */
int ReadUTF8Rev(const char *Base, int Offset, uint32_t *Val)
{
	 int	len = 0;
	
	// Scan backwards for the beginning of the character
	while( Offset > 0 && (Base[Offset--] & 0xC0) == 0x80 )
		len ++;
	// Invalid string (no beginning)
	if(Offset == 0 && (Base[Offset] & 0xC0) == 0x80 )
		return len;
	
	len ++;	// First character
	if( ReadUTF8(Base+Offset, Val) != len ) {
		*Val = 0xFFFD;
	}
	return len;
}

/**
 * \brief Write a UTF-8 character sequence to a string
 * \param buf	Destination buffer (must have at least 4 bytes available)
 * \param Val	Unicode codepoint to write
 * \return Number of bytes written
 * \note Does not NULL terminate the string in \a buf
 */
int WriteUTF8(char *buf, uint32_t Val)
{
	uint8_t	*str = (void*)buf;
	
	// ASCII
	if( Val < 128 ) {
		if(str) {
			*str = Val;
		}
		return 1;
	}
	
	// Two Byte
	if( Val < 0x8000 ) {
		if(str) {
			*str = 0xC0 | (Val >> 6);
			str ++;
			*str = 0x80 | (Val & 0x3F);
		}
		return 2;
	}
	
	// Three Byte
	if( Val < 0x10000 ) {
		if(str) {
			*str = 0xE0 | (Val >> 12);
			str ++;
			*str = 0x80 | ((Val >> 6) & 0x3F);
			str ++;
			*str = 0x80 | (Val & 0x3F);
		}
		return 3;
	}
	
	// Four Byte
	if( Val < 0x110000 ) {
		if(str) {
			*str = 0xF0 | (Val >> 18);
			str ++;
			*str = 0x80 | ((Val >> 12) & 0x3F);
			str ++;
			*str = 0x80 | ((Val >> 6) & 0x3F);
			str ++;
			*str = 0x80 | (Val & 0x3F);
		}
		return 4;
	}
	
	// UTF-8 Doesn't support more than four bytes
	return 0;
}

