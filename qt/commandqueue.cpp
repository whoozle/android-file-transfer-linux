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
	auto directory = _directories.find(fi.dir().path());
	Q_ASSERT(directory != _directories.end());
	_model->setParent(directory.value());
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
	if (!root)
	{
		auto parent = _directories.find(fi.dir().path());
		Q_ASSERT(parent != _directories.end());
		_model->setParent(parent.value());
	}
	try
	{
		mtp::u32 dirId = _model->createDirectory(fi.fileName());
		_directories[path] = dirId;
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
