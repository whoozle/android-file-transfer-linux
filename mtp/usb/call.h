#ifndef USB_USB_H
#define USB_USB_H

#include <mtp/usb/Exception.h>

#include <libusb.h>

#define USB_CALL(...) do { int r = (__VA_ARGS__); if (r != 0) throw mtp::usb::Exception(r) ; } while(false)

#endif

