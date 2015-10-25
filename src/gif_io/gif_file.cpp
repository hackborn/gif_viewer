#include "gif_file.h"

#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>
#include "gif_list.h"
#include "lzw_reader.h"

namespace gif {

namespace {
class GraphicControlExtension;
using GraphicControlExtensionRef = std::shared_ptr<GraphicControlExtension>;

const std::string	SIG("GIF");
enum class Version { kMissing, k87a, k89a };

size_t				color_count(const size_t encoded) {
	return static_cast<size_t>(std::pow(2, encoded+1));
}

int32_t				read_2_byte_int(const std::vector<char> &buffer, size_t &position) {
	uint8_t		a = buffer[position++],
				b = buffer[position++];
	return (b<<8) | a;
}

std::string			read_string(const std::vector<char> &buffer, const size_t size, size_t &position) {
	std::stringstream	buf;
	for (size_t k=0; k<size; ++k) buf << buffer[position++];
	return buf.str();
}

struct ColorTable {
	std::vector<gif::ColorA8u>	mColors;

	size_t			read(const std::vector<char> &buffer, const size_t count, size_t position) {
		for (size_t k=0; k<count; ++k) {
			const uint8_t	r = buffer[position++],
							g = buffer[position++],
							b = buffer[position++];
			mColors.push_back(gif::ColorA8u(r, g, b));
		}
		return position;
	}
};

// A place to stuff common read info, as well as any scratch data
struct BlockReadArgs {
	BlockReadArgs() = delete;
	BlockReadArgs(const BlockReadArgs&) = delete;
	BlockReadArgs(const int32_t screen_w, const int32_t screen_h, const ColorTable &global_ct, gif::ListConstructor &lc)
			: mScreenWidth(screen_w), mScreenHeight(screen_h), mGlobalColorTable(global_ct), mConstructor(lc) { }

	// Create the table and initialize the bitmap
	// Provide the target area within the bitmap.
	void						startLzwDecode(const int32_t left, const int32_t top, const int32_t width, const int32_t height) {
		mBitmap.mWidth = mScreenWidth;
		mBitmap.mHeight = mScreenHeight;
		mBitmap.mPixels.resize(mScreenWidth * mScreenHeight);
		mBitmapIndexX = left;
		mBitmapIndexY = top;
		mLeft = left;
		mTop = top;
		mRight = left + width;
		mBottom = top + height;
	}

	// A little annoying but this is defined later in the file because the Graphic Control Extension
	// is not declared at this point.
	void						addPixels(const std::vector<uint8_t> &indexes, const ColorTable &t);

	const int32_t				mScreenWidth,
								mScreenHeight;
	const ColorTable&			mGlobalColorTable;

	// Decoding
	gif::LzwReader				mDecoder;

	// A single bitmap is constructed and maintained through each successive image,
	// since the spec lets additional image data blocks leave pixels unmodified.
	gif::Bitmap					mBitmap;
	int32_t						mBitmapIndexX = 0,
								mBitmapIndexY = 0;
	// Target area, exclusive
	int32_t						mLeft = 0, mTop = 0, mRight = 0, mBottom = 0;

	// Will be cached from any GCE block before the current image block
	GraphicControlExtensionRef	mGceRef;

	// Output
	gif::ListConstructor&		mConstructor;
};

struct Header {
	Header() { }
	Header(const std::string &sig, const Version &v) : mSig(sig), mVersion(v) { }

	std::string		mSig;
	Version			mVersion = Version::kMissing;

	bool			isGif() const { return mSig == SIG; }

	size_t			read(const std::vector<char> &buffer, size_t position) {
		mSig = read_string(buffer, 3, position);

		std::string	v = read_string(buffer, 3, position);
		if (v == "87a") mVersion = Version::k87a;
		else if (v == "89a") mVersion = Version::k89a;

		return position;
	}

	void			write(std::ostream &buf) {
		buf << mSig;
		if (mVersion == Version::k87a) buf << "87a";
		else if (mVersion == Version::k89a) buf << "89a";
	}
};

struct LogicalScreen {
	static const uint32_t	GLOBAL_COLOR_TABLE_F = (1<<0);
	static const uint32_t	SORT_F = (1<<1);

