/*
 * Acess2 "libunicode" UTF Parser
 * - By John Hodge (thePowersGang)
 *
 * unicode.h
 * - Main header
 */
#ifndef _LIBUNICODE__UNICODE_H_
#define _LIBUNICODE__UNICODE_H_

#include <stdint.h>

/**
 * \breif Read a single codepoint from  a UTF-8 stream
 * \return Number of bytes read
 */
extern int	ReadUTF8(const char *Input, uint32_t *Val);
/**
 * \brief Read backwards in the stream
 */
extern int	ReadUTF8Rev(const char *Base, int Offset, uint32_t *Val);
/**
 * \breif Write a single codepoint to a UTF-8 stream
 */
extern int	WriteUTF8(char *buf, uint32_t Val);

#endif

