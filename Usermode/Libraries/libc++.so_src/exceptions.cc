/*
 * Acess2 C++ Library
 * - By John Hodge (thePowersGang)
 *
 * exceptions.cc
 * - exception and friends
 */
#include <string>
#include <exception>
#include <stdexcept>
#include <acess/sys.h>

// === CODE ===
namespace std {

exception::exception() throw()
{
}
exception::exception(const exception&) throw()
{
}
exception& exception::operator=(const exception&) throw()
{
	return *this;
}
exception::~exception() throw()
{
}
const char* exception::what() const throw()
{
	return "generic exception";
}

void terminate()
{
	_SysDebug("terminate()");
	_exit(0);
}

_bits::str_except::str_except(const string& what_arg):
	m_str(what_arg)
{
}
_bits::str_except::~str_except() noexcept
{
}
_bits::str_except& _bits::str_except::operator=(const str_except& e) noexcept
{
	m_str = e.m_str;
	return *this;
}
const char* _bits::str_except::what() const throw()
{
	return m_str.c_str();
}

// --- Standar Exceptions ---
logic_error::logic_error(const string& what_str):
	_bits::str_except(what_str)
{
}

out_of_range::out_of_range(const string& what_str):
	logic_error(what_str)
{
}

length_error::length_error(const string& what_str):
	logic_error(what_str)
{
}

runtime_error::runtime_error(const string& what_str):
	_bits::str_except(what_str)
{
}

}	// namespace std

