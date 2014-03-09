/*
 * Acess2 C++ Library
 * - By John Hodge (thePowersGang)
 *
 * typeinfo.cc
 * - typeid and dynamic_cast
 */
#include <typeinfo>

namespace std {

type_info::~type_info()
{
	// nop
}

bool type_info::operator==(const type_info& other) const
{
	return this->__type_name == other.__type_name;
}

bool type_info::operator!=(const type_info& other) const
{
	return this->__type_name != other.__type_name;
}

bool type_info::before(const type_info& other) const
{
	return this->__type_name < other.__type_name;
}

const char *type_info::name() const
{
	return this->__type_name;
}

// Private
type_info::type_info(const type_info& rhs):
	__type_name(rhs.__type_name)
{
}
type_info& type_info::operator=(const type_info& rhs)
{
	__type_name = rhs.__type_name;
	return *this;
}


};	// namespace std

