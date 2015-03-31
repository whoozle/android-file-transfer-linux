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
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "createdirectorydialog.h"
#include "progressdialog.h"
#include "renamedialog.h"
#include "mtpobjectsmodel.h"
#include "fileuploader.h"
#include <QDebug>
#include <QMessageBox>
#include <QKeyEvent>
#include <QFileDialog>
#include <QSettings>
#include <QDir>

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	_ui(new Ui::MainWindow),
	_objectModel(new MtpObjectsModel(this)),
	_uploader(new FileUploader(_objectModel, this))
{
	_ui->setupUi(this);
	connect(_ui->listView, SIGNAL(doubleClicked(QModelIndex)), SLOT(onActivated(QModelIndex)));
	connect(_ui->listView, SIGNAL(customContextMenuRequested(QPoint)), SLOT(showContextMenu(QPoint)));
	connect(_ui->actionBack, SIGNAL(triggered()), SLOT(back()));
	connect(_ui->actionGo_Down, SIGNAL(triggered()), SLOT(down()));
	connect(_ui->actionCreateDirectory, SIGNAL(triggered()), SLOT(createDirectory()));
	connect(_ui->actionUploadDirectory, SIGNAL(triggered()), SLOT(uploadDirectories()));
	connect(_ui->actionUpload_Album, SIGNAL(triggered()), SLOT(uploadAlbum()));
	connect(_ui->actionUpload, SIGNAL(triggered()), SLOT(uploadFiles()));
}

MainWindow::~MainWindow()
{
	delete _ui;
}

void MainWindow::showEvent(QShowEvent *)
{
	if (!_device)
	{
		_device = mtp::Device::Find();
		if (!_device)
		{
			QMessageBox::critical(this, tr("No MTP device found"), tr("No MTP device found"));
			return;
		}
		_objectModel->setSession(_device->OpenSession(1));
		_ui->listView->setModel(_objectModel);
	}
}

void MainWindow::onActivated ( const QModelIndex & index )
{
	if (_objectModel->enter(index.row()))
		_history.push_back(_objectModel->parentObjectId());
}

void MainWindow::showContextMenu ( const QPoint & pos )
{
	QItemSelectionModel *selection =_ui->listView->selectionModel();
	QModelIndexList rows = selection->selectedRows();

	QMenu menu(this);
	//http://standards.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html
	QAction * download_objects = menu.addAction("Download");
	QAction * rename_object = menu.addAction("Rename");
	rename_object->setEnabled(rows.size() == 1);
	QAction * delete_objects = menu.addAction(QIcon::fromTheme("edit-delete"), "Delete");
	QAction * action = menu.exec(_ui->listView->mapToGlobal(pos));
	if (!action)
		return;

	if (action == download_objects)
	{
		QList<quint32> objects;
		for(int i = 0; i < rows.size(); ++i)
		{
			QModelIndex row = rows[i];
			objects.push_back(_objectModel->objectIdAt(row.row()));
		}
		downloadFiles(objects);
	}
	else
	{
		for(int i = rows.size() - 1; i >= 0; --i)
		{
			QModelIndex row = rows[i];
			if (action == delete_objects)
				_objectModel->removeRow(row.row());
			else if (action == rename_object)
			{
				RenameDialog d(_objectModel->data(row).toString(), this);
				int r = d.exec();
				if (r)
					_objectModel->rename(row.row(), d.name());
			}
			else
				qDebug() << "unknown action!";
		}
	}
}

void MainWindow::back()
{
	if (_history.empty())
		return;
	_history.pop_back();
	mtp::u32 oid = _history.empty()? mtp::Session::Root: _history.back();
	_objectModel->setParent(oid);
}

void MainWindow::down()
{
	if (_objectModel->enter(_ui->listView->currentIndex().row()))
		_history.push_back(_objectModel->parentObjectId());
}

void MainWindow::createDirectory()
{
	CreateDirectoryDialog d(this);
	if (d.exec() && !d.name().isEmpty())
		_objectModel->createDirectory(d.name(), false);
}

void MainWindow::uploadFiles(const QStringList &files)
{
	if (files.isEmpty())
		return;

	qDebug() << "uploadFiles " << files;
	unsigned limit = 0;
	//temp workaround kernel/libusb bug
	{
		QFile f("/sys/module/usbcore/parameters/usbfs_memory_mb");
		if (f.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			QTextStream stream(&f);
			stream >> limit;
			qDebug() << "usbfs limit: " << limit << "Mib";
		}
		else
			qDebug() << "cannot read limit on usbfs transaction";
	}
	unsigned max = 0;
	for(QString file : files)
	{
		QFileInfo fi(file);
		unsigned mb = (fi.size() + 1024 * 1024 - 1) / 1024 / 1024;
		if (mb > max)
			max = mb;
	}

	if (limit && max > limit)
	{
		QMessageBox::warning(this, "Error", "This file(s) will trigger the bug in libusb-1.0.\n"
			"Please increase usbfs memory usage limit with the following command as root:\n"
			"echo -n " + QString::number(max * 3 / 2) + " > /sys/module/usbcore/parameters/usbfs_memory_mb\n"
			"and then, try again"
		);
		return;
	}
	ProgressDialog progressDialog(this);
	progressDialog.setModal(true);
	progressDialog.setValue(0);

	connect(_uploader, SIGNAL(uploadProgress(float)), &progressDialog, SLOT(setValue(float)));
	connect(_uploader, SIGNAL(uploadStarted(QString)), &progressDialog, SLOT(setFilename(QString)));
	connect(_uploader, SIGNAL(finished()), &progressDialog, SLOT(accept()));
	_uploader->upload(files);

	progressDialog.exec();
}


