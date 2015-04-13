#ifndef USB_EXCEPTION_H
#define USB_EXCEPTION_H

#include <stdexcept>

namespace mtp { namespace usb
{

	struct TimeoutException : public std::runtime_error
	{
		TimeoutException(const std::string &msg): std::runtime_error(msg) { }
	};

}}

#endif
