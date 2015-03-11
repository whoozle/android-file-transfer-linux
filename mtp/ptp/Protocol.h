#ifndef PROTOCOL_H
#define	PROTOCOL_H

#include <mtp/usb/BulkPipe.h>

namespace mtp
{
	class Protocol
	{
		usb::BulkPipePtr _pipe;

	public:
		Protocol(usb::BulkPipePtr pipe): _pipe(pipe) { }

		void Write(const ByteArray &data);
		ByteArray Read();

	private:
		ByteArray ReadMessage();
	};
}

#endif	/* PROTOCOL_H */

