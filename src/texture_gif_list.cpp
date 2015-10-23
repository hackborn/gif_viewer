#include "texture_gif_list.h"

#include <stdexcept>

#include <cinder/ImageIo.h>
#include "kt/app/kt_environment.h"

namespace cs {

/**
 * @class cs::TextureGifList
 */
TextureGifList::TextureGifList()
		: base([this](const gif::Bitmap &bm)->ImagePair { return convert(bm); }) {
}

void TextureGifList::makeTextures() {
	ci::gl::Texture2d::Format		fmt;
	fmt.loadTopDown(true);

	for (auto& f : mFrames) {
		f.mBitmap.second = ci::gl::Texture2d::create(f.mBitmap.first, fmt);
	}
}

ImagePair TextureGifList::convert(const gif::Bitmap &bm) const {
	ImagePair			ans;
	if (bm.mPixels.empty()) return ans;
	// Error condition, should never happen
	if (bm.mPixels.size() != static_cast<size_t>(bm.mWidth*bm.mHeight)) throw std::runtime_error("Bitmap pixels do not match size");

	ans.first = ci::Surface8u(bm.mWidth, bm.mHeight, true);
	ci::Surface8u&		dest(ans.first);
	auto				src(bm.mPixels.begin());
	auto				pix = dest.getIter();
	while (pix.line()) {
		while (pix.pixel()) {
			pix.r() = src->r;
			pix.g() = src->g;
			pix.b() = src->b;
			pix.a() = 255;

			++src;
		}
	}
	return ans;
#if 0
	ci::gl::Texture2d::Format		fmt;
	fmt.loadTopDown(true);
	return ci::gl::Texture2d::create(dest, fmt);
#endif
}

} // namespace cs
