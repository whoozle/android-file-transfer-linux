#ifndef AFT_USB_REQUEST_H
#define	AFT_USB_REQUEST_H

#include <mtp/types.h>
#include <mtp/ByteArray.h>

namespace mtp { namespace usb
{
	class Device;
	DECLARE_PTR(Device);

	class BaseRequest
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

	struct DeviceRequest : BaseRequest
	{
		using BaseRequest::BaseRequest;

		enum struct Type : u8
		{
			DeviceOut	= 0x00,
			DeviceIn	= 0x80
		};

		enum struct Request : u8
		{
			GetStatus			= 0,
			ClearFeature		= 1,
			SetFeature			= 3,
			SetAddress			= 5,
			GetDescriptor		= 6,
			SetDescriptor		= 7,
			GetConfiguration	= 8,
			SetConfiguration	= 9
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

	class InterfaceRequest : public BaseRequest
	{
		u16 _interface;
	public:

		enum struct Type : u8
		{
			InterfaceOut	= 0x01,
			InterfaceIn		= 0x81
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

	class EndpointRequest : public BaseRequest
	{
		u16 _endpoint;

	public:
		enum struct Type : u8
		{
			EnpointOut		= 0x02,
			EnpointIn		= 0x82
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
