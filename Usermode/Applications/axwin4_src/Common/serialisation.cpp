/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang) 
 *
 * serialisation.cpp
 * - IPC Serialisation
 */
#include <serialisation.hpp>
#include <cstddef>
#include <stdexcept>
#include <acess/sys.h>	// SysDebug

namespace AxWin {

CDeserialiser::CDeserialiser(const ::std::vector<uint8_t>& vector):
	m_vect(vector),
	m_offset(0)
{
}
CDeserialiser::CDeserialiser(::std::vector<uint8_t>&& vector):
	m_vect(vector),
	m_offset(0)
{
}
CDeserialiser& CDeserialiser::operator=(const CDeserialiser& x)
{
	m_vect = x.m_vect;
	m_offset = x.m_offset;
}

bool CDeserialiser::IsConsumed() const
{
	return m_offset == m_vect.size();
}

::uint8_t CDeserialiser::ReadU8()
{
	RangeCheck("CDeserialiser::ReadU8", 1);
	uint8_t rv = m_vect[m_offset];
	m_offset ++;
	return rv;
}

::uint16_t CDeserialiser::ReadU16()
{
	RangeCheck("CDeserialiser::ReadU16", 2);
	uint16_t rv = m_vect[m_offset] | ((uint16_t)m_vect[m_offset+1] << 8);
	m_offset += 2;
	return rv;
}

::int16_t CDeserialiser::ReadS16()
{
	uint16_t rv_u = ReadU16();
	if( rv_u < 0x8000 )
		return rv_u;
	else
		return ~rv_u + 1;
}

const ::std::vector<uint8_t> CDeserialiser::ReadBuffer()
{
	RangeCheck("CDeserialiser::ReadBuffer(len)", 2);
	size_t	size = ReadU16();
	
	auto range_start = m_vect.begin() + int(m_offset);
	::std::vector<uint8_t> ret( range_start, range_start + int(size) );
	m_offset += size;
	return ret;
}

const ::std::string CDeserialiser::ReadString()
{
	RangeCheck("CDeserialiser::ReadString(len)", 1);
	uint8_t len = ReadU8();
	
	RangeCheck("CDeserialiser::ReadString(data)", len);
	::std::string ret( reinterpret_cast<const char*>(m_vect.data()+m_offset), len );
	m_offset += len;
	return ret;
}

void CDeserialiser::RangeCheck(const char *Method, size_t bytes) throw(::std::out_of_range)
{
	if( m_offset + bytes > m_vect.size() ) {
		::_SysDebug("%s - out of range %i+%i >= %i", Method, m_offset, bytes, m_vect.size());
		throw ::std::out_of_range(Method);
	}
}

CSerialiser::CSerialiser()
{
}

void CSerialiser::WriteU8(::uint8_t Value)
{
	m_data.push_back(Value);
}

void CSerialiser::WriteU16(::uint16_t Value)
{
	m_data.push_back(Value & 0xFF);
	m_data.push_back(Value >> 8);
}

void CSerialiser::WriteS16(::int16_t Value)
{
	if( Value < 0 )
	{
		::uint16_t rawval = 0x10000 - (::int32_t)Value;
		WriteU16(rawval);
	}
	else
	{
		WriteU16(Value);
	}
}

void CSerialiser::WriteBuffer(size_t n, const void* val)
{
	const uint8_t*	val8 = static_cast<const uint8_t*>(val);
	if( n > 0xFFFF )
		throw ::std::length_error("CSerialiser::WriteBuffer");
	m_data.reserve( m_data.size() + 2 + n );
	WriteU16(n);
	for( size_t i = 0; i < n; i ++ )
		m_data.push_back(val8[i]);
}

void CSerialiser::WriteString(const char* val, size_t n)
{
	if( n > 0xFF )
		throw ::std::length_error("CSerialiser::WriteString");
	m_data.reserve( m_data.size() + 1 + n );
	WriteU8(n);
	for( size_t i = 0; i < n; i ++ )
		m_data.push_back(val[i]);
}

void CSerialiser::WriteSub(const CSerialiser& val)
{
	// TODO: Append reference to sub-buffer contents
	m_data.reserve( m_data.size() + val.m_data.size() );
	for( auto byte : val.m_data )
		m_data.push_back( byte );
}

const ::std::vector<uint8_t>& CSerialiser::Compact()
{
	return m_data;
}

};	// namespace AxWin

