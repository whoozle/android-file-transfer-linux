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

#include "fileuploader.h"
#include "commandqueue.h"
#include "mtpobjectsmodel.h"
#include <QStringList>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QDebug>
#include <mtp/metadata/Library.h>

FileUploader::FileUploader(MtpObjectsModel * model, QObject *parent) :
	QObject(parent),
	_model(model),
	_aborted(false)
{
	_worker = new CommandQueue(_model);
	_worker->moveToThread(&_workerThread);

	connect(&_workerThread, SIGNAL(finished()), SLOT(deleteLater()));
	connect(this, SIGNAL(executeCommand(Command*)), _worker, SLOT(execute(Command*)));
	connect(_worker, SIGNAL(progress(qint64)), SLOT(onProgress(qint64)));
	connect(_worker, SIGNAL(started(QString)), SLOT(onStarted(QString)));
	connect(_worker, SIGNAL(finished()), SLOT(onFinished()));
	_workerThread.start();
}

FileUploader::~FileUploader()
{
	_workerThread.quit();
	_workerThread.wait();
}

void FileUploader::setLibrary(const mtp::LibraryPtr & library)
{ _library = library; }

void FileUploader::onProgress(qint64 current)
{
	//qDebug() << "progress " << current << " of " << _total;
	qint64 secs = _startedAt.secsTo(QDateTime::currentDateTime());
	if (secs > 0)
		emit uploadSpeed(current / secs);

	if (_total > 0)
		emit uploadProgress(1.0 * current / _total);
}

void FileUploader::onStarted(const QString &file)
{
	emit uploadStarted(file);
}

void FileUploader::onFinished()
{
	qDebug() << "finished";
	emit finished();
}

void FileUploader::upload(QStringList files)
{
	_model->moveToThread(&_workerThread);
	_total = 0;

	mtp::ObjectId currentParentId = _model->parentObjectId();
	QList<Command *> commands;
	while(!files.empty())
	{
		QString currentFile = files.front();
		files.pop_front();
		QFileInfo currentFileInfo(currentFile);
		if (currentFileInfo.isDir())
		{
			qDebug() << "adding subdirectory" << currentFile;
			commands.push_back(new MakeDirectory(currentFile));
			QDirIterator it(currentFile, QDirIterator::Subdirectories);
			while(it.hasNext())
			{
				QString next = it.next();
				QFileInfo fi(next);
				QString filename = fi.fileName();
				if (filename == "." || filename == "..")
					continue;

				if (fi.isFile())
				{
					commands.push_back(new UploadFile(next));
					_total += fi.size();
				}
				else if (fi.isDir())
				{
					commands.push_back(new MakeDirectory(next));
					files.push_back(next);
				}
			}
		}
		else if (currentFileInfo.isFile())
		{
			commands.push_back(new UploadFile(currentFile));
			_total += currentFileInfo.size();
		}
	}
	qDebug() << "uploading" << _total << "bytes";
	if (_total < 1)
		_total = 1;

	_startedAt = QDateTime::currentDateTime();
	_aborted = false;

	for(auto command: commands)
	{
		if (_aborted)
			break;
		emit executeCommand(command);
	}
	emit executeCommand(new FinishQueue(currentParentId));
}

void FileUploader::download(const QString &rootPath, const QVector<mtp::ObjectId> &objectIds)
{
	_model->moveToThread(&_workerThread);
	_total = 0;

	mtp::ObjectId currentParentId = _model->parentObjectId();

	QVector<QPair<QString, mtp::ObjectId> > input;
	for(auto id : objectIds)
		input.push_back(qMakePair(rootPath, id));

	QVector<QPair<QString, mtp::ObjectId> > files;
	while(!input.empty())
	{
		QString prefix = input.front().first;
		mtp::ObjectId id = input.front().second;
		input.pop_front();

		MtpObjectsModel::ObjectInfo oi = _model->getInfoById(id);
		if (oi.Format == mtp::ObjectFormat::Association)
		{
			//enumerate here
			QString dirPath = prefix + "/" + oi.Filename;
			mtp::SessionPtr session = _model->session();
			mtp::msg::ObjectHandles handles = session->GetObjectHandles(mtp::Session::AllStorages, mtp::ObjectFormat::Any, id);
			qDebug() << "found " << handles.ObjectHandles.size() << " objects in " << dirPath;
			for(mtp::ObjectId id : handles.ObjectHandles)
				input.push_back(qMakePair(dirPath, id));
		}
		else
		{
			_total += oi.Size;
			files.push_back(qMakePair(prefix + "/" + oi.Filename, id));
		}
	}

	qDebug() << "downloading " << files.size() << " file(s), " << _total << " bytes";
	_startedAt = QDateTime::currentDateTime();
	_aborted = false;
	if (_total < 1)
		_total = 1;

	for(const auto & file : files)
	{
		if (_aborted)
			break;
		emit executeCommand(new DownloadFile(file.first, file.second));
	}
	emit executeCommand(new FinishQueue(currentParentId));
}

void FileUploader::importMusic(const QString & path)
{
	qDebug() << "importMusic " << path;

	_model->moveToThread(&_workerThread);
	_total = 0;

	QStringList files;
	files.push_back(path);

	QList<Command *> commands;
	while(!files.empty())
	{
		QString currentFile = files.front();
		files.pop_front();
		QFileInfo currentFileInfo(currentFile);
		if (currentFileInfo.isDir())
		{
			qDebug() << "going into subdirectory" << currentFile;
			QDirIterator it(currentFile, QDirIterator::Subdirectories);
			while(it.hasNext())
			{
				QString next = it.next();
				QFileInfo fi(next);
				QString filename = fi.fileName();
				if (filename == "." || filename == "..")
					continue;

				if (fi.isFile())
				{
					//commands.push_back(new ImportFile(next));
					_total += fi.size();
				}
				else if (fi.isDir())
				{
					files.push_back(next);
				}
			}
		}
		else if (currentFileInfo.isFile())
		{
			//commands.push_back(new ImportFile(currentFile));
			_total += currentFileInfo.size();
		}
	}
	qDebug() << "uploading" << _total << "bytes";
	if (_total < 1)
		_total = 1;

	_startedAt = QDateTime::currentDateTime();
	_aborted = false;

	for(auto command: commands)
	{
		if (_aborted)
			break;
		emit executeCommand(command);
	}
	emit executeCommand(new FinishQueue(mtp::Session::Root));
}

void FileUploader::abort()
{
	qDebug() << "abort request";
	_aborted = true;
	_worker->abort();
}
