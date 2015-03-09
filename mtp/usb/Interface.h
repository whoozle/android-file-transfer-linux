#ifndef INTERFACE_H
#define	INTERFACE_H

#include <mtp/types.h>

namespace mtp { namespace usb
{
	enum struct EndpointDirection {
		In, Out
	};
	class Endpoint
	{
		const libusb_endpoint_descriptor & _endpoint;

	public:
		Endpoint(const libusb_endpoint_descriptor & endpoint) : _endpoint(endpoint) { }

		u8 GetAddress() const
		{ return _endpoint.bEndpointAddress; }

		EndpointDirection GetDirection() const
		{
			u8 dir = GetAddress() & LIBUSB_ENDPOINT_DIR_MASK;
			if (dir == LIBUSB_ENDPOINT_IN)
				return EndpointDirection::In;
			else
				return EndpointDirection::Out;
		}
	};
	DECLARE_PTR(Endpoint);

	class Interface
	{
		const libusb_interface_descriptor &_interface;

	public:
		Interface(const libusb_interface_descriptor &interface): _interface(interface) { }

		u8 GetIndex() const
		{ return _interface.bInterfaceNumber; }

		int GetNameIndex() const
		{ return _interface.iInterface; }

		EndpointPtr GetEndpoint(int idx) const
		{ return std::make_shared<Endpoint>(_interface.endpoint[idx]); }

		int GetEndpointsCount() const
		{ return _interface.bNumEndpoints; }
	};
	DECLARE_PTR(Interface);

}}

#endif
