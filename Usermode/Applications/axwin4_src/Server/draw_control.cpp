/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang)
 *
 * draw_control.cpp
 * - Common "Control" Drawing
 *
 * Handles drawing of resizable controls defined by a bitmap and four region sizes
 */
#include <draw_control.hpp>
#include <axwin4/definitions.h>
#include <acess/sys.h>
#include <cassert>

// === CODE ===
namespace AxWin {

CControl::CControl(int EdgeX, int FillX, int InnerX, int EdgeY, int FillY, int InnerY, ::std::vector<uint32_t>&& data):
	m_edge_x(EdgeX),
	m_fill_x(FillX),
	m_inner_x(InnerX),
	m_edge_y(EdgeY),
	m_fill_y(FillY),
	m_inner_y(InnerY),
	m_data(data)
{
	_SysDebug("CControl(X={E:%i,F:%i,I:%i}, Y={E:%i,F:%i,I:%i}, data={Size:%i})",
		m_edge_x, m_fill_x, m_inner_x, m_edge_y, m_fill_y, m_inner_y, m_data.size());
}

void CControl::Render(CSurface& dest, const CRect& rect) const
{
	if( rect.m_w < m_edge_x*2 + m_fill_x*2 + m_inner_x )
		return ;
	if( rect.m_h < m_edge_y*2 + m_fill_y*2 + m_inner_y )
		return ;
	
	const int ctrl_width = m_edge_x + m_fill_x + m_inner_x + (m_inner_x ? m_fill_x : 0) + m_edge_x;
	
	const int top_fill_end = rect.m_h / 2 - m_inner_y;
	const int bot_fill_start = top_fill_end + m_inner_y;
	const int bot_fill_end   = rect.m_h - m_edge_y;
	
	::std::vector<uint32_t>	scanline( rect.m_w );
	 int	y = 0;
	 int	base_ofs = 0;
	// EdgeY
	for( int i = 0; i < m_edge_y; i ++ )
		renderLine(dest, y++, scanline, rect, &m_data[(base_ofs+i)*ctrl_width]);
	base_ofs += m_edge_y;
	// FillY
	assert(m_fill_y > 0 || y == top_fill_end);
	while( y < top_fill_end )
	{
		for( int i = 0; i < m_fill_y && y < top_fill_end; i ++ )
			renderLine(dest, y++, scanline, rect, &m_data[(base_ofs+i)*ctrl_width]);
	}
	base_ofs += m_fill_y;
	// InnerY
	if( m_inner_y > 0 )
	{
		for( int i = 0; i < m_inner_y; i ++ )
			renderLine(dest, y++, scanline, rect, &m_data[(base_ofs+i)*ctrl_width]);
		base_ofs += m_inner_y;
	}
	else
	{
		base_ofs -= m_fill_x;
	}
	// FillY
	while( y < bot_fill_end )
	{
		for( int i = 0; i < m_fill_y && y < bot_fill_end; i ++ )
			renderLine(dest, y++, scanline, rect, &m_data[(base_ofs+i)*ctrl_width]);
	}
	base_ofs += m_fill_y;
	// EdgeY
	for( int i = 0; i < m_edge_y; i ++ )
		renderLine(dest, y++, scanline, rect, &m_data[(base_ofs+i)*ctrl_width]);
	base_ofs += m_edge_y;
}

void CControl::renderLine(CSurface& dest, int y, ::std::vector<uint32_t>& scanline, const CRect& rect, const uint32_t* ctrl_line) const
{
	//_SysDebug("renderLine: (y=%i,rect={(%i,%i) %ix%i}", y, rect.m_x, rect.m_y, rect.m_w, rect.m_h);
	const int left_fill_end = rect.m_w / 2 - m_inner_x;
	const int right_fill_end = rect.m_w - m_edge_x;
	
	 int	x = 0;
	 int	base_ofs = 0;
	// EdgeX
	for( int i = 0; i < m_edge_x; i ++ )
		scanline[x++] = ctrl_line[base_ofs + i];
	base_ofs += m_edge_x;
	// FillX
	while( x < left_fill_end )
	{
		for( int i = 0; i < m_fill_x && x < left_fill_end; i ++ )
			scanline[x++] = ctrl_line[base_ofs + i];
	}
	base_ofs += m_fill_x;
	// InnerX
	if( m_inner_x > 0 )
	{
		for( int i = 0; i < m_inner_x; i ++ )
			scanline[x++] = ctrl_line[base_ofs + i];
		base_ofs += m_inner_x;
	}
	else
	{
		base_ofs -= m_fill_x;
	}
	// FillX
	while( x < right_fill_end )
	{
		for( int i = 0; i < m_fill_x && x < right_fill_end; i ++ )
			scanline[x++] = ctrl_line[base_ofs + i];
	}
	base_ofs += m_fill_x;
	// EdgeX
	for( int i = 0; i < m_edge_x; i ++ )
		scanline[x++] = ctrl_line[base_ofs + i];
	base_ofs += m_edge_x;
	
	dest.DrawScanline(rect.m_y + y, rect.m_x, rect.m_w, scanline.data());
}

// ---- Standard Controls ---
// Standard button control
CControl StdButton(2, 1, 0, 2, 1, 0, ::std::vector<uint32_t> {
	0xC0C0C0, 0xC0C0C0, 0xC0C0C0, 0xC0C0C0, 0xC0C0C0,
	0xC0C0C0, 0xFFD0D0, 0xFFD0D0, 0xFFD0D0, 0xC0C0C0,
	0xC0C0C0, 0xFFD0D0, 0xFFD0D0, 0xFFD0D0, 0xC0C0C0,
	0xC0C0C0, 0xFFD0D0, 0xFFD0D0, 0xFFD0D0, 0xC0C0C0,
	0xC0C0C0, 0xC0C0C0, 0xC0C0C0, 0xC0C0C0, 0xC0C0C0,
	});

// Toolbar
CControl StdToolbar(2, 1, 0, 2, 1, 0, ::std::vector<uint32_t> {
	0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
	0x000000, 0xA0A0A0, 0x0A0000, 0xA0A0A0, 0x000000,
	0x000000, 0xA0A0A0, 0xFFFFFF, 0xA0A0A0, 0x000000,
	0x000000, 0xA0A0A0, 0x0A0000, 0xA0A0A0, 0x000000,
	0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
	});

// Text Area
CControl StdText(2, 1, 0, 2, 1, 0, ::std::vector<uint32_t> {
	0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
	0x000000, 0xA0A0A0, 0x0A0000, 0xA0A0A0, 0x000000,
	0x000000, 0xA0A0A0, 0xFFFFFF, 0xA0A0A0, 0x000000,
	0x000000, 0xA0A0A0, 0x0A0000, 0xA0A0A0, 0x000000,
	0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
	});

const CControl* CControl::GetByName(const ::std::string& name)
{
	if( name == "StdButton" )
		return &StdButton;
	if( name == "StdText" )
		return &StdText;
	// TODO: Use another exception
	return nullptr;
}

const CControl* CControl::GetByID(uint16_t id)
{
	switch(id)
	{
	case AXWIN4_CTL_BUTTON:	return &StdButton;
	case AXWIN4_CTL_TOOLBAR:	return &StdToolbar;
	case AXWIN4_CTL_TEXTBOX:	return &StdText;
	default:	return nullptr;
	}
}

};	// AxWin

