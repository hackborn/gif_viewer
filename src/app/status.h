#ifndef APP_APP_STATUS_H_
#define APP_APP_STATUS_H_

#include <string>
#include <cinder/Thread.h>

namespace cs {

/**
 * @class cs::Status
 * @brief Store the current app status.
 */
class Status {
public:
	enum class Type			{ kOneShot, kStart, kEnd };
	
	Status(Type t, std::string m) : mType(t), mMessage(m) { }
	Status(Type t, uint32_t id, std::string m) : mType(t), mId(id), mMessage(m) { }

	Type					mType = Type::kOneShot;
	uint32_t				mId = 0;
	std::string				mMessage;
};

} // namespace cs

#endif