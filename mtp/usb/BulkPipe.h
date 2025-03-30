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

#ifndef AFTL_MTP_USB_BULKPIPE_H
#define AFTL_MTP_USB_BULKPIPE_H

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

	class BulkPipe //! USB BulkPipe, incapsulate three (in, out, interrupt) endpoints, allowing easier data transfer
	{
		std::mutex				_mutex;
		DevicePtr				_device;
		ConfigurationPtr		_conf;
		InterfacePtr			_interface;
		EndpointPtr				_in, _out, _interrupt;
		ITokenPtr				_claimToken;
		ICancellableStreamPtr	_currentStream;

	private:
		void SetCurrentStream(const ICancellableStreamPtr &stream);
		ICancellableStreamPtr GetCurrentStream();

		class CurrentStreamSetter;

	public:
		BulkPipe(DevicePtr device, ConfigurationPtr conf, InterfacePtr interface, EndpointPtr in, EndpointPtr out, EndpointPtr interrupt, ITokenPtr claimToken, bool clearHalt = false);
		~BulkPipe();

		DevicePtr GetDevice() const;
		InterfacePtr GetInterface() const;

		ByteArray ReadInterrupt(int timeout);

		void Read(const IObjectOutputStreamPtr &outputStream, int timeout = 10000);
		void Write(const IObjectInputStreamPtr &inputStream, int timeout = 10000);
		void Cancel();

		static BulkPipePtr Create(const usb::DevicePtr & device, const ConfigurationPtr & conf, const usb::InterfacePtr & owner, ITokenPtr claimToken);
	};

}}

#endif	/* BULKPIPE_H */
