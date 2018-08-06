/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2018  Vladimir Menshakov

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License,
    or (at your option) any later version.

    This library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include <mtp/ptp/Device.h>
#include <mtp/ptp/Response.h>
#include <mtp/ptp/Container.h>
#include <mtp/ptp/Messages.h>
#include <mtp/log.h>
#include <usb/Context.h>
#include <usb/Device.h>
#include <usb/Interface.h>
#include <mtp/ptp/OperationRequest.h>
#include <mtp/usb/Request.h>
#include <stdexcept>


namespace mtp
{

	Device::Device(usb::BulkPipePtr pipe): _packeter(pipe)
	{ }

	SessionPtr Device::OpenSession(u32 sessionId, int timeout)
	{
		OperationRequest req(OperationCode::OpenSession, 0, sessionId);
		Container container(req);
		_packeter.Write(container.Data, timeout);
		ByteArray data, response;
		ResponseType code;
		_packeter.Read(0, data, code, response, timeout);
		//HexDump("payload", data);

		return std::make_shared<Session>(_packeter.GetPipe(), sessionId);
	}

	int Device::GetInterfaceStringIndex(usb::DeviceDescriptorPtr desc, u8 number)
	{
		static const u16 DT_INTERFACE = 4;

		ByteArray descData = desc->GetDescriptor();
		HexDump("descriptor", descData);
		size_t offset = 0;
		while(offset < descData.size())
		{
			u8 len = descData.at(offset + 0);
			u8 type = descData.at(offset + 1);
			if (len < 2)
				throw std::runtime_error("invalid descriptor length");

			if (type == DT_INTERFACE && len >= 9 && descData.at(offset + 2) == number)
				return descData.at(offset + 8);

			offset += len;
		}
		throw std::runtime_error("no interface descriptor found");
	}

	DevicePtr Device::Open(usb::ContextPtr ctx, usb::DeviceDescriptorPtr desc, bool claimInterface)
	{
		debug("probing device ", hex(desc->GetVendorId(), 4), ":", hex(desc->GetProductId(), 4));
		usb::DevicePtr device = desc->TryOpen(ctx);
		if (!device)
			return nullptr;
		int confs = desc->GetConfigurationsCount();
		//debug("configurations: ", confs);

		for(int i = 0; i < confs; ++i)
		{
			usb::ConfigurationPtr conf = desc->GetConfiguration(i);
			int interfaces = conf->GetInterfaceCount();
			//debug("interfaces: ", interfaces);
			for(int j = 0; j < interfaces; ++j)
			{
				usb::InterfacePtr iface = conf->GetInterface(device, conf, j, 0);
				usb::InterfaceTokenPtr token = claimInterface? device->ClaimInterface(iface): nullptr;
				debug("Device usb interface: ", i, ':', j, ", index: ", iface->GetIndex(), ", enpoints: ", iface->GetEndpointsCount());

#ifdef USB_BACKEND_LIBUSB
				std::string name = iface->GetName();
#else
				ByteArray data = usb::DeviceRequest(device).GetDescriptor(usb::DescriptorType::String, 0, 0);
				HexDump("languages", data);
				if (data.size() < 4 || data[1] != (u8)usb::DescriptorType::String)
					continue;

				int interfaceStringIndex = GetInterfaceStringIndex(desc, j);
				u16 langId = data[2] | ((u16)data[3] << 8);
				data = usb::DeviceRequest(device).GetDescriptor(usb::DescriptorType::String, interfaceStringIndex, langId);
				HexDump("interface name", data);
				if (data.size() < 4 || data[1] != (u8)usb::DescriptorType::String)
					continue;

				u8 len = data[0];
				InputStream stream(data, 2);
				std::string name = stream.ReadString((len - 2) / 2);
#endif
				if (name == "MTP")
				{
					//device->SetConfiguration(configuration->GetIndex());
					usb::BulkPipePtr pipe = usb::BulkPipe::Create(device, conf, iface, token);
					return std::make_shared<Device>(pipe);
				}
				if (iface->GetClass() == 6 && iface->GetSubclass() == 1)
				{
					usb::BulkPipePtr pipe = usb::BulkPipe::Create(device, conf, iface, token);
					return std::make_shared<Device>(pipe);
				}
			}
		}
		return nullptr;
	}

	std::list<DevicePtr> Device::Find(bool claimInterface)
	{
		std::list<DevicePtr> foundDevices;

		usb::ContextPtr ctx(new usb::Context);

		for (usb::DeviceDescriptorPtr desc : ctx->GetDevices())
		try
		{
			auto device = Open(ctx, desc, claimInterface);
			if (device)
				foundDevices.push_back(device);
		}
		catch(const std::exception &ex)
		{ error("Device::Find failed:", ex.what()); }

		return foundDevices;
	}

}
