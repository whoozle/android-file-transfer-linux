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
#include "mtpobjectsmodel.h"
#include <QApplication>
#include <QStringList>
#include <QFileInfo>
#include <QDebug>

#include <unistd.h>

FileUploaderWorker::FileUploaderWorker(MtpObjectsModel *model): _model(model), _completedFilesSize(0)
{
	connect(_model, SIGNAL(filePositionChanged(qint64,qint64)), this, SLOT(onFileProgress(qint64,qint64)));
	qDebug() << "upload worker started";
}

FileUploaderWorker::~FileUploaderWorker()
{
	qDebug() << "upload worker stopped";
}

void FileUploaderWorker::uploadFile(const QString &file)
{
	if (file.isEmpty())
	{
		_model->moveToThread(QApplication::instance()->thread());
		emit finished();
		_completedFilesSize = 0;
		return;
	}
	qDebug() << "uploading file " << file;
	QFileInfo fi(file);
	emit started(fi.fileName());
	try
	{
		_model->uploadFile(file);
	} catch(const std::exception &ex)
	{ qDebug() << "uploading file " << file << " failed: " << ex.what(); }

	_completedFilesSize += fi.size();
	emit progress(_completedFilesSize);
}

void FileUploaderWorker::downloadFile(const QString &file, quint32 objectId)
{
	if (objectId == 0 || file.isEmpty())
	{
		_model->moveToThread(QApplication::instance()->thread());
		emit finished();
		_completedFilesSize = 0;
		return;
	}
	qDebug() << "downloading " << objectId << "to" << file;

	QFileInfo fi(file);
	emit started(fi.fileName());
	try
	{
		_model->downloadFile(file, objectId);
	} catch(const std::exception &ex)
	{ qDebug() << "downloading file " << file << " failed: " << ex.what(); }

	_completedFilesSize += fi.size();
	emit progress(_completedFilesSize);
}

void FileUploaderWorker::onFileProgress(qint64 pos, qint64)
{
	//qDebug() << "on file progress " << _completedFilesSize << " " << pos;
	emit progress(_completedFilesSize + pos);
}

FileUploader::FileUploader(MtpObjectsModel * model, QObject *parent) :
	QObject(parent),
	_model(model)
{
	FileUploaderWorker * worker = new FileUploaderWorker(_model);
	worker->moveToThread(&_workerThread);

	connect(&_workerThread, SIGNAL(finished()), SLOT(deleteLater()));
	connect(this, SIGNAL(uploadFile(QString)), worker, SLOT(uploadFile(QString)));
	connect(this, SIGNAL(downloadFile(QString,quint32)), worker, SLOT(downloadFile(QString,quint32)));
	connect(worker, SIGNAL(progress(qlonglong)), SLOT(onProgress(qlonglong)));
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
		emit uploadFile(file);
	}
	emit uploadFile(QString());
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
		emit downloadFile(path + "/" + file.first, file.second);
	}
	emit downloadFile(QString(), 0);
}