	int32_t			mScreenWidth = 0, mScreenHeight = 0;
	uint32_t		mFlags = 0;
	uint8_t			mColorResolution = 0,
					mSizeOfGlobalColorTable = 0,
					mBackgroundColorIndex = 0,
					mPixelAspectRatio = 0;

	bool			hasGlobalColorTable() const { return (mFlags&GLOBAL_COLOR_TABLE_F) != 0; }

	size_t			read(const std::vector<char> &buffer, size_t position) {
		mScreenWidth = read_2_byte_int(buffer, position);
		mScreenHeight = read_2_byte_int(buffer, position);

		uint8_t		a = buffer[position++];
		if ((a&(1<<7)) != 0) mFlags |= GLOBAL_COLOR_TABLE_F;
		mColorResolution = (a>>4)&0x7;
		if ((a&(1<<3)) != 0) mFlags |= SORT_F;
		mSizeOfGlobalColorTable = (a&0x7);

		mBackgroundColorIndex = buffer[position++];
		
		mPixelAspectRatio = buffer[position++];

		return position;
	}
};

class Data {
public:
	Data() { }

	std::vector<char>	mData;
};
using DataRef = std::shared_ptr<Data>;

class Block {
public:
	Block() { }
	virtual ~Block() { }

	std::vector<DataRef>	mSubBlocks;

	// Generic utility to read blocks
	size_t					readSubBlocks(const std::vector<char> &buffer, size_t position) {
		// Read data blocks, first byte is block size, 0 is the terminator
		uint8_t				block_size = 0;
		while ( (block_size = buffer[position++]) != 0) {
			DataRef			data = std::make_shared<Data>();
			data->mData.reserve(block_size);
			data->mData.insert(data->mData.begin(), buffer.begin()+position, buffer.begin()+position+block_size);
			mSubBlocks.push_back(data);

			position += block_size;
		}
		return position;
	}
};
using BlockRef = std::shared_ptr<Block>;

class GraphicControlExtension : public Block {
public:
	enum class Disposal			{ kUnspecified, kDoNotDispose, kRestoreToBackgroundColor, kRestoreToPrevious };
	static const uint32_t		TRANSPARENT_COLOR_F = (1<<0);
	static const uint32_t		USER_INPUT_EXPECTED_F = (1<<1);

	GraphicControlExtension() { }

	uint32_t				mFlags = 0;
	Disposal				mDisposal = Disposal::kUnspecified;
	uint8_t					mTransparencyIndex = 0;
	double					mDelay = 0.0;

	bool					hasTransparentColor() const { return (mFlags&TRANSPARENT_COLOR_F) != 0; }

	// We are past the introducer and app bytes here
	size_t					read(const std::vector<char> &buffer, size_t position) {
		uint8_t				block_size = buffer[position++];
		if (block_size != 4) throw std::runtime_error("GraphicControlExtension has illegal Block Size");

		// fields
		const uint8_t		fields = buffer[position++];
		if ((fields&(1<<0)) != 0) mFlags |= TRANSPARENT_COLOR_F;
		if ((fields&(1<<1)) != 0) mFlags |= USER_INPUT_EXPECTED_F;
		const uint8_t		disposal = ((fields>>2)&0x7);
		if (disposal == 1) mDisposal = Disposal::kDoNotDispose;
		else if (disposal == 2) mDisposal = Disposal::kRestoreToBackgroundColor;
		else if (disposal == 3) mDisposal = Disposal::kRestoreToPrevious;

		// delay time
		const uint8_t		dt0 = buffer[position++],
							dt1 = buffer[position++];
		// xxx decode...
		if (dt0 != 0 || dt1 != 0) {
			int32_t			v = (static_cast<int32_t>(dt1) << 8) | static_cast<int32_t>(dt0);
			mDelay = static_cast<double>(v) / 100.0;
		}
		// transparent color index
		mTransparencyIndex = buffer[position++];

		// terminator
		const uint8_t		term = buffer[position++];
		if (term != 0) throw std::runtime_error("GraphicControlExtension missing block terminator");

		return position;
	}
};

class ImageData : public Block {
public:
	static const uint32_t	LOCAL_COLOR_TABLE_F = (1<<0);
	static const uint32_t	INTERLACE_F = (1<<1);
	static const uint32_t	SORT_F = (1<<2);

