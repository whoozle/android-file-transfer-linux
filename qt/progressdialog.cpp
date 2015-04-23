/*
 * Android File Transfer for Linux: MTP client for android devices
 * Copyright (C) 2015  Vladimir Menshakov

 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
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
	if (current < 0)
		current = 0;
	if (current > 1)
		current = 1;
	ui->progressBar->setValue(current * 10000);
}

void ProgressDialog::setValue(float current)
{
	//qDebug() << "setValue " << current;
	_duration += _animation->currentTime();
	float currentAnimated = _animation->currentValue().toFloat();
	if (_duration <= 0)
		return;
	float currentSpeed = current * 1000 / _duration;
	int estimate = 1000 / currentSpeed;
	int duration = estimate - _duration;
	if (duration < 100)
		duration = 100;
	//qDebug() << current << currentSpeed << estimate;
	_animation->stop();
	_animation->setStartValue(currentAnimated);
	_animation->setDuration(duration);
	_animation->start();
}

void ProgressDialog::setSpeed(qint64 speed)
{
	static const double Kb = 1000;
	static const double Mb = 1000 * Kb; //haha
	static const double Gb = 1000 * Mb;
	if (speed < 2 * Mb)
		ui->speedLabel->setText(tr("Speed: ") + QString().sprintf("%.1f", speed / Kb) + tr(" Kb/s"));
	else if (speed < 2 * Gb)
		ui->speedLabel->setText(tr("Speed: ") + QString().sprintf("%.1f", speed / Mb) + tr(" Mb/s"));
	else
		ui->speedLabel->setText(tr("Speed: ") + QString().sprintf("%.1f", speed / Gb) + tr(" Gb/s"));
}

void ProgressDialog::setFilename(const QString &filename)
{
	ui->fileLabel->setText(filename);
}

void ProgressDialog::closeEvent(QCloseEvent *event)
{
	event->ignore();
}
