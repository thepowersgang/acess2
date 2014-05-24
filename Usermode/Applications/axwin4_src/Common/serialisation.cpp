/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang) 
 *
 * serialisation.cpp
 * - IPC Serialisation
 */
#include <serialisation.hpp>
#include <cstddef>

namespace AxWin {

CDeserialiser::CDeserialiser(size_t Length, const uint8_t *Buffer):
	m_length(Length),
	m_data(Buffer),
	m_offset(0)
{
}

::uint8_t CDeserialiser::ReadU8()
{
	return 0;
}

::uint16_t CDeserialiser::ReadU16()
{
	return 0;
}

::int16_t CDeserialiser::ReadS16()
{
	return 0;
}

::std::string CDeserialiser::ReadString()
{
	return "";
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

void CSerialiser::WriteString(const char* val, size_t n)
{
	//if( n >= 256 )
	//	throw ::std::out_of_range("CSerialiser::WriteString");
	WriteU8(n);
	for( size_t i = 0; i < n; i ++ )
		WriteU8(val[i]);
}

void CSerialiser::WriteSub(const CSerialiser& val)
{
	m_data.reserve( m_data.size() + val.m_data.size() );
	for( auto byte : val.m_data )
		m_data.push_back( byte );
}

};	// namespace AxWin

