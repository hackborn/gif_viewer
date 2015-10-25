#include "cs_app.h"

#include <cinder/app/RendererGl.h>
#include <cinder/gl/gl.h>
#include <cinder/Filesystem.h>
#include "kt/app/kt_environment.h"
#include "kt/app/kt_string.h"
#include "gif_io/gif_file.h"
#include "texture_gif_list.h"

namespace cs {

namespace {
// True if the path is a GIF
bool						is_gif(const ci::fs::path&);
// True if the path is a non-GIF image (jpg, png);
bool						is_image(const ci::fs::path&);
}

GifApp::GifApp()
		: mEnvInitialized(kt::env::initialize()) 
		, mGifView(glm::ivec2(getWindowWidth(), getWindowHeight())) {
	
	// SETUP CAMERA
	const glm::ivec2	iwin_size(getWindowWidth(), getWindowHeight());
	const glm::vec2		win_size(static_cast<float>(iwin_size.x), static_cast<float>(iwin_size.y));
	mCameraOrtho.setOrtho(0.0f, win_size.x, win_size.y, 0.0f, -1.0f, 1.0f);
}

GifApp::~GifApp() {
	mQuit = true;
	try {
		mThread.join();
	} catch (std::exception const&) {
	}
}

void GifApp::prepareSettings(Settings* s) {
	if (s) {
		s->setTitle("GIF Viewer");
		s->setWindowSize(glm::ivec2(1920, 1080));
//		s->setFullScreen(true);
//		s->setConsoleWindowEnabled(true);
	}
}

void GifApp::setup() {
	base::setup();
	// Printing during construction creates an error state, preventing further output,
	// so clear that out, in case anyone did.
	std::cout.clear();

	mParams = ci::params::InterfaceGl::create("Params", glm::ivec2(220, 120));
	mParams->addParam("Frame",	&mFrame, "", true);
	mParams->addParam<float>("Speed", &mPlaybackSpeed, false).min(0).max(8).step(0.01f).precision(2);

	// Start a thread to handle the actual loading
	mQuit = false;
	ci::gl::ContextRef backgroundCtx = ci::gl::Context::create(ci::gl::context());
	mThread = std::thread( bind( &GifApp::gifThread, this, backgroundCtx));

	// Load the default GIF.
	auto	input = mThreadInput.make();
	input->mPaths.push_back(kt::env::expand("$(DATA)/tumblr_n8njbcmeWS1t9jwm6o1_400.gif"));
	mThreadInput.push(input);
}

void GifApp::keyDown(ci::app::KeyEvent e) {
	if( e.getChar() == 'f' ) {
		// Toggle full screen when the user presses the 'f' key.
		setFullScreen( ! isFullScreen() );
	}
	else if( e.getCode() == ci::app::KeyEvent::KEY_ESCAPE ) {
		// Exit full screen, or quit the application, when the user presses the ESC key.
		if( isFullScreen() )
			setFullScreen( false );
		else
			quit();
	}
}

void GifApp::fileDrop(ci::app::FileDropEvent e) {
	try {
		auto				input(makeInput(e));
		if (!input) return;

		if (input->mType == InputType::kSave) {
			input->mSavePath = getSaveFilePath().string();
			if (input->mSavePath.empty()) return;
		}
		mThreadInput.push(input);
	} catch (std::exception const &ex) {
		std::cout << "gif load error=" << ex.what() << std::endl;
	}
}

void GifApp::update() {
	// Get current GIF list
	auto		list = mThreadOutput.pop();
	if (list) {
		mGifView.setTextures(*list);
	}

	// Update the view
	mGifView.setPlaybackSpeed(mPlaybackSpeed);
	mGifView.update();

	// Update params
	mFrame = static_cast<int32_t>(mGifView.getCurrentFrame());
	mParams->hide();	// Params don't seem designed for a high update rate, this flushes it.
	mParams->show();
}

void GifApp::draw() {
	ci::gl::setMatrices(mCameraOrtho);
	ci::gl::clear();
	mGifView.draw();
	mParams->draw();
}

std::shared_ptr<GifApp::Input> GifApp::makeInput(const ci::app::FileDropEvent &e) const {
	auto				input = mThreadInput.make();
	if (!input) return nullptr;
	for (const auto& it : e.getFiles()) {
		// Only add gifs to load input, images to save input. Take the first of whatever I find
		if (is_gif(it)) {
			if (input->mPaths.empty() || input->mType == InputType::kLoad) {
				input->mType = InputType::kLoad;
				input->mPaths.push_back(it.string());
			}
		} else if (is_image(it)) {
			if (input->mPaths.empty() || input->mType == InputType::kSave) {
				input->mType = InputType::kSave;
				input->mPaths.push_back(it.string());
			}
		}
	}
	return input;
}

void GifApp::gifThread(ci::gl::ContextRef context) {
	ci::ThreadSetup					threadSetup;
	context->makeCurrent();

	while (!mQuit) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		try {
			auto					input = mThreadInput.pop();
			if (input) {
				if (input->mType == InputType::kLoad) {
					gifThreadLoad(input->mPaths);
				} else if (input->mType == InputType::kSave) {
					gifThreadSave(*input);
				}
			}
		} catch (std::exception const&) {
		}
	}
}

void GifApp::gifThreadLoad(const std::vector<std::string> &input) {
	auto				output = mThreadOutput.make();
	for (const auto& it : input) {
		gif::Reader		file(it);
		file.read(*output);
		// Ideally we'd have a way to signal the gif to quit if
		// we got the quit command in the middle of loading.
	}
	mThreadOutput.push(output);
}
void GifApp::gifThreadSave(const Input &input) {
	gif::Writer		file(input.mSavePath);
	file.write();
}

namespace {

bool						is_gif(const ci::fs::path &p) {
	std::string				ext = kt::to_lower(p.extension().string());
	return ext == ".gif";
}

bool						is_image(const ci::fs::path &p) {
	std::string				ext = kt::to_lower(p.extension().string());
	return ext == ".jpg" || ext == ".jpeg" || ext == ".png";
}

}

} // namespace cs

CINDER_APP(cs::GifApp, ci::app::RendererGl, cs::GifApp::prepareSettings)