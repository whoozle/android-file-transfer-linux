#ifndef MTP_TOKEN_H
#define	MTP_TOKEN_H

#include <mtp/types.h>

namespace mtp
{
	struct IToken : Noncopyable
	{
		virtual ~IToken() { }
	};
	DECLARE_PTR(IToken);
}


#endif
