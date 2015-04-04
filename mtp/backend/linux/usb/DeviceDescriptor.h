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
#ifndef USB_DEVICE_H
#define USB_DEVICE_H

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
		~Configuration() { }

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
		std::string						_path;
		u16								_vendor, _product;
		std::map<int, ConfigurationPtr>	_configurations;

	public:
		DeviceDescriptor(const std::string &path);
		~DeviceDescriptor();

		u16 GetVendorId() const
		{ return _vendor; }
		u16 GetProductId() const
		{ return _product; }

		DevicePtr Open(ContextPtr context);
		DevicePtr TryOpen(ContextPtr context);

		int GetConfigurationsCount() const
		{ return _configurations.size(); }

		ConfigurationPtr GetConfiguration(int conf)
		{ return _configurations.at(conf); }

		void AddInterface(int confIndex, int interface, const std::string &path)
		{
			ConfigurationPtr &conf = _configurations[confIndex];
			if (!conf)
				conf = std::make_shared<Configuration>();
			conf->AddInterface(interface, path);
		}
	};
	DECLARE_PTR(DeviceDescriptor);

}}

#endif

