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
#ifndef COMMANDQUEUE_H
#define COMMANDQUEUE_H

#include <QObject>
#include <QQueue>
#include <QMap>
#include <mtp/ptp/ObjectFormat.h>

class MtpObjectsModel;
class CommandQueue;

struct Command
{
	virtual ~Command() { }
	virtual void execute(CommandQueue &queue) = 0;
};

struct FinishQueue : public Command
{
	quint32 DirectoryId; //return id
	FinishQueue(quint32 id): DirectoryId(id) { }
	virtual void execute(CommandQueue &queue);
};

struct FileCommand : public Command
{
	QString		Filename;

	FileCommand(const QString &filename) : Filename(filename) { }
};

struct MakeDirectory : public FileCommand
{
	bool					Root;
	MakeDirectory(const QString &filename, bool root = false) :
		FileCommand(filename), Root(root) { }
	void execute(CommandQueue &queue);
};

struct UploadFile : public FileCommand
{
	UploadFile(const QString &filename) : FileCommand(filename) { }
	void execute(CommandQueue &queue);
};

struct DownloadFile : public FileCommand
{
	quint32			ObjectId;

	DownloadFile(const QString &filename, quint32 objectId) : FileCommand(filename), ObjectId(objectId) { }
	void execute(CommandQueue &queue);
};

class CommandQueue: public QObject
{
	Q_OBJECT

private:
	MtpObjectsModel *			_model;
	qint64						_completedFilesSize;
	QString						_root;
	QMap<QString, quint32>		_directories;
	volatile bool				_aborted;

public:
	CommandQueue(MtpObjectsModel *model);
	~CommandQueue();

	MtpObjectsModel *model() const
	{ return _model; }

	void createDirectory(const QString &path, bool root);
	void uploadFile(const QString &file);
	void downloadFile(const QString &filename, quint32 objectId);

public slots:
	void onFileProgress(qint64, qint64);
	void execute(Command *cmd);
	void start(const QString &filename);
	void finish(quint32 directoryId);
	void addProgress(qint64);
	void abort();

signals:
	void started(QString);
	void progress(qint64 bytes);
	void finished();
};

#endif // COMMANDQUEUE_H
