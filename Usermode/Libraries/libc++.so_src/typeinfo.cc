/*
 * Acess2 C++ Library
 * - By John Hodge (thePowersGang)
 *
 * typeinfo.cc
 * - typeid and dynamic_cast
 */
#include <typeinfo>
#include <cxxabi.h>
#include <acess/sys.h>
#include <cstdlib>

namespace std {

type_info::~type_info()
{
	// nop
}

bool type_info::operator==(const type_info& other) const
{
	//_SysDebug("type_info::operator== - '%s' == '%s'", this->__type_name, other.__type_name);
	return this->__type_name == other.__type_name;
}

bool type_info::operator!=(const type_info& other) const
{
	//_SysDebug("type_info::operator!= - '%s' != '%s'", this->__type_name, other.__type_name);
	return this->__type_name != other.__type_name;
}

bool type_info::before(const type_info& other) const
{
	//_SysDebug("type_info::before - '%s' < '%s'", this->__type_name, other.__type_name);
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
	_SysDebug("type_info::operator=, was %s now %s", __type_name, rhs.__type_name);
	__type_name = rhs.__type_name;
	return *this;
}

bool type_info::is_class() const
{
	if( typeid(*this) == typeid(::__cxxabiv1::__class_type_info) )
		return true;
	if( is_subclass() )
		return true;
	return false;
}

bool type_info::is_subclass() const
{
	if( typeid(*this) == typeid(::__cxxabiv1::__si_class_type_info) )
		return true;
	if( typeid(*this) == typeid(::__cxxabiv1::__vmi_class_type_info) )
		return true;
	return false;
}

// Acess-defined
bool type_info::__is_child(const type_info &poss_child, unsigned long &offset) const
{
	_SysDebug("typeids = this:%s , poss_child:%s", typeid(*this).name(), typeid(poss_child).name());

	// Check #1: Child is same type
	if( poss_child == *this ) {
		offset = 0;
		return true;
	}
	
	// Check #2: This type must be a class
	if( !this->is_class() ) {
		return false;
	}
	// Check #3: Child class must be a subclass
	if( !poss_child.is_subclass() ) {
		return false;
	}
	
	if( typeid(poss_child) == typeid(::__cxxabiv1::__si_class_type_info) ) {
		auto &si_poss_child = reinterpret_cast<const ::__cxxabiv1::__si_class_type_info&>(poss_child);
		// Single inheritance
		_SysDebug("type_info::__is_child - Single inheritance");
		return __is_child( *si_poss_child.__base_type, offset );
	}
	else if( typeid(poss_child) == typeid(::__cxxabiv1::__vmi_class_type_info) ) {
		// Multiple inheritance
		_SysDebug("TODO: type_info::__is_child - Multiple inheritance");
		abort();
		for(;;);
	}
	else {
		// Oops!
		_SysDebug("ERROR: type_info::__is_child - Reported subclass type, but not a subclass %s",
			typeid(poss_child).name()
			);
		abort();
		for(;;);
	}
}


// NOTE: Not defined by the C++ ABI, but is similar to one defined by GCC (__cxa_type_match
//bool __acess_type_match(std::typeinfo& possibly_derived, const std::typeinfo& base_type)
//{
//	return false;
//}

};	// namespace std
