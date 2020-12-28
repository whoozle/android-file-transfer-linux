#include "devicesdialog.h"
#include "ui_devicesdialog.h"

DevicesDialog::DevicesDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::DevicesDialog)
{
	ui->setupUi(this);
}

DevicesDialog::~DevicesDialog()
{
	delete ui;
}
