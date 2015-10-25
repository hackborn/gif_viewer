#ifndef GIFIO_GIFFILE_H_
#define GIFIO_GIFFILE_H_

#include <string>
#include "gif_list.h"

namespace gif {

/**
 * @class gif::Reader
 * @brief Load a GIF file into a sequence of images.
 * GIF spec: http://www.w3.org/Graphics/GIF/spec-gif89a.txt
 */
class Reader {
public:
	Reader(std::string path);

	// Given a file path, load all frames of data to output.
	// This peforms no validation that the file is valid.
	// Answer false on error.
	bool				read(gif::ListConstructor &output);

private:
	std::string			mPath;
};

/**
 * @class gif::Writer
 * @brief Write image frames into a GIF file.
 * GIF spec: http://www.w3.org/Graphics/GIF/spec-gif89a.txt
 */
class Writer {
public:
	Writer(std::string path);

	// Decide how to create color table(s).
	// * kGlobalTableFromFirst -- create a global color table based solely on
	// the first frame of data. This is (potentially) the most memory-efficient
	// mode, as each frame can be written and then discarded.
	// * kGlobalTableFromAll -- create a global color table based on all frames.
	// This will likely result in the best balance of final output quality and
	// file size, but at an up-front memory cost of requiring to load all frames
	// of data.
	// * kLocalTable -- a local color table is created for each frame.
	enum class TableMode {	kGlobalTableFromFirst,
							kGlobalTableFromAll,
							kLocalTable };
	Writer&				setTableMode(TableMode);

	// Once all settings have been applied, start adding frames of data.
	bool				write();

private:
	std::string			mPath;
	bool				mFirstFrame = true;
	TableMode			mTableMode = TableMode::kGlobalTableFromFirst;
};

} // namespace gif

#endif
