
#ifndef _LIBCXX__LIBCXX_HELEPRS_H_
#define _LIBCXX__LIBCXX_HELEPRS_H_

#if __cplusplus > 199711L	// C++11 check
# define _CXX11_AVAIL	1
#else
# define _CXX11_AVAIL	0
#endif

namespace _sys {
extern void debug(const char *, ...);
};

#endif

