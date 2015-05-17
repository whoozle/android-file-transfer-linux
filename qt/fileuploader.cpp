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
#include "fileuploader.h"
#include "commandqueue.h"
#include "mtpobjectsmodel.h"
#include <QStringList>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QDebug>

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

void FileUploader::onProgress(qint64 current)
{
	//qDebug() << "progress " << current << " of " << _total;
	qint64 secs = _startedAt.secsTo(QDateTime::currentDateTime());
	if (secs)
		emit uploadSpeed(current / secs);
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

	mtp::u32 currentParentId = _model->parentObjectId();
	QList<Command *> commands;
	while(!files.empty())
	{
		QString currentFile = files.front();
		files.pop_front();
		QFileInfo currentFileInfo(currentFile);
		if (currentFileInfo.isDir())
		{
			qDebug() << "adding subdirectory" << currentFile;
			commands.push_back(new MakeDirectory(currentFile, true));
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

void FileUploader::download(const QString &rootPath, const QVector<quint32> &objectIds)
{
	_model->moveToThread(&_workerThread);
	_total = 0;

	mtp::u32 currentParentId = _model->parentObjectId();

	QVector<QPair<QString, mtp::u32> > input;
	for(auto id : objectIds)
		input.push_back(qMakePair(rootPath, id));

	QVector<QPair<QString, mtp::u32> > files;
	while(!input.empty())
	{
		QString prefix = input.front().first;
		mtp::u32 id = input.front().second;
		input.pop_front();

		MtpObjectsModel::ObjectInfo oi = _model->getInfoById(id);
		if (oi.Format == mtp::ObjectFormat::Association)
		{
			//enumerate here
			QString dirPath = prefix + "/" + oi.Filename;
			mtp::SessionPtr session = _model->session();
			mtp::msg::ObjectHandles handles = session->GetObjectHandles(mtp::Session::AllStorages, mtp::Session::AllFormats, id);
			qDebug() << "found " << handles.ObjectHandles.size() << " objects in " << dirPath;
			for(mtp::u32 id : handles.ObjectHandles)
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

void FileUploader::abort()
{
	qDebug() << "abort request";
	_aborted = true;
	_worker->abort();
}
