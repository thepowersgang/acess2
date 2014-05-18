/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang) 
 *
 * input.hpp
 * - Input Interface Header
 */
#ifndef _INPUT_H_
#define _INPUT_H_

#include <acess/sys.h>

namespace AxWin {
namespace Input {

extern void	Initialise(const CConfigInput& config);
extern int	FillSelect(::fd_set& rfds);
extern void	HandleSelect(::fd_set& rfds);

};
};

#endif

