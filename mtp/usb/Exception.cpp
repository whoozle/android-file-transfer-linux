#include <mtp/usb/Exception.h>
#include <libusb.h>

namespace mtp { namespace usb
{
	Exception::Exception(int returnCode) throw() : std::runtime_error(GetErrorMessage(returnCode))
	{

	}

	std::string Exception::GetErrorMessage(int returnCode)
	{
		return libusb_error_name(returnCode);
	}

}}