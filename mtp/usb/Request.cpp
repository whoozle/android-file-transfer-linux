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

#include <mtp/usb/Request.h>
#include <usb/Device.h>

namespace mtp { namespace usb
{

	BaseRequest::BaseRequest(const DevicePtr & device, int timeout): _device(device), _timeout(timeout)
	{ }

	BaseRequest::~BaseRequest()
	{ }

	u16 DeviceRequest::GetStatus()
	{
		ByteArray data(2);
		_device->ReadControl((u8)Type::DeviceIn, (u8)Request::GetStatus, 0, 0, data, _timeout);
		return data[0] | ((u16)data[1] << 8);
	}

	void DeviceRequest::ClearFeature(u16 feature)
	{ _device->WriteControl((u8)Type::DeviceOut, (u8)Request::ClearFeature, feature, 0, ByteArray(), _timeout); }

	void DeviceRequest::SetFeature(u16 feature)
	{ _device->WriteControl((u8)Type::DeviceOut, (u8)Request::SetFeature, feature, 0, ByteArray(), _timeout); }

	void DeviceRequest::SetAddress(u16 address)
	{ _device->WriteControl((u8)Type::DeviceOut, (u8)Request::SetAddress, address, 0, ByteArray(), _timeout); }

	ByteArray DeviceRequest::GetDescriptor(DescriptorType type, u8 index, u16 lang)
	{
		ByteArray data(255);
		_device->ReadControl((u8)Type::DeviceIn, (u8)Request::GetDescriptor, ((u16)type << 8) | index, lang, data, _timeout);
		return data;
	}

	void DeviceRequest::SetDescriptor(DescriptorType type, u8 index, u16 lang, const ByteArray &data)
	{ _device->WriteControl((u8)Type::DeviceOut, (u8)Request::SetDescriptor, ((u16)type << 8) | index, lang, data, _timeout); }

	u8 DeviceRequest::GetConfiguration()
	{
		ByteArray data(1);
		_device->ReadControl((u8)Type::DeviceIn, (u8)Request::GetConfiguration, 0, 0, data, _timeout);
		return data[0];
	}

	void DeviceRequest::SetConfiguration(u8 index)
	{ _device->WriteControl((u8)Type::DeviceOut, (u8)Request::SetConfiguration, index, 0, ByteArray(), _timeout); }


	//interface requests
	InterfaceRequest::InterfaceRequest(const DevicePtr & device, u16 interface, int timeout): BaseRequest(device, timeout), _interface(interface)
	{}

	u16 InterfaceRequest::GetStatus()
	{
		ByteArray data(2);
		_device->ReadControl((u8)Type::InterfaceIn, (u8)Request::GetStatus, 0, _interface, data, _timeout);
		return data[0] | ((u16)data[1] << 8);
	}

	void InterfaceRequest::ClearFeature(u16 feature)
	{ _device->WriteControl((u8)Type::InterfaceOut, (u8)Request::ClearFeature, feature, _interface, ByteArray(), _timeout); }

	void InterfaceRequest::SetFeature(u16 feature)
	{ _device->WriteControl((u8)Type::InterfaceOut, (u8)Request::SetFeature, feature, _interface, ByteArray(), _timeout); }

	u8 InterfaceRequest::GetInterface()
	{
		ByteArray data(1);
		_device->ReadControl((u8)Type::InterfaceIn, (u8)Request::GetInterface, 0, _interface, data, _timeout);
		return data[0];
	}

	void InterfaceRequest::SetInterface(u8 alt)
	{ _device->WriteControl((u8)Type::InterfaceOut, (u8)Request::SetInterface, alt, _interface, ByteArray(), _timeout); }

	//endpoint requests

	EndpointRequest::EndpointRequest(const DevicePtr & device, u16 endpoint, int timeout): BaseRequest(device, timeout), _endpoint(endpoint)
	{ }

	u16 EndpointRequest::GetStatus()
	{
		ByteArray data(2);
		_device->ReadControl((u8)Type::EnpointIn, (u8)Request::GetStatus, 0, _endpoint, data, _timeout);
		return data[0] | ((u16)data[1] << 8);
	}

	void EndpointRequest::ClearFeature(u16 feature)
	{ _device->WriteControl((u8)Type::EnpointOut, (u8)Request::ClearFeature, feature, _endpoint, ByteArray(), _timeout); }

	void EndpointRequest::SetFeature(u16 feature)
	{ _device->WriteControl((u8)Type::EnpointOut, (u8)Request::SetFeature, feature, _endpoint, ByteArray(), _timeout); }

	void EndpointRequest::SynchFrame(u16 frameIndex)
	{
		ByteArray data(2);
		data[0] = frameIndex;
		data[1] = frameIndex >> 8;
		_device->WriteControl((u8)Type::EnpointOut, (u8)Request::SynchFrame, 0, _endpoint, ByteArray(), _timeout);
	}

}}
