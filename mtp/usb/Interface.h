#ifndef INTERFACE_H
#define	INTERFACE_H

#include <mtp/types.h>

namespace mtp { namespace usb
{
	class Endpoint
	{
		const libusb_endpoint_descriptor & _endpoint;

	public:
		Endpoint(const libusb_endpoint_descriptor & endpoint) : _endpoint(endpoint) { }

		u8 GetAddress() const
		{ return _endpoint.bEndpointAddress; }
	};
	DECLARE_PTR(Endpoint);

	class Interface
	{
		const libusb_interface_descriptor &_interface;

	public:
		Interface(const libusb_interface_descriptor &interface): _interface(interface) { }

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
