/*
 * Acess2 C++ Library
 * - By John Hodge (thePowersGang)
 *
 * exceptions.cc
 * - ::std::exception and friends
 */
#include <string>
#include <stdexcept>

void ::std::_throw_out_of_range(const char *message)
{
	throw ::std::out_of_range( ::std::string(message) );
}

