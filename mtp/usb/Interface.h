#ifndef INTERFACE_H
#define	INTERFACE_H

#include <mtp/types.h>

namespace mtp { namespace usb
{

	class Interface
	{
		const libusb_interface_descriptor &_interface;

	public:
		Interface(const libusb_interface_descriptor &interface): _interface(interface) { }

		int GetEndpointsCount() const
		{ return _interface.bNumEndpoints; }
	};
	DECLARE_PTR(Interface);

}}

#endif
