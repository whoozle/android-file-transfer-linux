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
		return std::move(data);
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

	u16 InterfaceRequest::GetStatus(u16 interface)
	{
		ByteArray data(2);
		_device->ReadControl((u8)Type::InterfaceIn, (u8)Request::GetStatus, 0, interface, data, _timeout);
		return data[0] | ((u16)data[1] << 8);
	}

	void InterfaceRequest::ClearFeature(u16 interface, u16 feature)
	{ _device->WriteControl((u8)Type::InterfaceOut, (u8)Request::ClearFeature, feature, interface, ByteArray(), _timeout); }

	void InterfaceRequest::SetFeature(u16 interface, u16 feature)
	{ _device->WriteControl((u8)Type::InterfaceOut, (u8)Request::SetFeature, feature, interface, ByteArray(), _timeout); }

	u8 InterfaceRequest::GetInterface(u16 interface)
	{
		ByteArray data(1);
		_device->ReadControl((u8)Type::InterfaceIn, (u8)Request::GetInterface, 0, interface, data, _timeout);
		return data[0];
	}

	void InterfaceRequest::SetInterface(u16 interface, u8 alt)
	{ _device->WriteControl((u8)Type::InterfaceOut, (u8)Request::SetInterface, alt, interface, ByteArray(), _timeout); }

	//endpoint requests

	u16 EndpointRequest::GetStatus(u16 endpoint)
	{
		ByteArray data(2);
		_device->ReadControl((u8)Type::EnpointIn, (u8)Request::GetStatus, 0, endpoint, data, _timeout);
		return data[0] | ((u16)data[1] << 8);
	}

	void EndpointRequest::ClearFeature(u16 endpoint, u16 feature)
	{ _device->WriteControl((u8)Type::EnpointOut, (u8)Request::ClearFeature, feature, endpoint, ByteArray(), _timeout); }

	void EndpointRequest::SetFeature(u16 endpoint, u16 feature)
	{ _device->WriteControl((u8)Type::EnpointOut, (u8)Request::SetFeature, feature, endpoint, ByteArray(), _timeout); }

	void EndpointRequest::SynchFrame(u16 endpoint, u16 frameIndex)
	{
		ByteArray data(2);
		data[0] = frameIndex;
		data[1] = frameIndex >> 8;
		_device->WriteControl((u8)Type::EnpointOut, (u8)Request::SynchFrame, 0, endpoint, ByteArray(), _timeout);
	}

}}
