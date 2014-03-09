/*
 * Acess2 C++ Library
 * - By John Hodge (thePowersGang)
 *
 * cxxabi.cc
 * - C++ ABI Namespace
 */
#include <cxxabi.h>

namespace __cxxabiv1 {

// --- RTTI --
// - No inheritance class
__class_type_info::~__class_type_info()
{
	// nop
}

// - Single inheritance class
__si_class_type_info::~__si_class_type_info()
{
	
}

// - Multiple inheritance class
__vmi_class_type_info::~__vmi_class_type_info()
{
	
}

};	// namespace __cxxabiv1

