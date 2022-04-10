/*
 * DeviceDescriptor.cpp
 * Copyright (C) 2022 pulkomandy <pulkomandy@shredder>
 *
 * Distributed under terms of the MIT license.
 */

#include "DeviceDescriptor.h"



namespace mtp { namespace usb
{
	DeviceDescriptor::DeviceDescriptor(BUSBDevice* dev)
		:_dev(dev)
	{
	}

	DeviceDescriptor::~DeviceDescriptor()
	{
	}

	ConfigurationPtr DeviceDescriptor::GetConfiguration(int conf)
	{
		return std::make_shared<Configuration>(_dev->ConfigurationAt(conf));
	}

	ByteArray DeviceDescriptor::GetDescriptor() const
	{
		const usb_device_descriptor* descriptor = _dev->Descriptor();
		ByteArray out;
		out.reserve(sizeof(usb_device_descriptor));
		memcpy(out.data(), descriptor, sizeof(usb_device_descriptor));
		return out;
	}

	DevicePtr DeviceDescriptor::TryOpen(ContextPtr context)
	{
		if (_dev->InitCheck() != B_OK)
			return nullptr;
		return std::make_shared<Device>(context, _dev);
	}
}}
