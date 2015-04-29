#ifndef UTILS
#define UTILS

#include <QString>
#include <string>

inline QString fromUtf8(const std::string &str)
{
	return QString::fromUtf8(str.c_str(), str.size());
}

#endif // UTILS

