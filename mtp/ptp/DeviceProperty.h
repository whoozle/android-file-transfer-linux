/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2017  Vladimir Menshakov

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

#ifndef DEVICEPROPERTY_H
#define	DEVICEPROPERTY_H

namespace mtp
{

	enum struct DeviceProperty
	{
		Undefined					= 0x5000,
		BatteryLevel				= 0x5001,
		FunctionalMode				= 0x5002,
		ImageSize					= 0x5003,
		CompressionSetting			= 0x5004,
		WhiteBalance				= 0x5005,
		RgbGain						= 0x5006,
		FNumber						= 0x5007,
		FocalLength					= 0x5008,
		FocusDistance				= 0x5009,
		FocusMode					= 0x500a,
		ExposureMeteringMode		= 0x500b,
		FlashMode					= 0x500c,
		ExposureTime				= 0x500d,
		ExposureProgramMode			= 0x500e,
		ExposureIndex				= 0x500f,
		ExposureBiasCompensation	= 0x5010,
		Datetime					= 0x5011,
		CaptureDelay				= 0x5012,
		StillCaptureMode			= 0x5013,
		Contrast					= 0x5014,
		Sharpness					= 0x5015,
		DigitalZoom					= 0x5016,
		EffectMode					= 0x5017,
		BurstNumber					= 0x5018,
		BurstInterval				= 0x5019,
		TimelapseNumber				= 0x501a,
		TimelapseInterval			= 0x501b,
		FocusMeteringMode			= 0x501c,
		UploadUrl					= 0x501d,
		Artist						= 0x501e,
		CopyrightInfo				= 0x501f,
		SynchronizationPartner		= 0xd401,
		DeviceFriendlyName			= 0xd402,
		Volume						= 0xd403,
		SupportedFormatsOrdered		= 0xd404,
		DeviceIcon					= 0xd405,
		PlaybackRate				= 0xd410,
		PlaybackObject				= 0xd411,
		PlaybackContainerIndex		= 0xd412,
		SessionInitiatorVersionInfo	= 0xd406,
		PerceivedDeviceType			= 0xd407
	};

}

#endif	/* OBJECTPROPERTY_H */
