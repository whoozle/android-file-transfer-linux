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

#include "commandqueue.h"
#include "mtpobjectsmodel.h"
#include "utils.h"

#include <QFileInfo>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QApplication>

#include <mtp/metadata/Metadata.h>
#include <mtp/metadata/Library.h>
#include <mtp/scope_guard.h>

void FinishQueue::execute(CommandQueue &queue)
{ queue.finish(DirectoryId); }

void UploadFile::execute(CommandQueue &queue)
{ queue.uploadFile(Filename); }

void MakeDirectory::execute(CommandQueue &queue)
{ queue.createDirectory(Filename); }

void DownloadFile::execute(CommandQueue &queue)
{ queue.downloadFile(Filename, ObjectId); }

void ImportFile::execute(CommandQueue &queue)
{ queue.importFile(Filename); }

void CommandQueue::downloadFile(const QString &filename, mtp::ObjectId objectId)
{
	if (_aborted)
		return;
	qDebug() << "downloading " << objectId << "to" << filename;

	QFileInfo fi(filename);
	QDir().mkpath(fi.dir().path());
	start(fi.fileName());
	try
	{
		model()->downloadFile(filename, objectId);
	} catch(const std::exception &ex)
	{ qDebug() << "downloading file " << filename << " failed: " << fromUtf8(ex.what()); }

	addProgress(fi.size());
}

void CommandQueue::uploadFile(const QString &filename)
{
	if (_aborted)
		return;

	QFileInfo fi(filename);
	QString parentPath = fi.dir().path();

	qDebug() << "uploading file " << filename << ", parent: " << parentPath;

	if (_directories.empty())
	{
		qDebug() << "adding first parent path";
		_directories[parentPath] = _model->parentObjectId();
		qDebug() << "directories[0]: " << parentPath << " -> " << _model->parentObjectId().Id;
	}
	start(fi.fileName());
	auto parent = _directories.find(parentPath);
	if (parent == _directories.end())
	{
		qWarning() << "invalid parent " << parentPath;
		return;
	}
	try
	{
		if (_model->parentObjectId() != parent.value()) //needed for overwrite protection
			_model->setParent(parent.value());

		_model->uploadFile(parent.value(), filename);
	} catch(const std::exception &ex)
	{ qDebug() << "uploading file " << filename << " failed: " << fromUtf8(ex.what()); }

	addProgress(fi.size());
}

void CommandQueue::importFile(const QString &filename)
{
	if (_aborted || !_library)
		return;

	QFileInfo fi(filename);

	mtp::scope_guard r([this, &fi]() { addProgress(fi.size()); });
	std::string utfFilename = toUtf8(filename);
	mtp::ObjectFormat format = mtp::ObjectFormatFromFilename(utfFilename);

	if (!mtp::IsAudioFormat(format))
		return;

	auto metadata = mtp::Metadata::Read(utfFilename);
	if (!metadata)
		return;

	qDebug() << "import: " << filename <<
		", format: " << fromUtf8(mtp::ToString(format)) <<
		", artist: " << fromUtf8(metadata->Artist) <<
		", album: " << fromUtf8(metadata->Album) <<
		", title: " << fromUtf8(metadata->Title) <<
		", year: " << metadata->Year <<
		", track: " << metadata->Track <<
		", genre: " << fromUtf8(metadata->Genre) <<
		", size: " << fi.size();

	auto artist = _library->GetArtist(metadata->Artist);
	if (!artist)
		artist = _library->CreateArtist(metadata->Artist);
	if (!artist)
	{
		qDebug() << "can't create artist";
		return;
	}

	auto album = _library->GetAlbum(artist, metadata->Album);
	if (!album)
		album = _library->CreateAlbum(artist, metadata->Album, metadata->Year);
	if (!album)
	{
		qDebug() << "can't create album";
		return;
	}

	auto songId = _library->CreateTrack(
		artist, album,
		format,
		metadata->Title, metadata->Genre,
		metadata->Track, toUtf8(fi.fileName()), fi.size());

	_model->sendFile(filename);

	_library->AddTrack(album, songId);
}

void CommandQueue::createDirectory(const QString &srcPath)
{
	if (_aborted)
		return;

	QFileInfo fi(srcPath);
	QString parentPath = fi.dir().path();
	qDebug() << "making directory" << srcPath << ", parent: " << parentPath << ", dir: " << fi.fileName();
	if (_directories.empty())
	{
		qDebug() << "adding first parent path";
		_directories[parentPath] = _model->parentObjectId();
		qDebug() << "directories[0]: " << parentPath << " -> " << _model->parentObjectId().Id;
	}

	auto parent = _directories.find(parentPath);
	if (parent == _directories.end())
	{
		qWarning() << "invalid parent " << parentPath;
		return;
	}

	try
	{
		mtp::ObjectId dirId = _model->createDirectory(parent.value(), fi.fileName());
		_directories[srcPath] = dirId;
		qDebug() << "directories[]: " << srcPath << " -> " << dirId.Id;
	} catch(const std::exception &ex)
	{ qDebug() << "creating directory" << srcPath << "failed: " << fromUtf8(ex.what()); return; }
}

CommandQueue::CommandQueue(MtpObjectsModel *model): _model(model), _completedFilesSize(0), _aborted(false)
{
	connect(_model, SIGNAL(filePositionChanged(qint64,qint64)), this, SLOT(onFileProgress(qint64,qint64)));
	qDebug() << "upload worker started";
}

CommandQueue::~CommandQueue()
{
	qDebug() << "upload worker stopped";
}

void CommandQueue::setLibrary(const mtp::LibraryPtr library)
{ _library = library; }

void CommandQueue::execute(Command *ptr)
{
	std::unique_ptr<Command> cmd(ptr);
	cmd->execute(*this);
}

void CommandQueue::start(const QString &filename)
{
	emit started(filename);
}

void CommandQueue::finish(mtp::ObjectId directoryId)
{
	qDebug() << "finishing queue";
	try
	{
		model()->setParent(directoryId);
	} catch(const std::exception &ex)
	{ qDebug() << "finalizing commands failed: " << fromUtf8(ex.what()); }

	_model->moveToThread(QApplication::instance()->thread());
	_completedFilesSize = 0;
	_directories.clear();
	_aborted = false;
	emit finished();
}

void CommandQueue::abort()
{
	qDebug() << "aborting...";
	_aborted = true;
	_model->session()->AbortCurrentTransaction(6000);
	qDebug() << "sent abort request";
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
