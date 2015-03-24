#include "progressdialog.h"
#include "ui_progressdialog.h"
#include <QCloseEvent>
#include <QPropertyAnimation>
#include <QDebug>

ProgressDialog::ProgressDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ProgressDialog), _progress(0)
{
	ui->setupUi(this);
	ui->progressBar->setMaximum(10000);
	ui->buttonBox->setEnabled(false);

	_animation = new QPropertyAnimation(this, "progress", this);
	_animation->setDuration(30000);
	_animation->setStartValue(0);
	_animation->setEndValue(1);
	_animation->start();
}

ProgressDialog::~ProgressDialog()
{
	delete ui;
}

void ProgressDialog::setProgress(float current)
{
	ui->progressBar->setValue(current * 10000);
	if (current >= 0.99)
	{
		ui->buttonBox->setEnabled(true);
		accept();
	}
}

void ProgressDialog::setValue(float current)
{
	int duration = _animation->currentTime();
	if (duration <= 0)
		return;
	float currentSpeed = current * 1000 / duration;
	int estimate = 1000 / currentSpeed;
	//qDebug() << current << currentSpeed << estimate;
	_animation->setDuration(estimate);
}

void ProgressDialog::closeEvent(QCloseEvent *event)
{
	event->ignore();
}
