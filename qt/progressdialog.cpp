#include "progressdialog.h"
#include "ui_progressdialog.h"
#include <QCloseEvent>

ProgressDialog::ProgressDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ProgressDialog)
{
	ui->setupUi(this);
	ui->progressBar->setMaximum(10000);
	ui->buttonBox->setEnabled(false);
}

ProgressDialog::~ProgressDialog()
{
	delete ui;
}

void ProgressDialog::setValue(float current)
{
	ui->progressBar->setValue(current * 10000);
	if (current >= 0.99)
		ui->buttonBox->setEnabled(true);
}

void ProgressDialog::closeEvent(QCloseEvent *event)
{
	event->ignore();
}
