/*
 */
#ifndef _DRAW_CONTROL_HPP_
#define _DRAW_CONTROL_HPP_

#include <vector>
#include <string>
#include <cstdint>
#include <CSurface.hpp>

namespace AxWin {

class CControl
{
	unsigned int	m_edge_x;
	unsigned int	m_edge_y;
	unsigned int	m_fill_x;
	unsigned int	m_fill_y;
	unsigned int	m_inner_x;
	unsigned int	m_inner_y;
	::std::vector<uint32_t>	m_data;
public:
	CControl(int EdgeX, int FillX, int InnerX, int EdgeY, int FillY, int InnerY, ::std::vector<uint32_t>&& data);
	void Render(CSurface& dest, const CRect& rect) const;
	
	static const CControl*	GetByName(const ::std::string& name);
	static const CControl*	GetByID(uint16_t id);
	
private:
	void renderLine(CSurface& dest, int y, ::std::vector<uint32_t>& scanline, const CRect& rect, const uint32_t* ctrl_line) const;
};


}

#endif

