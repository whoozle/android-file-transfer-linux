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

#ifndef AFTL_MTP_BACKEND_LINUX_USB_DEVICEDESCRIPTOR_H
#define AFTL_MTP_BACKEND_LINUX_USB_DEVICEDESCRIPTOR_H

#include <mtp/types.h>
#include <usb/Device.h>
#include <usb/Interface.h>
#include <map>

namespace mtp { namespace usb
{
	class Configuration : Noncopyable
	{
		std::map<int, InterfacePtr> _interfaces;
	public:
		Configuration() { }

		int GetIndex() const
		{ return 0; }

		int GetInterfaceCount() const
		{ return _interfaces.size(); }

		int GetInterfaceAltSettingsCount(int idx) const
		{ return 1; }

		InterfacePtr GetInterface(DevicePtr device, ConfigurationPtr config, int idx, int settings) const
		{ return _interfaces.at(idx); }

		void AddInterface(int index, const std::string &path)
		{
			_interfaces[index] = std::make_shared<Interface>(index, path);
		}
	};

	class DeviceDescriptor
	{
		int								_busId;
		std::string						_path;
		u16								_vendor, _product;
		int								_deviceNumber;
		int								_configurationValue;
		std::map<int, ConfigurationPtr>	_configurationMap;
		std::vector<ConfigurationPtr>	_configuration;
		EndpointPtr						_controlEp;
		ByteArray						_descriptor;

	public:
		DeviceDescriptor(int busId, const std::string &path);
		~DeviceDescriptor();

		u16 GetVendorId() const
		{ return _vendor; }
		u16 GetProductId() const
		{ return _product; }

		DevicePtr Open(ContextPtr context);
		DevicePtr TryOpen(ContextPtr context);

		int GetConfigurationsCount() const
		{ return _configuration.size(); }

		ConfigurationPtr GetConfiguration(int conf)
		{ return _configuration.at(conf); }

		void AddInterface(int confIndex, int interface, const std::string &path)
		{
			ConfigurationPtr &conf = _configurationMap[confIndex];
			if (!conf)
			{
				conf = std::make_shared<Configuration>();
				_configuration.push_back(conf);
			}
			conf->AddInterface(interface, path);
		}

		ByteArray GetDescriptor() const
		{ return _descriptor; }
	};
	DECLARE_PTR(DeviceDescriptor);

}}

#endif

