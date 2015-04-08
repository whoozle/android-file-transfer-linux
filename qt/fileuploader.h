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
#ifndef FILEUPLOADER_H
#define FILEUPLOADER_H

#include <QObject>
#include <QThread>

class MtpObjectsModel;

class FileUploaderWorker: public QObject
{
	Q_OBJECT

private:
	MtpObjectsModel *	_model;
	qint64				_completedFilesSize;

public:
	FileUploaderWorker(MtpObjectsModel *model);
	~FileUploaderWorker();

public slots:
	void onFileProgress(qint64, qint64);
	void uploadFile(const QString &file);
	void downloadFile(const QString &path, quint32 objectId);

signals:
	void started(QString);
	void progress(qint64 bytes);
	void finished();
};

class FileUploader : public QObject
{
	Q_OBJECT

private:
	MtpObjectsModel	*	_model;
	QThread				_workerThread;
	qint64				_total;

private slots:
	void onProgress(qint64 current);
	void onStarted(const QString &file);
	void onFinished();

public:
	explicit FileUploader(MtpObjectsModel * model, QObject *parent = 0);
	~FileUploader();

	void upload(const QStringList &files);
	void download(const QString &path, const QList<quint32> & objectIds);

signals:
	//outgoing signals (to worker)
	void uploadFile(QString file);
	void downloadFile(QString file, quint32 objectId);

	//incoming signals (from worker)
	void uploadStarted(QString file);
	void uploadProgress(float);
	void finished();
};

#endif // FILEUPLOADER_H
