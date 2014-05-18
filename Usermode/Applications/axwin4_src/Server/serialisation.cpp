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

CDeserialiser::CDeserialiser(size_t Length, const void *Buffer)
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

CSerialiser::CSerialiser()
{
}

void CSerialiser::WriteU8(::uint8_t Value)
{
}

};	// namespace AxWin

