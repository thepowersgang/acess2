/*
 * Acess2 C++ Library
 * - By John Hodge (thePowersGang)
 *
 * system_error.cc
 * - ::std::system_error and other helpers
 */
#include <system_error>
#include <cerrno>

namespace std {

system_error::system_error(::std::error_code ec):
	m_error_code(ec),
	m_what_str( (::std::string)ec.category().name() + ":" + ec.message())
{
	::_sys::debug("system_error(%s:%s)", ec.category().name(), ec.message().c_str());
}
system_error::system_error(::std::error_code ec, const ::std::string& what_arg):
	system_error(ec)
{
	m_what_str += " - ";
	m_what_str += what_arg;
}
system_error::system_error(::std::error_code ec, const char* what_arg):
	system_error(ec)
{
	m_what_str += " - ";
	m_what_str += what_arg;
}
system_error::system_error(int ev, const ::std::error_category& ecat):
	system_error( ::std::error_code(ev, ecat) )
{
}
system_error::system_error(int ev, const ::std::error_category& ecat, const ::std::string& what_arg):
	system_error(ev, ecat)
{
	m_what_str += " - ";
	m_what_str += what_arg;
}
system_error::system_error(int ev, const ::std::error_category& ecat, const char* what_arg):
	system_error(ev, ecat)
{
	m_what_str += " - ";
	m_what_str += what_arg;
}

system_error::~system_error() noexcept
{
}

const char* system_error::what() const noexcept
{
	return m_what_str.c_str();
}


bool error_category::equivalent(const error_code& code, int valcond) const noexcept {
	return *this == code.category() && code.value() == valcond;
}


class class_generic_category:
	public error_category
{
public:
	class_generic_category() {
	}
	~class_generic_category() noexcept {
	}
	const char *name() const noexcept {
		return "generic";
	}
	::std::string message(int val) const {
		return ::std::string( ::strerror(val) );
	}
} g_generic_category;

const ::std::error_category& generic_category() noexcept
{
	return g_generic_category;
}


class class_system_category:
	public error_category
{
public:
	class_system_category() {
	}
	~class_system_category() noexcept {
	}
	const char *name() const noexcept {
		return "system";
	}
	::std::string message(int val) const {
		return ::std::string( ::strerror(val) );
	}
} g_system_category;

const ::std::error_category& system_category() noexcept
{
	return g_system_category;
}

};	// namespace std

