#ifndef OBJECTFORMAT_H
#define	OBJECTFORMAT_H

#include <mtp/types.h>
#include <string>

namespace mtp
{
	//please look here: https://msdn.microsoft.com/en-us/library/jj584872(v=winembedded.70).aspx

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

		//audio
		Wma				= 0xb901,
		Ogg				= 0xb902,
		Aac				= 0xb903,
		Audible			= 0xb904,
		Flac			= 0xb906,

		//video
		Wmv				= 0xb980,
		Mp4				= 0xb982,
		Mp2				= 0xb983,
		_3gp			= 0xb984,

		AudioAlbum		= 0xba03,

		//playlists
		Wpl				= 0xba10,
		M3u				= 0xba11,
		Mpl				= 0xba12,
		Asx				= 0xba13,
		Pls				= 0xba14,

		Xml				= 0xba82,
		Doc				= 0xba83,
		Mht				= 0xba84,
		Xls				= 0xba85,
		Ppt				= 0xba86,

		VCard2			= 0xbb82
	};

	ObjectFormat ObjectFormatFromFilename(const std::string &filename);

	DECLARE_ENUM(ObjectFormat, u16);

}

#endif
