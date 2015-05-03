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
#include <QDebug>

FileUploader::FileUploader(MtpObjectsModel * model, QObject *parent) :
	QObject(parent),
	_model(model)
{
	CommandQueue * worker = new CommandQueue(_model);
	worker->moveToThread(&_workerThread);

	connect(&_workerThread, SIGNAL(finished()), SLOT(deleteLater()));
	connect(this, SIGNAL(executeCommand(Command*)), worker, SLOT(execute(Command*)));
	connect(worker, SIGNAL(progress(qint64)), SLOT(onProgress(qint64)));
	connect(worker, SIGNAL(started(QString)), SLOT(onStarted(QString)));
	connect(worker, SIGNAL(finished()), SLOT(onFinished()));
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

void FileUploader::upload(const QStringList &files)
{
	_model->moveToThread(&_workerThread);

	_total = 1;
	for(QString file : files)
	{
		QFileInfo fi(file);
		_total += fi.size();
	}
	_startedAt = QDateTime::currentDateTime();

	for(QString file : files)
	{
		emit executeCommand(new UploadFile(file));
	}
	emit executeCommand(new FinishQueue());
}

void FileUploader::download(const QString &path, const QList<quint32> &objectIds)
{
	_model->moveToThread(&_workerThread);

	_total = 1;
	QVector<QPair<QString, mtp::u32> > files;
	for(quint32 id : objectIds)
	{
		MtpObjectsModel::ObjectInfo oi = _model->getInfo(id);
		_total += oi.Size; //fixme: invalid for > 4Gb
		files.push_back(QPair<QString, mtp::u32>(oi.Filename, id));
	}
	qDebug() << "downloading " << files.size() << " file(s), " << _total << " bytes";
	_startedAt = QDateTime::currentDateTime();

	for(const auto & file : files)
	{
		emit executeCommand(new DownloadFile(path + "/" + file.first, file.second));
	}
	emit executeCommand(new FinishQueue());
}
