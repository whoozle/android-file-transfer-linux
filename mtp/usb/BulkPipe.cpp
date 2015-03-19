#include <mtp/usb/BulkPipe.h>
#include <mtp/usb/call.h>

namespace mtp { namespace usb
{
	BulkPipe::BulkPipe(DevicePtr device, InterfacePtr interface, EndpointPtr in, EndpointPtr out, EndpointPtr interrupt):
		_device(device), _interface(interface), _in(in), _out(out), _interrupt(interrupt)
	{
		//libusb_clear_halt(device->GetHandle(), in->GetAddress());
		//libusb_clear_halt(device->GetHandle(), out->GetAddress());
		//libusb_clear_halt(device->GetHandle(), interrupt->GetAddress());
	}

	ByteArray BulkPipe::ReadInterrupt()
	{
		ByteArray data(_interrupt->GetMaxPacketSize());
		int tr = 0;
		int r = libusb_interrupt_transfer(_device->GetHandle(), _interrupt->GetAddress(), data.data(), data.size(), &tr, 1);
		if (r == 0)
			data.resize(tr);
		else
			data.clear();
		return data;
	}

	ByteArray BulkPipe::Read(int timeout)
	{
		ByteArray data(_in->GetMaxPacketSize());
		int tr = 0;
		USB_CALL(libusb_bulk_transfer(_device->GetHandle(), _in->GetAddress(), data.data(), data.size(), &tr, timeout));
		data.resize(tr);
		return data;
	}

	void BulkPipe::Write(const ByteArray &data, int timeout)
	{
		int tr = 0;
		USB_CALL(libusb_bulk_transfer(_device->GetHandle(), _out->GetAddress(), const_cast<u8 *>(data.data()), data.size(), &tr, timeout));
		if (tr != (int)data.size())
			throw std::runtime_error("short write");
	}

	BulkPipePtr BulkPipe::Create(usb::DevicePtr device, usb::InterfacePtr interface)
	{
		int epn = interface->GetEndpointsCount();

		usb::EndpointPtr out, in, interrupt;
		//printf("endpoints: %d\n", epn);
		for(int i = 0; i < epn; ++i)
		{
			usb::EndpointPtr ep = interface->GetEndpoint(i);
			//printf("endpoint: %d: %02x\n", i, ep->GetAddress());
			//check for bulk here
			if (ep->GetDirection() == usb::EndpointDirection::Out)
			{
				if (ep->GetType() == usb::EndpointType::Bulk)
				{
					//printf("OUT\n");
					out = ep;
				}
			}
			else
			{
				if (ep->GetType() == usb::EndpointType::Bulk)
				{
					//printf("IN\n");
					in = ep;
				}
				else
				{
					//printf("INTERRUPT\n");
					interrupt = ep;
				}
			}
		}
		if (!in || !out || !interrupt)
			throw std::runtime_error("invalid endpoint");

		return std::make_shared<BulkPipe>(device, interface, in, out, interrupt);
	}

}}
