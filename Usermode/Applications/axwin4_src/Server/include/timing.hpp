/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang) 
 *
 * timing.hpp
 * - Timing Interface Header
 */
#ifndef _TIMING_H_
#define _TIMING_H_

#include <cstdint>

namespace AxWin {
namespace Timing {

extern ::int64_t	GetTimeToNextEvent();
extern void	CheckEvents();

};	// namespace Timing
};	// namespace AxWin

#endif

