#ifndef USB_CONTEXT_H
#define USB_CONTEXT_H

#include <libusb.h>
#include <mtp/types.h>
#include <mtp/usb/DeviceDescriptor.h>
#include <vector>

namespace mtp { namespace usb
{

	class Context
	{
	private:
		libusb_context *		_ctx;
		std::vector<DeviceDescriptorPtr>	_devices;

	public:
		Context();
		~Context();
	};
	DECLARE_PTR(Context);

}}

#endif

