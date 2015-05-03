#ifndef COMMANDQUEUE_H
#define COMMANDQUEUE_H

#include <QObject>

class MtpObjectsModel;

class CommandQueue: public QObject
{
	Q_OBJECT

private:
	MtpObjectsModel *	_model;
	qint64				_completedFilesSize;

public:
	CommandQueue(MtpObjectsModel *model);
	~CommandQueue();

public slots:
	void onFileProgress(qint64, qint64);
	void uploadFile(const QString &file);
	void downloadFile(const QString &path, quint32 objectId);

signals:
	void started(QString);
	void progress(qint64 bytes);
	void finished();
};

#endif // COMMANDQUEUE_H
