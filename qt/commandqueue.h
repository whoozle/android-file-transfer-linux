#ifndef COMMANDQUEUE_H
#define COMMANDQUEUE_H

#include <QObject>
#include <QQueue>
#include <QMap>
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
	bool					Root;
	MakeDirectory(const QString &filename, bool root = false) :
		FileCommand(filename), Root(root) { }
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
	MtpObjectsModel *			_model;
	qint64						_completedFilesSize;
	QString						_root;
	QMap<QString, quint32>		_directories;

public:
	CommandQueue(MtpObjectsModel *model);
	~CommandQueue();

	MtpObjectsModel *model() const
	{ return _model; }

	void createDirectory(const QString &path, bool root);
	void uploadFile(const QString &file);

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
