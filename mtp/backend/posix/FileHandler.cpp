#include <FileHandler.h>

#include <unistd.h>

namespace mtp { namespace posix
{

	FileHandler::~FileHandler()
	{ close(_fd); }

}}
