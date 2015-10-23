#ifndef APP_TEXTUREGIFVIEW_H_
#define APP_TEXTUREGIFVIEW_H_

#include <cinder/gl/Batch.h>
#include "kt/time/seconds.h"
#include "texture_gif_list.h"

namespace cs {

/**
 * @class cs::TextureGifView
 * @brief Draw the gif list.
 */
class TextureGifView {
public:
	TextureGifView() = delete;
	TextureGifView(const TextureGifView&) = delete;
	TextureGifView(const glm::ivec2 &window_size);

	void					update();
	void					draw();

	void					setTextures(const TextureGifList&);
	size_t					getCurrentFrame() const { return mTextureIndex; }
	void					setPlaybackSpeed(const float = 1.0f);

private:
	void					restartTimer();
	// Answer the frame rate of the current frame, or the default.
	double					findFrameRate() const;

	TextureGifList			mTextures;
	size_t					mTextureIndex = 0;
	kt::time::Seconds		mTimer;
	double					mNextTime;
	float					mPlaybackSpeed = 1.0f,
							mNextPlaybackSpeed = 1.0f;
	ci::gl::BatchRef		mBatch;
};

} // namespace cs

#endif
