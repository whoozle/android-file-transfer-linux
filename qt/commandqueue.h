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

#ifndef COMMANDQUEUE_H
#define COMMANDQUEUE_H

#include <QObject>
#include <QQueue>
#include <QMap>
#include <mtp/ptp/ObjectId.h>
#include <mtp/ptp/ObjectFormat.h>

class MtpObjectsModel;
class CommandQueue;

struct Command
{
	virtual ~Command() = default;
	virtual void execute(CommandQueue &queue) = 0;
};

struct FinishQueue : public Command
{
	mtp::ObjectId DirectoryId; //return id
	FinishQueue(mtp::ObjectId id): DirectoryId(id) { }
	virtual void execute(CommandQueue &queue);
};

struct FileCommand : public Command
{
	QString		Filename;

	FileCommand(const QString &filename) : Filename(filename) { }
};

struct MakeDirectory : public FileCommand
{
	MakeDirectory(const QString &filename) :
		FileCommand(filename) { }
	void execute(CommandQueue &queue);
};

struct UploadFile : public FileCommand
{
	UploadFile(const QString &filename) : FileCommand(filename) { }
	void execute(CommandQueue &queue);
};

struct DownloadFile : public FileCommand
{
	mtp::ObjectId			ObjectId;

	DownloadFile(const QString &filename, mtp::ObjectId objectId) : FileCommand(filename), ObjectId(objectId) { }
	void execute(CommandQueue &queue);
};

class CommandQueue: public QObject
{
	Q_OBJECT

private:
	MtpObjectsModel *				_model;
	qint64							_completedFilesSize;
	QMap<QString, mtp::ObjectId>	_directories;
	volatile bool					_aborted;

public:
	CommandQueue(MtpObjectsModel *model);
	~CommandQueue();

	MtpObjectsModel *model() const
	{ return _model; }

	void createDirectory(const QString &path);
	void uploadFile(const QString &file);
	void downloadFile(const QString &filename, mtp::ObjectId objectId);

public slots:
	void onFileProgress(qint64, qint64);
	void execute(Command *cmd);
	void start(const QString &filename);
	void finish(mtp::ObjectId directoryId);
	void addProgress(qint64);
	void abort();

signals:
	void started(QString);
	void progress(qint64 bytes);
	void finished();
};

#endif // COMMANDQUEUE_H
