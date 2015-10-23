#ifndef CS_APP_H_
#define CS_APP_H_

#include "cinder/params/Params.h"

#include <cinder/gl/Context.h>
#include <cinder/app/App.h>
#include <cinder/Camera.h>
#include <cinder/Thread.h>
#include "safe_value.h"
#include "texture_gif_view.h"

namespace cs {

/**
 * @class cs::GifApp
 */
class GifApp : public ci::app::App {
private:
	// Small convience, initialize a class that can locate various file paths.
	const bool					mEnvInitialized;

public:
	GifApp();
	~GifApp();

	static void					prepareSettings(Settings*);
	void						setup() override;

	void						keyDown(ci::app::KeyEvent) override;
	void						fileDrop(ci::app::FileDropEvent) override;

	void						update() override;
	void						draw() override;

private:
	// Separate thread where all the file loading and saving occurs.
	void						gifThread(ci::gl::ContextRef);

	using base = ci::app::App;
	using StringVec = std::vector<std::string>;

	// Drawing
	ci::CameraOrtho				mCameraOrtho;
	TextureGifView				mGifView;

	// Params
	ci::params::InterfaceGlRef	mParams;
	int32_t						mFrame = 0;
	float						mPlaybackSpeed = 1.0f;

	// Gif loading
	std::thread					mThread;
	std::atomic_bool			mQuit;
	SafeValue<StringVec>		mThreadInput;
	SafeValue<TextureGifList>	mThreadOutput;
};

} // namespace cs

#endif