	ImageData() { }

	int32_t					mLeftPosition = 0,
							mTopPosition = 0,
							mWidth = 0,
							mHeight = 0;
	uint32_t				mFlags = 0;
	uint8_t					mSizeOfLocalColorTable = 0;
	ColorTable				mColorTable;

	// We are past the image separator byte here
	size_t					read(const std::vector<char> &buffer, size_t position, BlockReadArgs &bra) {
		const ColorTable*	ct = &bra.mGlobalColorTable;

		// Image descriptor
		mLeftPosition = read_2_byte_int(buffer, position);
		mTopPosition = read_2_byte_int(buffer, position);
		mWidth = read_2_byte_int(buffer, position);
		mHeight = read_2_byte_int(buffer, position);
		
		uint8_t		fields = buffer[position++];
		if ((fields&(1<<7)) != 0) mFlags |= LOCAL_COLOR_TABLE_F;
		if ((fields&(1<<6)) != 0) mFlags |= INTERLACE_F;
		if ((fields&(1<<5)) != 0) mFlags |= SORT_F;
		mSizeOfLocalColorTable = (fields&0x7);

		// Optional local color table
		if ((mFlags&LOCAL_COLOR_TABLE_F) != 0) {
			position = mColorTable.read(buffer, color_count(mSizeOfLocalColorTable), position);
			ct = &mColorTable;
		}

		// Image data
		uint8_t			lzw_code_size = buffer[position++],
						block_size = 0;
		bra.startLzwDecode(mLeftPosition, mTopPosition, mWidth, mHeight);
		gif::LzwReader&	decoder(bra.mDecoder);
		auto			flush_fn = [&bra, &ct](const std::vector<uint8_t> &data) { bra.addPixels(data, *ct); };
		decoder.begin(lzw_code_size, flush_fn);
		while ( (block_size = buffer[position++]) != 0) {
			decoder.decode(buffer.begin()+position, buffer.begin()+(position+block_size));
			position += block_size;
		}
		const double	delay = (bra.mGceRef ? bra.mGceRef->mDelay : 0.0);
		bra.mConstructor.addFrame(bra.mBitmap, delay);
		return position;
	}
};

class AppExtension : public Block {
public:
	AppExtension() { }

	// We are past the introducer and app bytes here
	size_t			read(const std::vector<char> &buffer, size_t position) {
		uint8_t		block_size = buffer[position++];
		if (block_size != 11) throw std::runtime_error("AppExtension has illegal Block Size");

		// identifier
		for (size_t k=0; k<8; ++k) ++position;

		// authentication
		for (size_t k=0; k<3; ++k) ++position;

		return readSubBlocks(buffer, position);
	}
};

class BlockList {
public:
	BlockList() { }

	size_t			read(const uint8_t byte1, const std::vector<char> &buffer, size_t position, BlockReadArgs &bra) {
		// Select between:
		//		Image Descriptor				- 0x2c (image)
		//		Graphic Control Extension		- 0x21 (extension), 0xf9 (graphic control)
		//		Application Extension			- 0x21 (extension), 0xff (application)
		//		Comment Extension				- 0x21 (extension), 0xfe (comment)
		//		Plain Text Extension			- 0x21 (extension), 0x01 (plain text)
		if (byte1 == 0x21) {
			uint8_t		byte2 = buffer[position++];
			// text
			if (byte2 == 0x01) {
				// XXX unimplemented, don't have any examples
				std::cout << "text block unimplemented" << std::endl;
				throw std::runtime_error("text block unimplemented");
			// commment
			} else if (byte2 == 0xfe) {
				// XXX unimplemented, don't have any examples
				std::cout << "comment block unimplemented" << std::endl;
				throw std::runtime_error("comment block unimplemented");
			// graphic control
			} else if (byte2 == 0xf9) {
				std::shared_ptr<GraphicControlExtension>	block = std::make_shared<GraphicControlExtension>();
				position = block->read(buffer, position);
				// Provide me to the next image block
				bra.mGceRef = block;
				mBlocks.push_back(block);
			// application
			} else if (byte2 == 0xff) {
				std::shared_ptr<AppExtension>				block = std::make_shared<AppExtension>();
				position = block->read(buffer, position);
				mBlocks.push_back(block);
			} else {
				throw std::runtime_error("Read block on invalid extension byte");
			}
		// Image
		} else if (byte1 == 0x2c) {
			std::shared_ptr<ImageData>						block = std::make_shared<ImageData>();
			position = block->read(buffer, position, bra);
			mBlocks.push_back(block);
			// Clear out my associated GCE
			bra.mGceRef.reset();
		} else {
			throw std::runtime_error("Read block on invalid introducer byte");
		}
		return position;
	}

