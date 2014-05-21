/*
 * Acess2 C++ Library
 * - By John Hodge (thePowersGang)
 *
 * exceptions.cc
 * - ::std::exception and friends
 */
#include <string>
#include <exception>
#include <stdexcept>
#include <acess/sys.h>

// === CODE ===
::std::exception::exception() throw():
	m_what_str("-empty-")
{
}
::std::exception::exception(const exception& other) throw():
	m_what_str(other.m_what_str)
{
}
::std::exception::exception(const string& str) throw():
	m_what_str(str)
{
}
::std::exception& ::std::exception::operator=(const exception& other) throw()
{
	m_what_str = other.m_what_str;
	return *this;
}
::std::exception::~exception() throw()
{
}
const char* ::std::exception::what() const throw()
{
	return m_what_str.c_str();
}

void ::std::terminate()
{
	_exit(0);
}

// --- Standar Exceptions ---
::std::logic_error::logic_error(const ::std::string& what_str):
	exception(what_str)
{
}

::std::out_of_range::out_of_range(const ::std::string& what_str):
	logic_error(what_str)
{
}
