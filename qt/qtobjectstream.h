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

#ifndef AFTL_QT_QTOBJECTSTREAM_H
#define AFTL_QT_QTOBJECTSTREAM_H

#include <QObject>
#include <QFile>
#include <mtp/ptp/IObjectStream.h>

class QtObjectInputStream : public QObject, public mtp::IObjectInputStream, public mtp::CancellableStream
{
	Q_OBJECT

signals:
	void positionChanged(qint64, qint64);

private:
	QFile		_file;
	qint64		_size;
	int			_progress;

public:
	QtObjectInputStream(const QString &file) : _file(file), _size(_file.size()), _progress(-1) {
		_file.open(QFile::ReadOnly);
	}

	bool Valid() const
	{ return _file.isOpen(); }

	virtual mtp::u64 GetSize() const
	{ return _size; }

	virtual size_t Read(mtp::u8 *data, size_t size)
	{
		CheckCancelled();
		qint64 r = _file.read(static_cast<char *>(static_cast<void *>(data)), size);
		if (r < 0)
			throw std::runtime_error(_file.errorString().toStdString());

		int progress = _size > 0? _file.pos() * 1000L / _size: 0;
		if (progress != _progress) //throttle events a bit
		{
			_progress = progress;
			emit positionChanged(_file.pos(), _size);
		}
		return r;
	}
};

class QtObjectOutputStream : public QObject, public mtp::IObjectOutputStream, public mtp::CancellableStream
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
		CheckCancelled();
		qint64 r = _file.write(static_cast<const char *>(static_cast<const void *>(data)), size);
		if (r < 0)
			throw std::runtime_error(_file.errorString().toStdString());
		emit positionChanged(_file.pos(), _size);
		return r;
	}
};

#endif // QTOBJECTSTREAM_H