	std::vector<BlockRef>	mBlocks;
};

/**
 * BlockReadArgs
 * Need to implement a function after the Graphic Control Extension block is defined.
 */
void BlockReadArgs::addPixels(const std::vector<uint8_t> &indexes, const ColorTable &t) {
	const bool				has_transparent = (mGceRef && mGceRef->hasTransparentColor());

	for (const auto& it : indexes) {
		const size_t		bi = static_cast<size_t>((mBitmapIndexY * mScreenWidth) + mBitmapIndexX);
		if (bi >= mBitmap.mPixels.size()) return;
		if (++mBitmapIndexX >= mRight) {
			mBitmapIndexX = mLeft;
			++mBitmapIndexY;
		}
		if (has_transparent && it == mGceRef->mTransparencyIndex) continue;

		if (it < t.mColors.size()) {
			mBitmap.mPixels[bi] = t.mColors[it];
		} else {
			// error
			mBitmap.mPixels[bi] = gif::ColorA8u(0, 0, 0, 0);
		}
	}
}

}

/**
 * @class gif::Reader
 */
Reader::Reader(std::string path)
		: mPath(path) {
}

bool Reader::read(gif::ListConstructor &constructor) {
	try {
		std::ifstream		input(mPath, std::ios::binary);
		std::vector<char>	buffer(	(std::istreambuf_iterator<char>(input)), 
									(std::istreambuf_iterator<char>()));
		input.close();

		Header				header;
		LogicalScreen		screen;
		ColorTable			globalColorTable;
		BlockList			blocks;

		size_t				pos = 0;

		// Header
		if (buffer.size() < 6) throw std::runtime_error("No header");
		pos = header.read(buffer, pos);
		if (!header.isGif()) throw std::runtime_error("Header signature is not GIF");
		if (header.mVersion == Version::kMissing) throw std::runtime_error("Header has no version");

		// Logical Screen
		pos = screen.read(buffer, pos);

		// Global color table
		if (screen.hasGlobalColorTable()) {
			pos = globalColorTable.read(buffer, color_count(screen.mSizeOfGlobalColorTable), pos);
		}

		BlockReadArgs		bra(screen.mScreenWidth, screen.mScreenHeight, globalColorTable, constructor);
		while (pos < buffer.size()) {
			const uint8_t	byte1 = buffer[pos++];
			if (byte1 == 0x3b) {
				// Trailer, success
				return true;
			} else {
				pos = blocks.read(byte1, buffer, pos, bra);
			}
		}
	} catch (std::exception const &ex) {
		std::cout << "Error in gif::Reader::read()=" << ex.what() << std::endl;
	}
	return false;
}

/**
 * @class gif::Writer
 */
Writer::Writer(std::string path)
		: mPath(path) {
}

Writer&	Writer::setTableMode(TableMode m) {
	mTableMode = m;
	return *this;
}

bool Writer::write() {
	try {
		std::ofstream		output(mPath, std::ios::out | std::ios::binary);
		Header				header(SIG, Version::k89a);
		LogicalScreen		screen;

		header.write(output);
	} catch (std::exception const &ex) {
		std::cout << "Error in gif::Writer::write()=" << ex.what() << std::endl;
	}
	return false;
}

} // namespace gif
