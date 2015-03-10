#ifndef USB_CONTEXT_H
#define USB_CONTEXT_H

#include <libusb.h>
#include <mtp/types.h>
#include <mtp/usb/DeviceDescriptor.h>
#include <vector>

namespace mtp { namespace usb
{

	class Context : Noncopyable
	{
	private:
		libusb_context *		_ctx;

	public:
		typedef std::vector<DeviceDescriptorPtr> Devices;

	private:
		Devices					_devices;

	public:
		Context();
		~Context();

		void Wait();

		const Devices & GetDevices() const
		{ return _devices; }
	};
	DECLARE_PTR(Context);

}}

#endif

