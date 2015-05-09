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
#ifndef QTOBJECTSTREAM_H
#define QTOBJECTSTREAM_H

#include <QObject>
#include <QFile>
#include <mtp/ptp/IObjectStream.h>

class QtObjectInputStream : public QObject, public mtp::IObjectInputStream
{
	Q_OBJECT

signals:
	void positionChanged(qint64, qint64);

private:
	QFile		_file;
	qint64		_size;

public:
	QtObjectInputStream(const QString &file) : _file(file), _size(_file.size()) {
		_file.open(QFile::ReadOnly);
	}

	bool Valid() const
	{ return _file.isOpen(); }

	virtual mtp::u64 GetSize() const
	{ return _size; }

	virtual size_t Read(mtp::u8 *data, size_t size)
	{
		qint64 r = _file.read(static_cast<char *>(static_cast<void *>(data)), size);
		if (r < 0)
			throw std::runtime_error(_file.errorString().toStdString());
		emit positionChanged(_file.pos(), _size);
		return r;
	}
};

class QtObjectOutputStream : public QObject, public mtp::IObjectOutputStream
{
	Q_OBJECT

signals:
	void positionChanged(qint64, qint64);

private:
	QFile		_file;
	qint64		_size;

public:
	QtObjectOutputStream(const QString &file): _file(file)
	{ _file.open(QFile::WriteOnly | QFile::Truncate); }

	bool Valid() const
	{ return _file.isOpen(); }

	virtual size_t Write(const mtp::u8 *data, size_t size)
	{
		qint64 r = _file.write(static_cast<const char *>(static_cast<const void *>(data)), size);
		if (r < 0)
			throw std::runtime_error(_file.errorString().toStdString());
		emit positionChanged(_file.pos(), _size);
		return r;
	}
};

#endif // QTOBJECTSTREAM_H
