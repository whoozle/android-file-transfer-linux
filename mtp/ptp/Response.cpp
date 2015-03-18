#include <mtp/ptp/Response.h>
#include <stdio.h>

namespace mtp
{
#	define R(NAME) case ResponseType::NAME: return #NAME

	namespace
	{
		std::string GetResponseName(ResponseType r)
		{
			switch(r)
			{
				R(OK);
				R(GeneralError);
				R(SessionNotOpen);
				R(InvalidTransaction);
				R(OperationNotSupported);
				R(ParameterNotSupported);
				R(IncompleteTransfer);
				R(InvalidStorageID);
				R(InvalidObjectHandle);
				R(DevicePropNotSupported);
				R(InvalidObjectFormatCode);
				R(StoreFull);
				R(ObjectWriteProtected);
				R(StoreReadOnly);
				R(AccessDenied);
				R(NoThumbnailPresent);
				R(SelfTestFailed);
				R(PartialDeletion);
				R(StoreNotAvailable);
				R(SpecificationByFormatUnsupported);
				R(NoValidObjectInfo);
				R(InvalidCodeFormat);
				R(UnknownVendorCode);
				R(CaptureAlreadyTerminated);
				R(DeviceBusy);
				R(InvalidParentObject);
				R(InvalidDevicePropFormat);
				R(InvalidDevicePropValue);
				R(InvalidParameter);
				R(SessionAlreadyOpen);
				R(TransactionCancelled);
				R(SpecificationOfDestinationUnsupported);
				default:
					return "Unknown";
			}
		}

		std::string FormatMessage(ResponseType r)
		{
			char buf[1024];
			snprintf(buf, sizeof(buf), "invalid response code %s (0x%04x)", GetResponseName(r).c_str(), r);
			return buf;
		}
	}

	InvalidResponseException::InvalidResponseException(ResponseType type): std::runtime_error(FormatMessage(type)), Type(type)
	{ }

}
