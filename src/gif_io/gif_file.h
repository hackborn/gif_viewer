#ifndef GIFIO_GIFFILE_H_
#define GIFIO_GIFFILE_H_

#include <string>
#include "gif_list.h"

namespace gif {

/**
 * @class gif::File
 * @brief Load a GIF file into a sequence of images.
 * GIF spec: http://www.w3.org/Graphics/GIF/spec-gif89a.txt
 */
class File {
public:
	File();

	// Given a file path, load all frames of data to output.
	// This peforms no validation that the file is valid.
	// Answer false on error.
	bool				load(const std::string&, gif::ListConstructor &output);

	// Given a file path, save all frames of data.
	// Answer false on error.
	bool				save(const std::string&);
};

} // namespace gif

#endif
