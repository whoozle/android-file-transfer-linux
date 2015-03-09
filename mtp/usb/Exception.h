#ifndef MTP_EXCEPTION_H
#define	MTP_EXCEPTION_H

#include <stdexcept>

namespace mtp { namespace usb
{
	class Exception : public std::runtime_error
	{
	public:
		Exception(const std::string &what, int retCode) throw();
		static std::string GetErrorMessage(int retCode);
	};
}}

#endif
