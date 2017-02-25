/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2017  Vladimir Menshakov

    Android File Transfer For Linux is free software: you can redistribute
    it and/or modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    Android File Transfer For Linux is distributed in the hope that it will
    be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Android File Transfer For Linux.
    If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef QTOBJECTSTREAM_H
#define QTOBJECTSTREAM_H

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
		CheckCancelled();
		qint64 r = _file.read(static_cast<char *>(static_cast<void *>(data)), size);
		if (r < 0)
			throw std::runtime_error(_file.errorString().toStdString());
		emit positionChanged(_file.pos(), _size);
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
