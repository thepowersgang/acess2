/*
 * Acess2 C++ Library
 * - By John Hodge (thePowersGang)
 *
 * mutex.cc
 * - ::std::mutex and helpers
 */
#include <mutex>
#include <system_error>
#include <cerrno>

// === CODE ===
::std::mutex::~mutex()
{
	
}

void ::std::mutex::lock()
{
	if( m_flag )
	{
		_sys::debug("TODO: userland mutexes");
		throw ::std::system_error(ENOTIMPL, ::std::system_category());
	}
	m_flag = true;
}

bool ::std::mutex::try_lock()
{
	bool rv = m_flag;
	m_flag = true;
	return !rv;
}

void ::std::mutex::unlock()
{
	m_flag = false;
}

