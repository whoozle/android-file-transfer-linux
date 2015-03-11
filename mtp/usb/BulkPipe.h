#ifndef BULKPIPE_H
#define	BULKPIPE_H

#include <mtp/usb/Device.h>
#include <mtp/usb/Interface.h>
#include <mtp/ByteArray.h>

namespace mtp { namespace usb
{
	class BulkPipe;
	DECLARE_PTR(BulkPipe);

	class BulkPipe
	{
		DevicePtr		_device;
		InterfacePtr	_interface;
		EndpointPtr		_in, _out, _interrupt;

	public:
		BulkPipe(DevicePtr device, InterfacePtr interface, EndpointPtr in, EndpointPtr out, EndpointPtr interrupt);

		ByteArray Read(int timeout = 3000);
		void Write(const ByteArray &data, int timeout = 3000);

		static BulkPipePtr Create(usb::DevicePtr device, usb::InterfacePtr owner);
	};

}}

#endif	/* BULKPIPE_H */
