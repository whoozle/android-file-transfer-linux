/*
 * Context.cpp
 * Copyright (C) 2022 pulkomandy <pulkomandy@shredder>
 *
 * Distributed under terms of the MIT license.
 */

#include "Context.h"

namespace mtp { namespace usb
{
	Context::Context(int debugLevel)
		: BUSBRoster()
	{
		Start();
	}

	Context::~Context() {
		Stop();
	}

	status_t Context::DeviceAdded(BUSBDevice* device) {
		_devices.emplace_back(std::make_shared<DeviceDescriptor>(device));

		return B_OK;
	}

	void Context::DeviceRemoved(BUSBDevice* device) {
		// TODO _devices.remove(device);
	}

	
} };
