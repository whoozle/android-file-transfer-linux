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
	QtObjectOutputStream(const QString &file): _file(file), _size(_file.size())
	{ _file.open(QFile::WriteOnly); }

	bool Valid() const
	{ return _file.isOpen(); }

	virtual mtp::u64 GetSize() const
	{ return _size; }

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
