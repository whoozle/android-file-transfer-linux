#ifndef OBJECTFORMAT_H
#define	OBJECTFORMAT_H

namespace mtp
{

	enum struct ObjectFormat : u16
	{
		Undefined		= 0x3000,
		Association		= 0x3001,
		Script			= 0x3002,
		Executable		= 0x3003,
		Text			= 0x3004,
		Html			= 0x3005,
		Dpof			= 0x3006,
		Aiff			= 0x3007,
		Wav				= 0x3008,
		Mp3				= 0x3009,
		Avi				= 0x300a,
		Mpeg			= 0x300b,
		Asf				= 0x300c,
		UndefinedImage	= 0x3800,
		ExifJpeg		= 0x3801,
		TiffEp			= 0x3802,
		Flashpix		= 0x3803,
		Bmp				= 0x3804,
		Ciff			= 0x3805,
		Reserved		= 0x3806,
		Gif				= 0x3807,
		Jfif			= 0x3808,
		Pcd				= 0x3809,
		Pict			= 0x380a,
		Png				= 0x380b,
		Reserved2		= 0x380c,
		Tiff			= 0x380d,
		TiffIt			= 0x380e,
		Jp2				= 0x380f,
		Jpx				= 0x3810,
	};

	template<typename Stream>
	Stream &operator << (Stream & stream, ObjectFormat format)
	{ stream << (u16)format; return stream; }

	template<typename Stream>
	Stream &operator >> (Stream & stream, ObjectFormat &format)
	{ u16 value; stream >> value; format = (ObjectFormat)value; return stream; }

}

#endif
