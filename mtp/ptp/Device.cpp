/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2020  Vladimir Menshakov

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

	u8 Device::GetInterfaceStringIndex(usb::DeviceDescriptorPtr desc, u8 number)
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

	DevicePtr Device::Open(usb::ContextPtr ctx, usb::DeviceDescriptorPtr desc, bool claimInterface, bool resetDevice)
	{
		debug("probing device ", hex(desc->GetVendorId(), 4), ":", hex(desc->GetProductId(), 4));
		usb::DevicePtr device = desc->TryOpen(ctx);

		if (resetDevice && device) {
			device->Reset();
			device.reset();
			device = desc->TryOpen(ctx);
		}

		if (!device) {
			debug("descriptor->TryOpen() failed");
			return nullptr;
		}

		int confs = desc->GetConfigurationsCount();
		debug("configurations: ", confs);

		for(int i = 0; i < confs; ++i)
		{
			usb::ConfigurationPtr conf = desc->GetConfiguration(i);
			int interfaces = conf->GetInterfaceCount();
			if (interfaces == 0) {
				debug("device not configured (no interfaces), setting configuration now...");
				int index = conf->GetIndex();
				conf.reset();
				device->SetConfiguration(index);
				debug("device configured, retrieving configuration descriptor again...");
				conf = desc->GetConfiguration(i);
				interfaces = conf->GetInterfaceCount();
			}
			debug("interfaces: ", interfaces);
			for(int j = 0; j < interfaces; ++j)
			{
				usb::InterfacePtr iface = conf->GetInterface(device, conf, j, 0);
				debug("Device usb interface: ", i, ':', j, ", index: ", iface->GetIndex(), ", endpoints: ", iface->GetEndpointsCount());

#ifdef USB_BACKEND_LIBUSB
				std::string name = iface->GetName();
#else
				ByteArray data = usb::DeviceRequest(device).GetDescriptor(usb::DescriptorType::String, 0, 0);
				HexDump("languages", data);
				if (data.size() < 4 || data[1] != (u8)usb::DescriptorType::String)
					continue;

				u16 langId = data[2] | ((u16)data[3] << 8);

				std::string name;

				try
				{
					data = usb::DeviceRequest(device).GetDescriptor(usb::DescriptorType::String, static_cast<u8>(usb::DeviceRequest::Request::GetOSStringDescriptor), langId);
					HexDump("OSStringDescriptor", data);
					if (data.size() < 0x12 || data.at(2) != 'M' || data.at(4) != 'S' || data.at(6) != 'F' || data.at(8) != 'T')
						throw std::runtime_error("invalid OSString descriptor");
					u8 command = data.at(0x10);
					debug("vendor code: 0x", hex(command, 2));

					//getting extended OS compat descriptor
					data.resize(255);
					device->ReadControl(static_cast<u8>(usb::RequestType::DeviceToHost | usb::RequestType::Vendor | usb::RequestType::Device),
						command, 0, 4, data, usb::BaseRequest::DefaultTimeout);
					HexDump("extended compat id os feature desctriptor", data);
					if (data.at(0x12) == 'M' && data.at(0x13) == 'T' && data.at(0x14) == 'P')
						name = "MTP";
				}
				catch (const std::exception & ex)
				{ debug("winusb handshake failed: ", ex.what()); }

				if (name.find("MTP") == name.npos)
				{
					auto interfaceStringIndex = GetInterfaceStringIndex(desc, j);
					data = usb::DeviceRequest(device).GetDescriptor(usb::DescriptorType::String, interfaceStringIndex, langId);
					HexDump("interface name", data);
					if (data.size() < 4 || data[1] != (u8)usb::DescriptorType::String)
						continue;

					u8 len = data[0];
					InputStream stream(data, 2);
					name = stream.ReadString((len - 2) / 2);
				}
#endif
				//device->SetConfiguration(i);
				if (name.find("MTP") != name.npos)
				{
					usb::InterfaceTokenPtr token = claimInterface? device->ClaimInterface(iface): nullptr;
					usb::BulkPipePtr pipe = usb::BulkPipe::Create(device, conf, iface, token);
					return std::make_shared<Device>(pipe);
				} else
					debug("skipping interface with name ", name);

				if (iface->GetClass() == 6 && iface->GetSubclass() == 1)
				{
					usb::InterfaceTokenPtr token = claimInterface? device->ClaimInterface(iface): nullptr;
					usb::BulkPipePtr pipe = usb::BulkPipe::Create(device, conf, iface, token);
					return std::make_shared<Device>(pipe);
				}
			}
		}
		return nullptr;
	}

	DevicePtr Device::FindFirst(const std::string & filter, bool claimInterface, bool resetDevice)
	{
		usb::ContextPtr ctx(new usb::Context);
		return FindFirst(ctx, filter, claimInterface, resetDevice);
	}

	DevicePtr Device::FindFirst(usb::ContextPtr ctx, const std::string & filter, bool claimInterface, bool resetDevice)
	{
		int vendor, product;
		bool useUsbVendorProduct = true;
		if (sscanf(filter.c_str(), "%x:%x", &vendor, &product) != 2)
		{
			vendor = product = -1;
			useUsbVendorProduct = false;
		}

		for (usb::DeviceDescriptorPtr desc : ctx->GetDevices())
		try
		{
			if (vendor >= 0 && product >= 0)
			{
				if (desc->GetVendorId() != vendor || desc->GetProductId() != product)
					continue;
			}

			auto device = Open(ctx, desc, claimInterface, resetDevice);
			if (device && (useUsbVendorProduct || device->Matches(filter)))
				return device;
		}
		catch(const std::exception &ex)
		{ error("Device::Find failed:", ex.what()); }

		return nullptr;
	}

	msg::DeviceInfo Device::GetInfo()
	{ return Session::GetDeviceInfo(_packeter, /*transactionId=*/0); }

	bool Device::Matches(const std::string & filter)
	{
		if (filter.empty())
			return true;

		auto di = GetInfo();
		return di.Matches(filter);
	}

}
