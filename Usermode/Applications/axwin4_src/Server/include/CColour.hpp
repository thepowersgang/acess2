/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang)
 *
 * CColour.hpp
 * - Generic colour handling and blending
 */
#ifndef _CCOLOUR_HPP_
#define _CCOLOUR_HPP_

namespace AxWin {

class CColour
{
	static const unsigned int comp_max = 0x7FFF;
	
	unsigned int	m_alpha;
	unsigned int	m_red;
	unsigned int	m_green;
	unsigned int	m_blue;

private:
	static unsigned int u8_to_ui(uint8_t u8v) {
		return (unsigned int)u8v * comp_max / UINT8_MAX;
	}
	static uint8_t ui_to_u8(unsigned int uiv) {
		return uiv * UINT8_MAX / comp_max;
	}
	// Perform an alpha-based blend on two components
	static unsigned int alpha_blend(unsigned int alpha_comp, unsigned int left, unsigned int right) {
		return (left * (comp_max - alpha_comp) + right * alpha_comp) / comp_max;
	}
	// Float values:
	// - infinity == saturation, 1 == nothing
	// fv = MAX / (MAX - uiv)
	static float ui_to_float(unsigned int uiv) {
		return (float)comp_max / (comp_max - uiv);
	}
	// uiv = MAX - MAX / fv
	static unsigned int float_to_ui(float fv) {
		return comp_max - comp_max / fv;
	}
	// perform a non-oversaturating blend of two colours (using an inverse relationship)
	static unsigned int add_blend(unsigned int a, unsigned int b) {
		return float_to_ui( ui_to_float(a) + ui_to_float(b) );
	}

	CColour(unsigned int r, unsigned int g, unsigned int b, unsigned int a):
		m_alpha(a), m_red(r), m_green(g), m_blue(b)
	{
	}
public:

	static CColour from_argb(uint32_t val) {
		return CColour(
			u8_to_ui((val>>16) & 0xFF),
			u8_to_ui((val>> 8) & 0xFF),
			u8_to_ui((val>> 0) & 0xFF),
			u8_to_ui((val>>24) & 0xFF)
			);
	}
	
	uint32_t to_argb() const {
		uint32_t	rv = 0;
		rv |= (uint32_t)ui_to_u8(m_red)   << 16;
		rv |= (uint32_t)ui_to_u8(m_green) <<  8;
		rv |= (uint32_t)ui_to_u8(m_blue)  <<  0;
		rv |= (uint32_t)ui_to_u8(m_alpha) << 24;
		return rv;
	}
	
	// performs a blend of the two colours, maintaining the target alpha, using the source alpha as the blend control
	CColour& blend(const CColour& other) {
		m_red   = alpha_blend(other.m_alpha, m_red  , other.m_red  );
		m_green = alpha_blend(other.m_alpha, m_green, other.m_green);
		m_blue  = alpha_blend(other.m_alpha, m_blue , other.m_blue );
		return *this;
	}
	// Add all components
	CColour& operator+(const CColour& other) {
		m_alpha = add_blend(m_alpha, other.m_alpha);
		m_red   = add_blend(m_red  , other.m_red  );
		m_green = add_blend(m_green, other.m_green);
		m_blue  = add_blend(m_blue , other.m_blue );
		return *this;
	}
};

}	// namespace AxWin

#endif

