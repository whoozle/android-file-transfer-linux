#ifndef DEVICESDIALOG_H
#define DEVICESDIALOG_H

#include <QDialog>

namespace Ui {
class DevicesDialog;
}

class DevicesDialog : public QDialog
{
	Q_OBJECT

public:
	explicit DevicesDialog(QWidget *parent = nullptr);
	~DevicesDialog();

private:
	Ui::DevicesDialog *ui;
};

#endif // DEVICESDIALOG_H
