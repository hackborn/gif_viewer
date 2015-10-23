#ifndef APP_TEXTUREGIFLIST_H_
#define APP_TEXTUREGIFLIST_H_

#include <cinder/gl/Texture.h>
#include "gif_io/gif_list.h"

#include <utility>

namespace cs {

/**
 * @class cs::TextureGifList
 * @brief Provide a list of local Textures.
 * @description This class is more complex than it should be -- it should
 * just store the list of textures, but as soon as I moved to generating
 * them on a different thread it started having problems, so now it still
 * loads the surfaces on a separate thread, but converts them to textures
 * on the main thread.
 */
using ImagePair = std::pair<ci::Surface8u, ci::gl::TextureRef>;
class TextureGifList : public gif::List<ImagePair> {
public:
	TextureGifList();

	void				makeTextures();

private:
	ImagePair			convert(const gif::Bitmap&) const;

	using base = gif::List<ImagePair>;
};

} // namespace cs

#endif
