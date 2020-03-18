/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2020  Vladimir Menshakov

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License,
    or (at your option) any later version.

    This library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#ifndef AFTL_QT_UTILS_H
#define AFTL_QT_UTILS_H

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

int GetCoverScore(const QString &str_);

#endif // UTILS

