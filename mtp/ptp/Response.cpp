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
				R(InvalidObjectPropCode);
				R(InvalidObjectPropFormat);
				R(InvalidObjectPropValue);
				R(InvalidObjectReference);
				R(GroupNotSupported);
				R(InvalidDataset);
				R(UnsupportedSpecByGroup);
				R(UnsupportedSpecByDepth);
				R(ObjectTooLarge);
				R(ObjectPropNotSupported);

				default:
					return "Unknown";
			}
		}

		std::string FormatMessage(ResponseType r)
		{
			char buf[1024];
			snprintf(buf, sizeof(buf), "invalid response code %s (0x%04hx)", GetResponseName(r).c_str(), (unsigned)r);
			return buf;
		}
	}

	InvalidResponseException::InvalidResponseException(const std::string &where, ResponseType type): std::runtime_error(where + ": " + FormatMessage(type)), Type(type)
	{ }

}
