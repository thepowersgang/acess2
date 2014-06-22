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
	::std::vector<uint8_t>	m_vect;
	size_t	m_offset;
public:
	CDeserialiser():
		CDeserialiser(::std::vector<uint8_t>())
	{}
	CDeserialiser(const ::std::vector<uint8_t>& vect);
	CDeserialiser(::std::vector<uint8_t>&& vect);
	CDeserialiser(const CDeserialiser& x) { *this = x; };
	CDeserialiser& operator=(const CDeserialiser& x);
	bool	IsConsumed() const;
	::uint8_t	ReadU8();
	::uint16_t	ReadU16();
	::int16_t	ReadS16();
	::uint32_t	ReadU32();
	::uint64_t	ReadU64();
	const ::std::vector<uint8_t>	ReadBuffer();
	const ::std::string	ReadString();
private:
	void RangeCheck(const char *Method, size_t bytes) throw(::std::out_of_range);
};

class CSerialiser
{
	::std::vector<uint8_t>	m_data;
public:
	CSerialiser();
	void WriteU8(::uint8_t val);
	void WriteU16(::uint16_t val);
	void WriteS16(::int16_t val);
	void WriteU32(::uint32_t val);
	void WriteU64(::uint64_t val);
	void WriteBuffer(size_t n, const void* val);
	void WriteString(const char* val, size_t n);
	void WriteString(const char* val) {
		WriteString(val, ::std::char_traits<char>::length(val));
	}
	void WriteString(const ::std::string& val) {
		WriteString(val.data(), val.size());
	}
	void WriteSub(const CSerialiser& val);
	
	const ::std::vector<uint8_t>& Compact();
};

};

#endif

