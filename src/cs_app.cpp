#include "cs_app.h"

#include <cinder/app/RendererGl.h>
#include <cinder/gl/gl.h>
#include "kt/app/kt_environment.h"
#include "gif_io/gif_file.h"
#include "texture_gif_list.h"

namespace cs {

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
	// Printing during construction creates an error, so clear that out, in case anyone did.
	std::cout.clear();

	mParams = ci::params::InterfaceGl::create("Params", glm::ivec2(220, 120));
	mParams->addParam("Frame",	&mFrame, "", true);
	mParams->addParam<float>("Speed", &mPlaybackSpeed, false).min(0).max(8).step(0.01f).precision(2);

	// Start a thread to handle the actual loading
	mQuit = false;
	ci::gl::ContextRef backgroundCtx = ci::gl::Context::create(ci::gl::context());
	mThread = std::thread( bind( &GifApp::gifLoadThread, this, backgroundCtx));

	// Load the default GIF.
	auto	input = mThreadInput.make();
	input->push_back(kt::env::expand("$(DATA)/tumblr_n8njbcmeWS1t9jwm6o1_400.gif"));
	mThreadInput.push(input);
}

void GifApp::mouseDrag(ci::app::MouseEvent e) {
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
		auto				strings = mThreadInput.make();
		for (const auto& it : e.getFiles()) {
			strings->push_back(it.string());
		}
		mThreadInput.push(strings);
	} catch (std::exception const &ex) {
		std::cout << "gif load error=" << ex.what() << std::endl;
	}
}

void GifApp::update() {
	// Get current gif list
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

void GifApp::gifLoadThread(ci::gl::ContextRef context) {
	ci::ThreadSetup threadSetup;
	context->makeCurrent();

	gif::File						gif;
	while (!mQuit) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		try {
			auto					input = mThreadInput.pop();
			if (input) {
				auto				output = mThreadOutput.make();
				for (const auto& it : (*input)) {
					gif.load(it, *output);
					// Ideally we'd have a way to signal the gif to quit if
					// we got the quit command in the middle of loading.
				}
				mThreadOutput.push(output);
			}
		} catch (std::exception const&) {
		}
	}
}

} // namespace cs

// This line tells Cinder to actually create and run the application.
CINDER_APP(cs::GifApp, ci::app::RendererGl, cs::GifApp::prepareSettings)