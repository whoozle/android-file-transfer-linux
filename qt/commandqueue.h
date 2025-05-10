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

#ifndef AFTL_QT_COMMANDQUEUE_H
#define AFTL_QT_COMMANDQUEUE_H

#include <QObject>
#include <QQueue>
#include <QMap>
#include <QDebug>
#include <mtp/ptp/ObjectId.h>
#include <mtp/ptp/ObjectFormat.h>
#include <mtp/metadata/Library.h>

class MtpObjectsModel;
class CommandQueue;

namespace mtp
{
	class Library;
	DECLARE_PTR(Library);
}

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
	mtp::ObjectFormat Format;
	UploadFile(const QString &filename, mtp::ObjectFormat format) : FileCommand(filename), Format(format) { }
	void execute(CommandQueue &queue);
};

struct ImportFile : public FileCommand
{
	ImportFile(const QString &filename) : FileCommand(filename) { }
	void execute(CommandQueue &queue);
};

struct DownloadFile : public FileCommand
{
	mtp::ObjectId			ObjectId;

	DownloadFile(const QString &filename, mtp::ObjectId objectId) : FileCommand(filename), ObjectId(objectId) { }
	void execute(CommandQueue &queue);
};

struct LoadLibrary : public Command
{
	void execute(CommandQueue &queue);
};

class CommandQueue: public QObject
{
	Q_OBJECT

private:
	MtpObjectsModel *				_model;
	qint64							_completedFilesSize;
	QMap<QString, mtp::ObjectId>	_directories;
	std::map<QString, mtp::Library::AlbumPtr> _albums;

	struct Cover
	{
		QString 	Path;
		int			Score;
	};

	std::map<QString, Cover> 		_covers;
	mtp::LibraryPtr					_library;
	volatile bool					_aborted;

public:
	CommandQueue(MtpObjectsModel *model);
	~CommandQueue();

	MtpObjectsModel *model() const
	{ return _model; }

	mtp::LibraryPtr library() const;

	void loadLibrary();
	void createDirectory(const QString &path);
	void uploadFile(const QString &file, mtp::ObjectFormat format);
	void downloadFile(const QString &filename, mtp::ObjectId objectId);
	void importFile(const QString &file);

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
	void total(qint64 bytes);
	void finished();
};

#endif // COMMANDQUEUE_H
