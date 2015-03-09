#ifndef INTERFACE_H
#define	INTERFACE_H

#include <mtp/types.h>

namespace mtp { namespace usb
{

	class Interface
	{
		const libusb_interface &_interface;

	public:
		Interface(const libusb_interface &interface): _interface(interface) { }

	};
	DECLARE_PTR(Interface);

}}

#endif
