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

namespace AxWin {

class CDeserialiseException:
	public ::std::exception
{
};

class CDeserialiser
{
public:
	CDeserialiser(size_t Length, const void *Buffer);
	::uint8_t	ReadU8();
	::uint16_t	ReadU16();
};

class CSerialiser
{
public:
	CSerialiser();
	void	WriteU8(::uint8_t val);
	void	WriteU16(::uint16_t val);
};

};

#endif

