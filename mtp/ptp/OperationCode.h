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
#ifndef OPERATIONCODE_H
#define	OPERATIONCODE_H

#include <mtp/types.h>

namespace mtp
{
	enum class OperationCode : u16
	{
		GetDeviceInfo			= 0x1001,
		OpenSession             = 0x1002,
		CloseSession            = 0x1003,
		GetStorageIDs           = 0x1004,
		GetStorageInfo          = 0x1005,
		GetNumObjects           = 0x1006,
		GetObjectHandles        = 0x1007,
		GetObjectInfo           = 0x1008,
		GetObject               = 0x1009,
		GetThumb                = 0x100a,
		DeleteObject            = 0x100b,
		SendObjectInfo          = 0x100c,
		SendObject              = 0x100d,
		InitiateCapture         = 0x100e,
		FormatStore             = 0x100f,
		ResetDevice             = 0x1010,
		SelfTest                = 0x1011,
		SetObjectProtection		= 0x1012,
		PowerDown               = 0x1013,
		GetDevicePropDesc       = 0x1014,
		GetDevicePropValue      = 0x1015,
		SetDevicePropValue      = 0x1016,
		ResetDevicePropValue    = 0x1017,
		TerminateOpenCapture    = 0x1018,
		MoveObject              = 0x1019,
		CopyObject              = 0x101a,
		GetPartialObject        = 0x101b,
		InitiateOpenCapture     = 0x101c,
		GetObjectPropsSupported = 0x9801,
		GetObjectPropDesc		= 0x9802,
		GetObjectPropValue		= 0x9803,
		SetObjectPropValue		= 0x9804,
		GetObjectReferences		= 0x9810,
		SetObjectReferences		= 0x9811,
		Skip					= 0x9820
	};

	DECLARE_ENUM(OperationCode, u16);
}

#endif	/* OPERATIONCODE_H */
