
#ifndef _LIBCXX__LIBCXX_HELEPRS_H_
#define _LIBCXX__LIBCXX_HELEPRS_H_

#if __cplusplus > 199711L	// C++11 check
# define _CXX11_AVAIL	1
#else
# define _CXX11_AVAIL	0
#endif

#define _libcxx_assert(cnd) do { \
	if(!(cnd)) {\
		::_sys::debug("libc++ assert failure %s:%i - %s", __FILE__, __LINE__, #cnd);\
		::_sys::abort(); \
	} \
} while(0)

namespace _sys {
extern void abort() __asm__ ("abort") __attribute__((noreturn));
extern void debug(const char *, ...);
};

#if _CXX11_AVAIL
#define _CXX11_MOVE(val)	::std::move(val)
#else
#define _CXX11_MOVE(val)	val
#endif

#endif

