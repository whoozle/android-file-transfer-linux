#ifndef COMMANDQUEUE_H
#define COMMANDQUEUE_H

#include <QObject>
#include <QQueue>
#include <mtp/ptp/ObjectFormat.h>

class MtpObjectsModel;
class CommandQueue;

struct Command
{
	virtual ~Command() { }
	virtual void execute(CommandQueue &queue) = 0;
};

struct FinishQueue : public Command
{
	virtual void execute(CommandQueue &queue);
};

struct FileCommand : public Command
{
	QString		Filename;

	FileCommand(const QString &filename) : Filename(filename) { }
};

struct MakeDirectory : public FileCommand
{
	quint32					ParentId;
	quint32					StorageId;
	mtp::AssociationType	Type;
	MakeDirectory(const QString &filename, quint32 parentId, quint32 storageId, mtp::AssociationType type) : FileCommand(filename), ParentId(parentId), StorageId(storageId), Type(type) { }
	void execute(CommandQueue &queue);
};

struct UploadFile : public FileCommand
{
	UploadFile(const QString &filename) : FileCommand(filename) { }
	void execute(CommandQueue &queue);
};

struct DownloadFile : public FileCommand
{
	quint32			ObjectId;

	DownloadFile(const QString &filename, quint32 objectId) : FileCommand(filename), ObjectId(objectId) { }
	void execute(CommandQueue &queue);
};

class CommandQueue: public QObject
{
	Q_OBJECT

private:
	MtpObjectsModel *	_model;
	qint64				_completedFilesSize;

public:
	CommandQueue(MtpObjectsModel *model);
	~CommandQueue();

	MtpObjectsModel *model() const
	{ return _model; }

public slots:
	void onFileProgress(qint64, qint64);
	void execute(Command *cmd);
	void start(const QString &filename);
	void finish();
	void addProgress(qint64);

signals:
	void started(QString);
	void progress(qint64 bytes);
	void finished();
};

#endif // COMMANDQUEUE_H
