#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H

#include <QDialog>

namespace Ui {
class ProgressDialog;
}

class QPropertyAnimation;

class ProgressDialog : public QDialog
{
	Q_OBJECT
	Q_PROPERTY(float progress READ progress WRITE setProgress);

public:
	explicit ProgressDialog(QWidget *parent = 0);
	~ProgressDialog();

	float progress() const
	{ return _progress; }

	void setProgress(float value);

public slots:
	void setFilename(const QString &filename);
	void setValue(float current);
	virtual void reject();

private:
	void closeEvent(QCloseEvent *event);

private:
	Ui::ProgressDialog *ui;
	QPropertyAnimation *_animation;
	float				_progress;
	int					_duration;
};

#endif // PROGRESSDIALOG_H
