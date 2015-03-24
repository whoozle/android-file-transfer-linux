#include "fileuploader.h"
#include "mtpobjectsmodel.h"
#include <QStringList>
#include <QFileInfo>
#include <QPropertyAnimation>
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
	_animation = new QPropertyAnimation(this, "uploadedBytes");
	connect(_animation, SIGNAL(valueChanged(QVariant)), SLOT(onValueChanged(QVariant)));

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

void FileUploader::onValueChanged(QVariant variant)
{
	setUploadedBytes(variant.toLongLong());
}

void FileUploader::onProgress(qlonglong size)
{
	qDebug() << "progress size" << size;
	_current += size;
	int duration = 1 + _animation->currentTime();
	qDebug() << "progress" << _current << _total << duration;

	_animation->stop();
	_animation->setEndValue(_current);
	_animation->setDuration(10000);
	_animation->start();

	setUploadedBytes(_current);
}

void FileUploader::setUploadedBytes(qlonglong bytes)
{
	qDebug() << "ub = " << bytes;
	_uploadedBytes = bytes;
	emit uploadProgress(1.0 * _uploadedBytes / _total);
}

void FileUploader::upload(const QStringList &files)
{
	_total = _current = 1;
	for(QString file : files)
	{
		QFileInfo fi(file);
		_total += fi.size();
	}

	_animation->setStartValue(0);
	_animation->setEndValue(_total);
	_animation->setDuration(60000);
	_animation->start();

	for(QString file : files)
	{
		emit uploadFile(file);
	}
}