void MainWindow::downloadFiles(const QList<quint32> &objects)
{
	if (objects.isEmpty())
		return;

	QString path = QFileDialog::getExistingDirectory(this, tr("Enter destination directory"));
	if (path.isEmpty())
		return;

	qDebug() << "downloading to " << path;
	ProgressDialog progressDialog(this);
	progressDialog.show();

	connect(_uploader, SIGNAL(uploadProgress(float)), &progressDialog, SLOT(setValue(float)));
	connect(_uploader, SIGNAL(uploadStarted(QString)), &progressDialog, SLOT(setFilename(QString)));
	connect(_uploader, SIGNAL(finished()), &progressDialog, SLOT(accept()));
	_uploader->download(path, objects);

	progressDialog.exec();
}


void MainWindow::uploadFiles()
{
	QFileDialog d(this);

	QSettings settings;
	{
		QVariant ld = settings.value("the-latest-directory");
		if (ld.isValid())
			d.setDirectory(ld.toString());
	}

	d.setAcceptMode(QFileDialog::AcceptOpen);
	d.setFileMode(QFileDialog::ExistingFiles);
	d.setOption(QFileDialog::ShowDirsOnly, false);
	d.exec();

	settings.setValue("the-latest-directory", d.directory().absolutePath());

	uploadFiles(d.selectedFiles());
}

void MainWindow::uploadDirectories()
{
	QSettings settings;
	QString dirPath;
	{
		QVariant ld = settings.value("the-latest-directory");
		if (ld.isValid())
			dirPath = ld.toString();
	}

	dirPath = QFileDialog::getExistingDirectory(this, "Upload Directories", dirPath);
	if (dirPath.isEmpty())
		return;

	settings.setValue("the-latest-directory", dirPath);

	QDir dir(dirPath);
	qDebug() << "adding directory " << dir.dirName();
	mtp::u32 dirId = _objectModel->createDirectory(dir.dirName(), false);
	_objectModel->setParent(dirId);
	_history.push_back(dirId);

	QStringList files;
	for(QString file : dir.entryList(QDir::Files))
	{
		files.push_back(dir.canonicalPath() + "/" + file);
	}
	uploadFiles(files);
	back();
}

void MainWindow::uploadAlbum()
{
	QString dirPath;
	QSettings settings;
	{
		QVariant ld = settings.value("the-latest-directory");
		if (ld.isValid())
			dirPath = ld.toString();
	}

	dirPath = QFileDialog::getExistingDirectory(this, "Upload Album", dirPath);
	if (!dirPath.isEmpty()) {
		settings.setValue("the-latest-directory", dirPath);
		uploadAlbum(dirPath);
	}
	back();
}

namespace
{
	int GetScore(const QString &str_)
	{
		QString str = str_.toLower();
		int score = 0;
		if (str.contains("art"))
			score += 1;
		if (str.contains("album"))
			score += 1;
		if (str.contains("large"))
			score += 2;
		if (str.contains("small"))
			score += 1;
		if (str.contains("folder"))
			score += 1;
		return score;
	}

	bool HeuristicLess(const QString &s1, const QString &s2)
	{
		return GetScore(s1) > GetScore(s2);
	}
}

void MainWindow::uploadAlbum(QString dirPath)
{
	QDir dir(dirPath);
	qDebug() << "adding directory " << dir.dirName();

	mtp::u32 dirId = _objectModel->createDirectory(dir.dirName(), true);
	_objectModel->setParent(dirId);
	_history.push_back(dirId);

	QString cover, coverTarget;
	QStringList covers;
	{
		QStringList ext({"*.png", "*.jpg", "*.jpeg"});
		covers = dir.entryList(ext, QDir::Files);
		qSort(covers.begin(), covers.end(), &HeuristicLess);
		qDebug() << "covers" << covers;
		if (!covers.isEmpty())
			cover = covers.front();

		if (cover.endsWith(".png"))
			coverTarget = "albumart.png";
		else
			coverTarget = "albumart.jpg";

		qDebug() << "use " << cover << " as album art -> " << coverTarget;
	}

	_objectModel->uploadFile(dir.canonicalPath() + "/" + cover, coverTarget);

	QStringList files;
	for(QString file : dir.entryList(QDir::Files))
	{
		if (!covers.contains(file))
			files.push_back(dir.canonicalPath() + "/" + file);
	}
	uploadFiles(files);
}
