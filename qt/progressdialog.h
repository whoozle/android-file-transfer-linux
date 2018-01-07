/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2018  Vladimir Menshakov

    Android File Transfer For Linux is free software: you can redistribute
    it and/or modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    Android File Transfer For Linux is distributed in the hope that it will
    be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Android File Transfer For Linux.
    If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H

#include <QDialog>

namespace Ui {
class ProgressDialog;
}

class QPropertyAnimation;

class ProgressDialog : public QDialog
{
	Q_OBJECT
	Q_PROPERTY(float progress READ progress WRITE setProgress);

public:
	explicit ProgressDialog(QWidget *parent = 0);
	~ProgressDialog();

	float progress() const
	{ return _progress; }

	void setProgress(float value);

signals:
	void abort();

public slots:
	void setSpeed(qint64 speed);
	void setFilename(const QString &filename);
	void setValue(float current);
	virtual void reject();

private slots:
	void onAbortButtonPressed();

private:
	void closeEvent(QCloseEvent *event);

private:
	Ui::ProgressDialog *ui;
	QPropertyAnimation *_animation;
	float				_progress;
	int					_duration;
};

#endif // PROGRESSDIALOG_H
