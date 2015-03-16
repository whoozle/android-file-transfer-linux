#include "createdirectorydialog.h"
#include "ui_createdirectorydialog.h"

CreateDirectoryDialog::CreateDirectoryDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::CreateDirectoryDialog)
{
	ui->setupUi(this);
}

CreateDirectoryDialog::~CreateDirectoryDialog()
{
	delete ui;
}

QString CreateDirectoryDialog::name() const
{
	return ui->lineEdit->text();
}
