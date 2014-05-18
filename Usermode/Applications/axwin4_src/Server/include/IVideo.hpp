/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang)
 *
 * IVideo.hpp
 * - Graphics backend abstraction
 */
#ifndef _IVIDEO_HPP_
#define _IVIDEO_HPP_

namespace AxWin {

class IVideo
{
public:
	virtual ~IVideo();
	
	// Allocate a new hardware surface
	IHWSurface&	AllocateHWSurface(uint16_t Width, uint16_t Height);
	
	// Request redraw of backbuffer
	void	Flip();
};

};

#endif

