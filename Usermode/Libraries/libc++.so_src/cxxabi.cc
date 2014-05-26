/*
 * Acess2 C++ Library
 * - By John Hodge (thePowersGang)
 *
 * cxxabi.cc
 * - C++ ABI Namespace
 * 
 * NOTE: GCC follows the Itanium™ C++ ABI on all platforms
 * http://mentorembedded.github.io/cxx-abi/abi.html
 * http://libcxxabi.llvm.org/spec.html
 */
#include <cxxabi.h>
#include <acess/sys.h>

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

extern "C" void __cxa_bad_cast ()
{
	_SysDebug("__cxa_bad_cast");
	for(;;);
	//throw ::std::bad_cast;
}

extern "C" void __cxa_bad_typeid ()
{
	_SysDebug("__cxa_bad_typeid");
	for(;;);
	//throw ::std::bad_typeid;
}

