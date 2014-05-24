/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang) 
 *
 * serialisation.hpp
 * - Generic (de)serialisation code
 */
#ifndef _SERIALISATION_H_
#define _SERIALISATION_H_

#include <cstdint>
#include <cstddef>
#include <exception>
#include <string>
#include <vector>

namespace AxWin {

class CDeserialiseException:
	public ::std::exception
{
};

class CDeserialiser
{
	const size_t	m_length;
	const uint8_t*	m_data;
	size_t	m_offset;
public:
	CDeserialiser(size_t Length, const uint8_t *Buffer);
	::uint8_t	ReadU8();
	::uint16_t	ReadU16();
	::int16_t	ReadS16();
	::std::string	ReadString();
};

class CSerialiser
{
	::std::vector<uint8_t>	m_data;
public:
	CSerialiser();
	void WriteU8(::uint8_t val);
	void WriteU16(::uint16_t val);
	void WriteS16(::int16_t val);
	void WriteString(const char* val, size_t n);
	void WriteString(const char* val) {
		WriteString(val, ::std::char_traits<char>::length(val));
	}
	void WriteString(const ::std::string& val) {
		WriteString(val.data(), val.size());
	}
	void WriteSub(const CSerialiser& val);
};

};

#endif

