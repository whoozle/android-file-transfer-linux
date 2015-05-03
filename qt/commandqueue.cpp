#include "commandqueue.h"
#include "mtpobjectsmodel.h"
#include <QFileInfo>
#include <QDebug>
#include <QApplication>


void FileCommand::execute(CommandQueue &queue)
{
	QFileInfo fi(Filename);
	emit queue.started(fi.fileName());
}

void FinishQueue::execute(CommandQueue &queue)
{
	queue.finish();
}

void UploadFile::execute(CommandQueue &queue)
{
	qDebug() << "uploading file " << Filename;
	FileCommand::execute(queue);
	QFileInfo fi(Filename);
	queue.start(fi.fileName());
	try
	{
		queue.model()->uploadFile(fi.fileName());
	} catch(const std::exception &ex)
	{ qDebug() << "uploading file " << Filename << " failed: " << ex.what(); }

	queue.addProgress(fi.size());
}

void DownloadFile::execute(CommandQueue &queue)
{
	FileCommand::execute(queue);

	qDebug() << "downloading " << ObjectId << "to" << Filename;

	QFileInfo fi(Filename);
	queue.start(fi.fileName());
	try
	{
		queue.model()->downloadFile(Filename, ObjectId);
	} catch(const std::exception &ex)
	{ qDebug() << "downloading file " << Filename << " failed: " << ex.what(); }

	queue.addProgress(fi.size());
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
