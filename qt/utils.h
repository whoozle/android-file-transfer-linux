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

inline int GetCoverScore(const QString &str_)
{
	QString str = str_.toLower();
	int score = 0;
	if (str.contains("0001"))
		score += 1001;
	else if (str.contains("0000"))
		score += 1000;
	else if (str.contains("001"))
		score += 101;
	else if (str.contains("000"))
		score += 100;
	else if (str.contains("01"))
		score += 11;
	else if (str.contains("00"))
		score += 10;
	else if (str.contains("1"))
		score += 1;

	if (str.contains("art"))
		score += 10000;
	if (str.contains("album"))
		score += 10000;
	if (str.contains("cover"))
		score += 20000;
	if (str.contains("large"))
		score += 20000;
	if (str.contains("small"))
		score += 10000;
	if (str.contains("folder"))
		score += 10000;
	return score;
}


#endif // UTILS

