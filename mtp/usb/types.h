#ifndef USB_TYPES_H
#define	USB_TYPES_H

namespace mtp { namespace usb
{

	enum struct EndpointType
	{
		Control = 0, Isochronous = 1, Bulk = 2, Interrupt = 3
	};

	enum struct EndpointDirection
	{
		In, Out, Both
	};

}}

#endif
