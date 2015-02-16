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

#ifdef __cplusplus
extern "C" {
#endif

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


static inline int	Unicode_IsPrinting(uint32_t Codepoint) { return 1; }

#ifdef __cplusplus
}	// extern "C"
#endif

#ifdef __cplusplus
#include <iterator>

namespace libunicode {

class utf8iterator:
	public ::std::iterator< ::std::forward_iterator_tag, uint32_t >
{
	const char*	m_curpos;
	uint32_t	m_curval;
public:
	utf8iterator():
		m_curpos(0)
	{
	}
	utf8iterator(const char *pos):
		m_curpos(pos)
	{
		(*this)++;
	}
	utf8iterator(const utf8iterator& other) {
		m_curpos = other.m_curpos;
		m_curval = other.m_curval;
	}
	utf8iterator& operator=(const utf8iterator& other) {
		m_curpos = other.m_curpos;
		m_curval = other.m_curval;
		return *this;
	}
	
	bool operator== (const utf8iterator& other) {
		return other.m_curpos == m_curpos;
	}
	bool operator!= (const utf8iterator& other) {
		return other.m_curpos != m_curpos;
	}
	utf8iterator& operator++() {
		m_curpos += ::ReadUTF8(m_curpos, &m_curval);
		return *this;
	}
	utf8iterator operator++(int) {
		utf8iterator	rv(*this);
		m_curpos += ::ReadUTF8(m_curpos, &m_curval);
		return rv;
	}
	uint32_t operator*() const {
		return m_curval;
	}
	uint32_t operator->() const {
		return m_curval;
	}
};

class utf8string
{
	const char* m_data;
	size_t	m_len;
	
	size_t _strlen(const char*s) {
		size_t	l = 0;
		while(*s)	l ++;
		return l;
	}
public:
	utf8string(const char* c_str):
		m_data(c_str),
		m_len(_strlen(c_str))
	{
	}
	utf8string(const char* c_str, size_t len):
		m_data(c_str),
		m_len(len)
	{
	}
	utf8string(const ::std::string& str):
		m_data(str.c_str()),
		m_len(str.size())
	{
	}
	
	utf8iterator begin() const {
		return utf8iterator(m_data);
	}
	utf8iterator end() const {
		return utf8iterator(m_data + m_len);
	}
};


};

#endif

#endif

