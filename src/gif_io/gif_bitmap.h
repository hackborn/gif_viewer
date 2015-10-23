#ifndef GIFIO_GIFBITMAP_H_
#define GIFIO_GIFBITMAP_H_

#include <vector>
#include "gif_color.h"

namespace gif {

/**
 * @class gif::Bitmap
 * @brief A local bitmap definition, an array of colours.
 */
class Bitmap {
public:
	Bitmap() { }
	Bitmap(const int32_t w, const int32_t h) : mWidth(w), mHeight(h) {
		if (w > 0 && h > 0) mPixels.resize(w * h);
	}

	int32_t						mWidth = 0,
								mHeight = 0;
	std::vector<gif::ColorA8u>	mPixels;
};

} // namespace gif

#endif
