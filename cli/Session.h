#ifndef AFT_CLI_SESSION_H
#define AFT_CLI_SESSION_H

#include <mtp/ptp/Device.h>
#include <mtp/ptp/Session.h>
#include <mtp/ptp/Messages.h>

namespace cli
{
	class Session
	{
		mtp::DevicePtr				_device;
		mtp::SessionPtr				_session;
		mtp::msg::DeviceInfo		_gdi;
		mtp::u32					_cd;

	public:
		Session(const mtp::DevicePtr &device);

		void InteractiveInput();

		void List(mtp::u32 parent = mtp::Session::Root);

		template<typename FunctionType>
		void AddCommand(const std::string &name, FunctionType && callback)
		{ }
	};
}

#endif

