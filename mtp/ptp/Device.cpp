/*
 * Android File Transfer for Linux: MTP client for android devices
 * Copyright (C) 2015  Vladimir Menshakov

 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include <mtp/ptp/Device.h>
#include <mtp/ptp/Response.h>
#include <mtp/ptp/Container.h>
#include <mtp/ptp/Messages.h>
#include <usb/Context.h>
#include <mtp/ptp/OperationRequest.h>
#include <stdexcept>


namespace mtp
{

	Device::Device(usb::BulkPipePtr pipe): _packeter(pipe)
	{ }

	SessionPtr Device::OpenSession(u32 sessionId)
	{
		OperationRequest req(OperationCode::OpenSession, 0, sessionId);
		Container container(req);
		_packeter.Write(container.Data);
		ByteArray data, response;
		ResponseType code;
		_packeter.Read(0, data, code, response);
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

	DevicePtr Device::Find()
	{
		using namespace mtp;
		usb::ContextPtr ctx(new usb::Context);

		for (usb::DeviceDescriptorPtr desc : ctx->GetDevices())
		try
		{
			usb::DevicePtr device = desc->TryOpen(ctx);
			if (!device)
				continue;
			int confs = desc->GetConfigurationsCount();
			//fprintf(stderr, "configurations: %d\n", confs);

			for(int i = 0; i < confs; ++i)
			{
				usb::ConfigurationPtr conf = desc->GetConfiguration(i);
				int interfaces = conf->GetInterfaceCount();
				//fprintf(stderr, "interfaces: %d\n", interfaces);
				for(int j = 0; j < interfaces; ++j)
				{
					usb::InterfacePtr iface = conf->GetInterface(device, conf, j, 0);
					usb::InterfaceTokenPtr token = device->ClaimInterface(iface);
					fprintf(stderr, "%d:%d index %u, eps %u\n", i, j, iface->GetIndex(), iface->GetEndpointsCount());

					static const u16 DT_STRING = 3;

					ByteArray data(255);
					device->ReadControl(0x80, 0x06, (DT_STRING << 8) | 0, 0, data, 1000);
					HexDump("languages", data);
					if (data.size() < 4 || data[1] != DT_STRING)
						continue;

					int interfaceStringIndex = GetInterfaceStringIndex(desc, j);
					u16 langId = data[2] | ((u16)data[3] << 8);
					data.resize(255);

					device->ReadControl(0x80, 0x06, (DT_STRING << 8) | interfaceStringIndex, langId, data, 1000);
					HexDump("interface name", data);
					if (data.size() < 4 || data[1] != DT_STRING)
						continue;

					u8 len = data[0];
					InputStream stream(data, 2);
					std::string name = stream.ReadString((len - 2) / 2);
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
		}
		catch(const std::exception &ex)
		{ fprintf(stderr, "%s\n", ex.what()); }

		return nullptr;
	}

}
