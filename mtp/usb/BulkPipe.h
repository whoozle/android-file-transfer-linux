/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015  Vladimir Menshakov

    Android File Transfer For Linux is free software: you can redistribute
    it and/or modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    Android File Transfer For Linux is distributed in the hope that it will
    be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Android File Transfer For Linux.
    If not, see <http://www.gnu.org/licenses/>.
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
		BulkPipe(DevicePtr device, ConfigurationPtr conf, InterfacePtr interface, EndpointPtr in, EndpointPtr out, EndpointPtr interrupt, ITokenPtr claimToken);
		~BulkPipe();

		DevicePtr GetDevice() const;

		ByteArray ReadInterrupt();

		void Read(const IObjectOutputStreamPtr &outputStream, int timeout = 10000);
		void Write(const IObjectInputStreamPtr &inputStream, int timeout = 10000);
		void Cancel();

		static BulkPipePtr Create(const usb::DevicePtr & device, const ConfigurationPtr & conf, const usb::InterfacePtr & owner, ITokenPtr claimToken);
	};

}}

#endif	/* BULKPIPE_H */
