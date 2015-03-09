#ifndef DEVICE_H
#define	DEVICE_H

#include <mtp/types.h>
#include <libusb.h>

namespace mtp { namespace usb
{
	class Device : Noncopyable
	{
	private:
		libusb_device_handle *_handle;

	public:
		Device(libusb_device_handle * handle);
		~Device();

		libusb_device_handle * GetHandle() { return _handle; }

		void SetConfiguration(int idx);

		std::string GetString(int idx) const;
	};
	DECLARE_PTR(Device);
}}

#endif	/* DEVICE_H */

