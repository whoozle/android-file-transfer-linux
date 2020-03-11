#ifndef AFTL_MTP_SCOPE_GUARD_H
#define AFTL_MTP_SCOPE_GUARD_H

#include <functional>
#include <mtp/types.h>

namespace mtp
{
	class scope_guard : Noncopyable
	{
		using Callback = std::function<void ()>;
		Callback _callback;

	public:
		scope_guard(Callback && c): _callback(c)
		{ }

		~scope_guard()
		{ _callback(); }
	};

}

#endif
