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
#ifndef BULKPIPE_H
#define	BULKPIPE_H

#include <mtp/Token.h>
#include <mtp/ByteArray.h>
#include <mtp/ptp/IObjectStream.h>

namespace mtp { namespace usb
{
	class Device;
	DECLARE_PTR(Device);
	class Configuration;
	DECLARE_PTR(Configuration);
	class Interface;
	DECLARE_PTR(Interface);
	class Endpoint;
	DECLARE_PTR(Endpoint);
	class BulkPipe;
	DECLARE_PTR(BulkPipe);

	class BulkPipe
	{
		DevicePtr				_device;
		ConfigurationPtr		_conf;
		InterfacePtr			_interface;
		EndpointPtr				_in, _out, _interrupt;
		ITokenPtr				_claimToken;

	public:
		BulkPipe(DevicePtr device, ConfigurationPtr conf, InterfacePtr interface, EndpointPtr in, EndpointPtr out, EndpointPtr interrupt);
		~BulkPipe();

		ByteArray ReadInterrupt();

		void Read(const IObjectOutputStreamPtr &outputStream, int timeout = 10000);
		void Write(const IObjectInputStreamPtr &inputStream, int timeout = 10000);

		static BulkPipePtr Create(const usb::DevicePtr & device, const ConfigurationPtr & conf, const usb::InterfacePtr & owner);
	};

}}

#endif	/* BULKPIPE_H */
