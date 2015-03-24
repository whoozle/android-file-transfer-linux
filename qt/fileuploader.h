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

public:
	FileUploaderWorker(MtpObjectsModel *model);
	~FileUploaderWorker();

public slots:
	void uploadFile(const QString &file);

signals:
	void progress(qlonglong bytes);
};

class FileUploader : public QObject
{
	Q_OBJECT

private:
	MtpObjectsModel	*	_model;
	QThread				_workerThread;
	qlonglong			_total, _current;

private slots:
	void onProgress(qlonglong size);

public:
	explicit FileUploader(MtpObjectsModel * model, QObject *parent = 0);
	~FileUploader();

	void upload(const QStringList &files);

signals:
	void uploadFile(QString file);
	void uploadProgress(float);
	void finished();
};

#endif // FILEUPLOADER_H
