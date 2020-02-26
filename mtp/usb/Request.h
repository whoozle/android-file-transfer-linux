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

#ifndef AFTL_MTP_USB_REQUEST_H
#define AFTL_MTP_USB_REQUEST_H

#include <mtp/types.h>
#include <mtp/ByteArray.h>

namespace mtp { namespace usb
{
	class Device;
	DECLARE_PTR(Device);

	enum struct RequestType : u8
	{
		HostToDevice		= 0x00,
		DeviceToHost		= 0x80,

		Standard			= 0x00,
		Class				= 0x20,
		Vendor				= 0x40,

		Device				= 0x00,
		Interface			= 0x01,
		Endpoint			= 0x02,
		Other				= 0x03
	};

	constexpr RequestType operator | (RequestType r1, RequestType r2)
	{ return static_cast<RequestType>(static_cast<u8>(r1) | static_cast<u8>(r2)); };

	class BaseRequest //! Base USB Standard Request
	{
	public:
		static const int DefaultTimeout = 1000;

		BaseRequest(const DevicePtr & device, int timeout = DefaultTimeout);
		~BaseRequest();

	protected:
		DevicePtr	_device;
		int			_timeout;
	};

	enum struct DescriptorType : u8
	{
		Device						= 1,
		Configuration				= 2,
		String						= 3,
		Interface					= 4,
		Endpoint					= 5,
		DeviceQualifier				= 6,
		OtherSpeedConfiguration		= 7,
		InterfacePower				= 8,
		OnTheGo						= 9
	};

	struct DeviceRequest : BaseRequest //! USB Standard Request for Device
	{
		using BaseRequest::BaseRequest;

		enum struct Type : u8
		{
			DeviceOut	= static_cast<u8>(RequestType::HostToDevice | RequestType::Standard | RequestType::Device),
			DeviceIn	= static_cast<u8>(RequestType::DeviceToHost | RequestType::Standard | RequestType::Device)
		};

		enum struct Request : u8
		{
			GetStatus				= 0,
			ClearFeature			= 1,
			SetFeature				= 3,
			SetAddress				= 5,
			GetDescriptor			= 6,
			SetDescriptor			= 7,
			GetConfiguration		= 8,
			SetConfiguration		= 9,
			GetOSStringDescriptor  	= 0xee
		};

		u16 GetStatus();

		void ClearFeature(u16 feature);
		void SetFeature(u16 feature);

		void SetAddress(u16 address);

		ByteArray GetDescriptor(DescriptorType type, u8 index, u16 lang = 0);
		void SetDescriptor(DescriptorType type, u8 index, u16 lang, const ByteArray &data);
		u8 GetConfiguration();
		void SetConfiguration(u8 index);
	};

	class InterfaceRequest : public BaseRequest //! USB Standard Request for Interface
	{
		u16 _interface;
	public:

		enum struct Type : u8
		{
			InterfaceOut	= static_cast<u8>(RequestType::HostToDevice | RequestType::Standard | RequestType::Interface),
			InterfaceIn		= static_cast<u8>(RequestType::DeviceToHost | RequestType::Standard | RequestType::Interface)
		};

		enum struct Request : u8
		{
			GetStatus			= 0,
			ClearFeature		= 1,
			SetFeature			= 3,
			GetInterface		= 10,
			SetInterface		= 17
		};

		InterfaceRequest(const DevicePtr & device, u16 interface, int timeout = DefaultTimeout);

		u16 GetStatus();

		void ClearFeature(u16 feature);
		void SetFeature(u16 feature);

		u8 GetInterface();
		void SetInterface(u8 alt);
	};

	class EndpointRequest : public BaseRequest //! USB Standard Request for Endpoint
	{
		u16 _endpoint;

	public:
		enum struct Type : u8
		{
			EnpointOut		= static_cast<u8>(RequestType::HostToDevice | RequestType::Standard | RequestType::Endpoint),
			EnpointIn		= static_cast<u8>(RequestType::DeviceToHost | RequestType::Standard | RequestType::Endpoint)
		};

		enum struct Request : u8
		{
			GetStatus			= 0,
			ClearFeature		= 1,
			SetFeature			= 3,
			SynchFrame			= 18
		};

		EndpointRequest(const DevicePtr & device, u16 endpoint, int timeout = DefaultTimeout);

		u16 GetStatus();

		void ClearFeature(u16 feature);
		void SetFeature(u16 feature);

		void SynchFrame(u16 frameIndex);
	};

}}

#endif
