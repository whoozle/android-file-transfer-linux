#ifndef UTILS
#define UTILS

#include <QString>
#include <string>

inline QString fromUtf8(const std::string &str)
{
	return QString::fromUtf8(str.c_str(), str.size());
}

inline std::string toUtf8(const QString &str)
{
	QByteArray utf8(str.toUtf8());
	return std::string(utf8.data(), utf8.size());
}

#endif // UTILS

