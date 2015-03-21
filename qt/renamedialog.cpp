#include "renamedialog.h"
#include "ui_renamedialog.h"

RenameDialog::RenameDialog(QString name, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::RenameDialog)
{
	ui->setupUi(this);
	ui->lineEdit->setText(name);
}

QString RenameDialog::name() const
{ return ui->lineEdit->text(); }

RenameDialog::~RenameDialog()
{
	delete ui;
}
