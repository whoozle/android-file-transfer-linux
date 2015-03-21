#ifndef RENAMEDIALOG_H
#define RENAMEDIALOG_H

#include <QDialog>

namespace Ui {
class RenameDialog;
}

class RenameDialog : public QDialog
{
	Q_OBJECT

public:
	explicit RenameDialog(QString name, QWidget *parent = 0);
	~RenameDialog();

	QString name() const;

private:
	Ui::RenameDialog *ui;
};

#endif // RENAMEDIALOG_H
