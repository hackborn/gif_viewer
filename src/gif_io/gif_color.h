#ifndef GIFIO_GIFCOLOR_H_
#define GIFIO_GIFCOLOR_H_

#include <cstdint>

namespace gif {

/**
 * @class gif::ColorA8u
 * @brief A local color defintion, RGBA with 8 bits per pixel.
 */
struct ColorA8u {
	ColorA8u() { }
	ColorA8u(const uint8_t _r, const uint8_t _g, const uint8_t _b, const uint8_t _a = 255) : r(_r), g(_g), b(_b), a(_a) { }

	uint8_t		r = 0,
				g = 0,
				b = 0,
				a = 0;
};

} // namespace gif

#endif
