#include "fileuploader.h"
#include "mtpobjectsmodel.h"
#include <QStringList>
#include <QFileInfo>
#include <QDebug>

#include <unistd.h>

FileUploaderWorker::FileUploaderWorker(MtpObjectsModel *model): _model(model)
{
	qDebug() << "upload worker started";
}

FileUploaderWorker::~FileUploaderWorker()
{
	qDebug() << "upload worker stopped";
}

void FileUploaderWorker::uploadFile(const QString &file)
{
	qDebug() << "uploading file " << file;
	QFileInfo fi(file);
	try
	{
		//_model->uploadFile(file)
		sleep(1);
	} catch(const std::exception &ex)
	{ qDebug() << "uploading file " << file << " failed: " << ex.what(); }

	emit progress(fi.size());
}

FileUploader::FileUploader(MtpObjectsModel * model, QObject *parent) :
	QObject(parent),
	_model(model)
{
	FileUploaderWorker * worker = new FileUploaderWorker(_model);
	worker->moveToThread(&_workerThread);

	connect(&_workerThread, SIGNAL(finished()), SLOT(deleteLater()));
	connect(this, SIGNAL(uploadFile(QString)), worker, SLOT(uploadFile(QString)));
	connect(worker, SIGNAL(progress(qlonglong)), SLOT(onProgress(qlonglong)));
	_workerThread.start();
}

FileUploader::~FileUploader()
{
	_workerThread.quit();
	_workerThread.wait();
}

void FileUploader::onProgress(qlonglong size)
{
	qDebug() << "progress size" << size;
	_current += size;
	emit uploadProgress(1.0 * _current / _total);
}

void FileUploader::upload(const QStringList &files)
{
	_total = _current = 1;
	for(QString file : files)
	{
		QFileInfo fi(file);
		_total += fi.size();
	}

	for(QString file : files)
	{
		emit uploadFile(file);
	}
}
