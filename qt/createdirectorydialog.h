#ifndef CREATEDIRECTORYDIALOG_H
#define CREATEDIRECTORYDIALOG_H

#include <QDialog>

namespace Ui {
class CreateDirectoryDialog;
}

class CreateDirectoryDialog : public QDialog
{
	Q_OBJECT

public:
	explicit CreateDirectoryDialog(QWidget *parent = 0);
	~CreateDirectoryDialog();

	QString name() const;

private:
	Ui::CreateDirectoryDialog *ui;
};

#endif // CREATEDIRECTORYDIALOG_H
