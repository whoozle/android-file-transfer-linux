#include "progressdialog.h"
#include "ui_progressdialog.h"
#include <QCloseEvent>
#include <QPropertyAnimation>
#include <QDebug>

ProgressDialog::ProgressDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ProgressDialog), _progress(0), _duration(0)
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

void ProgressDialog::reject()
{ }

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
	_duration += _animation->currentTime();
	float currentAnimated = _animation->currentValue().toFloat();
	if (_duration <= 0)
		return;
	float currentSpeed = current * 1000 / _duration;
	int estimate = 1000 / currentSpeed;
	//qDebug() << current << currentSpeed << estimate;
	_animation->stop();
	_animation->setStartValue(currentAnimated);
	_animation->setDuration(estimate - _duration);
	_animation->start();
}

void ProgressDialog::setFilename(const QString &filename)
{
	ui->label->setText(filename);
}

void ProgressDialog::closeEvent(QCloseEvent *event)
{
	event->ignore();
}
