#include "commandqueue.h"
#include "mtpobjectsmodel.h"
#include <QFileInfo>
#include <QDebug>
#include <QApplication>


CommandQueue::CommandQueue(MtpObjectsModel *model): _model(model), _completedFilesSize(0)
{
	connect(_model, SIGNAL(filePositionChanged(qint64,qint64)), this, SLOT(onFileProgress(qint64,qint64)));
	qDebug() << "upload worker started";
}

CommandQueue::~CommandQueue()
{
	qDebug() << "upload worker stopped";
}

void CommandQueue::uploadFile(const QString &file)
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

void CommandQueue::downloadFile(const QString &file, quint32 objectId)
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

void CommandQueue::onFileProgress(qint64 pos, qint64)
{
	//qDebug() << "on file progress " << _completedFilesSize << " " << pos;
	emit progress(_completedFilesSize + pos);
}
