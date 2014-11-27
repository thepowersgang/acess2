/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang)
 *
 * compositor.cpp
 * - Window compositor
 */
#include <video.hpp>
#include <CCompositor.hpp>
#include <CClient.hpp>
#include <ipc.hpp>
#include <cassert>

namespace AxWin {

CCompositor::CCompositor(CVideo& video):
	// TODO: Support multiple screens
	m_video(video),
	m_windowIDBuffer(video.width(), video.height())
{
	// 
}

void CCompositor::ShowWindow(CWindow* window)
{
	DamageArea(window->m_surface.m_rect);
	// TODO: Append to separate sub-lists (or to separate lists all together)
	//   if flags AXWIN4_WNDFLAG_KEEPBELOW or AXWIN4_WNDFLAG_KEEPABOVE are set
	m_windows.push_back(window);
}
void CCompositor::HideWindow(CWindow* window)
{
	DamageArea(window->m_surface.m_rect);
	m_windows.remove(window);
}

bool CCompositor::GetScreenDims(unsigned int ScreenID, unsigned int* W, unsigned int* H)
{
	assert(W && H);
	if( ScreenID != 0 )
	{
		*W = 0;
		*H = 0;
		return false;
	}
	else
	{
		m_video.GetDims(*W, *H);
		return true;
	}
}

void CCompositor::Redraw()
{
	// Redraw the screen and clear damage rects
	if( m_damageRects.empty() ) {
		_SysDebug("- No damaged regions");
		return ;
	}
	
	// Build up foreground grid (Rects and windows)
	// - This should already be built (mutated on window move/resize/reorder)
	
	// For all windows, check for intersection with damage rects
	for( auto rect : m_damageRects )
	{
		// window list should be sorted by draw order (lowest first)
		for( auto window : m_windows )
		{
			if( window->m_is_shown && rect.HasIntersection( window->m_surface.m_rect ) )
			{
				// TODO: just reblit
				CRect	rel_rect = window->m_surface.m_rect.RelativeIntersection(rect);
				_SysDebug("Reblit (%i,%i) %ix%i", rel_rect.m_x, rel_rect.m_y, rel_rect.m_w, rel_rect.m_h);
				BlitFromSurface( window->m_surface, rel_rect );
				//window->Repaint( rel_rect );
				m_windowIDBuffer.set(rel_rect.m_x, rel_rect.m_y, rel_rect.m_w, rel_rect.m_h, window);
			}
		}
		
		// TODO: Blit from windows to a local surface, then blit from there to screen here
	}

	m_damageRects.clear();
	m_video.Flush();
}

void CCompositor::DamageArea(const CRect& area)
{
	m_damageRects.push_back( area );
	// 1. Locate intersection with any existing damaged areas
	// 2. Append after removing intersections
}

void CCompositor::BlitFromSurface(const CSurface& dest, const CRect& src_rect)
{
	for( unsigned int i = 0; i < src_rect.m_h; i ++ )
	{
		m_video.BlitLine(
			dest.GetScanline(src_rect.m_y+i, src_rect.m_x),
			dest.m_rect.m_y + src_rect.m_y + i,
			dest.m_rect.m_x + src_rect.m_x,
			src_rect.m_w
			);
	}
}

void CCompositor::MouseMove(unsigned int Cursor, unsigned int X, unsigned int Y, int dX, int dY)
{
	//_SysDebug("MouseButton(%i, %i,%i, %+i,%+i)", Cursor, X, Y, dX, dY);
	m_video.SetCursorPos(X+dX, Y+dY);
	CWindow	*dstwin = getWindowForCoord(X, Y);
	if( dstwin )
	{
		// TODO: Pass event on to window
	}
}

void CCompositor::MouseButton(unsigned int Cursor, unsigned int X, unsigned int Y, eMouseButton Button, bool Press)
{
	_SysDebug("MouseButton(%i, %i,%i, %i=%i)", Cursor, X, Y, Button, Press);
	CWindow	*dstwin = getWindowForCoord(X, Y);
	_SysDebug("- dstwin = %p", dstwin);
	if( dstwin )
	{
		// 1. Give focus and bring to front
		// 2. Send event
		// TODO: Pass event on to window
	}
}

void CCompositor::KeyState(unsigned int KeyboardID, uint32_t KeySym, bool Press, uint32_t Codepoint)
{
	_SysDebug("KeyState(%i, 0x%x, %b, 0x%x)", KeyboardID, KeySym, Press, Codepoint);
}

CWindow* CCompositor::getWindowForCoord(unsigned int X, unsigned int Y)
{
	return m_windowIDBuffer.get(X, Y);
}

// --------------------------------------------------------------------
CWindowIDBuffer::CWindowIDBuffer(unsigned int W, unsigned int H):
	m_w(W),
	m_buf(W*H)
{
}
void CWindowIDBuffer::set(unsigned int X, unsigned int Y, unsigned int W, unsigned int H, CWindow* win)
{
	TWindowID	ent = {
		.Client = win->client().id(),
		.Window = win->id(),
		};
	for( unsigned int row = 0; row < H; row ++ )
	{
		TWindowID* dst = &m_buf[ (Y+row) * m_w ];
		for( unsigned int col = 0; col < W; col ++ )
			dst[col] = ent;
	}
}
CWindow* CWindowIDBuffer::get(unsigned int X, unsigned int Y)
{
	if( X >= m_w )
		return nullptr;
	unsigned int pos = Y*m_w + X;
	if( pos >= m_buf.size() )
		return nullptr;
	auto id = m_buf[pos];
	//_SysDebug("CWindowIDBuffer::get id = {%i,%i}", id.Client, id.Window);
	auto client = ::AxWin::IPC::GetClientByID(id.Client);
	if( client == nullptr ) {
		//_SysDebug("CWindowIDBuffer::get client=%p", client);
		return nullptr;
	}
	return client->GetWindow(id.Window);
}

}	// namespace AxWin

