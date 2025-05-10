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

#ifndef AFTL_QT_FILEUPLOADER_H
#define AFTL_QT_FILEUPLOADER_H

#include <QObject>
#include <QThread>
#include <QDateTime>
#include <mtp/types.h>
#include <mtp/ptp/ObjectFormat.h>
#include <memory>

class MtpObjectsModel;
struct Command;
class CommandQueue;

namespace mtp
{
	struct ObjectId;
	class Library;
	DECLARE_PTR(Library);
}

class FileUploader : public QObject
{
	Q_OBJECT

private:
	MtpObjectsModel	*	_model;
	QThread				_workerThread;
	CommandQueue *		_worker;
	qint64				_total;
	QDateTime			_startedAt;
	bool				_aborted;

private slots:
	void onTotal(qint64 total);
	void onProgress(qint64 current);
	void onStarted(const QString &file);
	void onFinished();

public:
	explicit FileUploader(MtpObjectsModel * model, QObject *parent = 0);
	~FileUploader();

	void tryCreateLibrary();
	mtp::LibraryPtr library() const;

	void upload(QStringList files, mtp::ObjectFormat format);
	void importMusic(const QString & path);
	void download(const QString &path, const QVector<mtp::ObjectId> & objectIds);

public slots:
	void abort();

signals:
	void executeCommand(Command *cmd);

	//incoming signals (from worker)
	void uploadStarted(QString file);
	void uploadProgress(float);
	void uploadSpeed(qint64);
	void finished();
};

#endif // FILEUPLOADER_H
