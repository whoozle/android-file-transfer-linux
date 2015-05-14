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
#include "commandqueue.h"
#include "mtpobjectsmodel.h"
#include "utils.h"
#include <QFileInfo>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QApplication>

void FinishQueue::execute(CommandQueue &queue)
{
	queue.finish();
}

void UploadFile::execute(CommandQueue &queue)
{
	queue.uploadFile(Filename);
}

void MakeDirectory::execute(CommandQueue &queue)
{
	queue.createDirectory(Filename, Root);
}

void CommandQueue::uploadFile(const QString &filename)
{
	qDebug() << "uploading file " << filename;

	QFileInfo fi(filename);
	if (_directories.empty())
		_directories[fi.dir().path()] = _model->parentObjectId();
	start(fi.fileName());
	auto parent = _directories.find(fi.dir().path());
	Q_ASSERT(parent != _directories.end());
	if (_model->parentObjectId() != parent.value())
		_model->setParent(parent.value());
	try
	{
		_model->uploadFile(filename);
	} catch(const std::exception &ex)
	{ qDebug() << "uploading file " << filename << " failed: " << fromUtf8(ex.what()); }

	addProgress(fi.size());
}

void DownloadFile::execute(CommandQueue &queue)
{
	qDebug() << "downloading " << ObjectId << "to" << Filename;

	QFileInfo fi(Filename);
	QDir().mkpath(fi.dir().path());
	queue.start(fi.fileName());
	try
	{
		queue.model()->downloadFile(Filename, ObjectId);
	} catch(const std::exception &ex)
	{ qDebug() << "downloading file " << Filename << " failed: " << fromUtf8(ex.what()); }

	queue.addProgress(fi.size());
}

void CommandQueue::createDirectory(const QString &path, bool root)
{
	qDebug() << "making directory" << path;
	QFileInfo fi(path);
	QString parentPath = fi.dir().path();
	if (_directories.empty())
		_directories[parentPath] = _model->parentObjectId();
	if (!root)
	{
		auto parent = _directories.find(parentPath);
		Q_ASSERT(parent != _directories.end());
		if (_model->parentObjectId() != parent.value())
			_model->setParent(parent.value());
	}
	try
	{
		mtp::u32 dirId = _model->createDirectory(fi.fileName());
		_directories[path] = dirId;
		_model->setParent(dirId);
	} catch(const std::exception &ex)
	{ qDebug() << "creating directory" << path << "failed: " << fromUtf8(ex.what()); return; }
}

CommandQueue::CommandQueue(MtpObjectsModel *model): _model(model), _completedFilesSize(0)
{
	connect(_model, SIGNAL(filePositionChanged(qint64,qint64)), this, SLOT(onFileProgress(qint64,qint64)));
	qDebug() << "upload worker started";
}

CommandQueue::~CommandQueue()
{
	qDebug() << "upload worker stopped";
}

void CommandQueue::execute(Command *ptr)
{
	std::unique_ptr<Command> cmd(ptr);
	cmd->execute(*this);
}

void CommandQueue::start(const QString &filename)
{
	emit started(filename);
}

void CommandQueue::finish()
{
	_model->moveToThread(QApplication::instance()->thread());
	emit finished();
	_completedFilesSize = 0;
	_directories.clear();
}

void CommandQueue::addProgress(qint64 fileSize)
{
	_completedFilesSize += fileSize;
	emit progress(_completedFilesSize);
}

void CommandQueue::onFileProgress(qint64 pos, qint64)
{
	//qDebug() << "on file progress " << _completedFilesSize << " " << pos;
	emit progress(_completedFilesSize + pos);
}
