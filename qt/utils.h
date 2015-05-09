/*
 * Android File Transfer for Linux: MTP client for android devices
 * Copyright (C) 2015  Vladimir Menshakov

 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
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

