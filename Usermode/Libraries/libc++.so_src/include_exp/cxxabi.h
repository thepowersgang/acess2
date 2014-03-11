/*
 * Acess2 C++ Library
 * - By John Hodge (thePowersGang)
 *
 * cxxabi.h
 * - C++ ABI Namespace
 */
#ifndef _LIBCXX__CXXABI_H_
#define _LIBCXX__CXXABI_H_

#include <typeinfo>

namespace __cxxabiv1 {

class __class_type_info : public std::type_info
{
public:
	virtual ~__class_type_info();
};

class __si_class_type_info : public __class_type_info
{
public:
	virtual ~__si_class_type_info();
	
	const __class_type_info	*__base_type;
};

struct __base_class_type_info
{
public:
	
	const __class_type_info *__base_type;
	long __offset_flags;

	enum __offset_flags_masks {
		__virtual_mask = 0x1,
		__public_mask = 0x2,
		__offset_shift = 8
	};

};

class __vmi_class_type_info : public __class_type_info
{
public:
	virtual ~__vmi_class_type_info();
	
	unsigned int	__flags;
	unsigned int	__base_count;
	__base_class_type_info	__base_info[1];
	
	enum __flags_masks {
		__non_diamond_repeat_mask = 0x1,
		__diamond_shaped_mask = 0x2,
	};
};

class __pbase_type_info : public std::type_info
{
public:
	unsigned int __flags;
	const std::type_info *__pointee;

	enum __masks {
		__const_mask = 0x1,
		__volatile_mask = 0x2,
		__restrict_mask = 0x4,
		__incomplete_mask = 0x8,
		__incomplete_class_mask = 0x10
	};
};

};	// namespace __cxxabiv1

#endif
