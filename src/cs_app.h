#ifndef APP_APP_H_
#define APP_APP_H_

#include "cinder/params/Params.h"

#include <cinder/app/App.h>
#include <cinder/gl/Context.h>
#include <cinder/Camera.h>
#include <cinder/Thread.h>
#include "safe_value.h"
#include "texture_gif_view.h"

namespace gif { class File; }
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
	using base = ci::app::App;
	using StringVec = std::vector<std::string>;

	enum class InputType		{ kLoad, kSave };
	struct Input {
		InputType				mType = InputType::kLoad;
		StringVec				mPaths;
		std::string				mSavePath;
	};

	std::shared_ptr<Input>		makeInput(const ci::app::FileDropEvent&) const;
	// Separate thread where all the file loading and saving occurs.
	void						gifThread(ci::gl::ContextRef);
	void						gifThreadLoad(const std::vector<std::string>&, gif::File&);
	void						gifThreadSave(const Input&, gif::File&);

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
	SafeValue<Input>			mThreadInput;
	SafeValue<TextureGifList>	mThreadOutput;
};

} // namespace cs

#endif