/*
 */
#ifndef _COMMON_H_
#define _COMMON_H_

#include <exception>

namespace AxWin {

class InitFailure:
	public ::std::exception
{
	const char *m_what;
public:
	InitFailure(const char *reason):
		m_what(reason)
	{
	}
	
	virtual const char* what() const throw();
};

}	// namespace AxWin

#endif

