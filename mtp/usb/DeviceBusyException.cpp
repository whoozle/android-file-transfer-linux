#include <mtp/usb/DeviceBusyException.h>

namespace mtp { namespace usb
{

	DeviceBusyException::DeviceBusyException(int fd, const std::string &msg):
		std::runtime_error(msg)
	{ }

}}