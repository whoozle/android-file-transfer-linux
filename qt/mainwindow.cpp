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
#include "mtpstoragesmodel.h"
#include "fileuploader.h"
#include "utils.h"
#include <mtp/usb/TimeoutException.h>
#include <QDebug>
#include <QSortFilterProxyModel>
#include <QMessageBox>
#include <QKeyEvent>
#include <QFileDialog>
#include <QSettings>
#include <QDir>

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	_ui(new Ui::MainWindow),
	_proxyModel(new QSortFilterProxyModel),
	_storageModel(),
	_objectModel(new MtpObjectsModel()),
	_uploader(new FileUploader(_objectModel, this))
{
	_ui->setupUi(this);
	setWindowIcon(QIcon(":/icons/android-file-transfer.png"));

	_ui->listView->setModel(_proxyModel);

	_proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
	_proxyModel->sort(0);
	_proxyModel->setDynamicSortFilter(true);

	_objectModel->moveToThread(QApplication::instance()->thread());

	connect(_ui->listView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), SLOT(updateActionsState()));
	connect(_ui->listView, SIGNAL(doubleClicked(QModelIndex)), SLOT(onActivated(QModelIndex)));
	connect(_ui->listView, SIGNAL(customContextMenuRequested(QPoint)), SLOT(showContextMenu(QPoint)));
	connect(_ui->actionBack, SIGNAL(triggered()), SLOT(back()));
	connect(_ui->actionGo_Down, SIGNAL(triggered()), SLOT(down()));
	connect(_ui->actionCreateDirectory, SIGNAL(triggered()), SLOT(createDirectory()));
	connect(_ui->actionUploadDirectory, SIGNAL(triggered()), SLOT(uploadDirectories()));
	connect(_ui->actionUpload_Album, SIGNAL(triggered()), SLOT(uploadAlbum()));
	connect(_ui->actionUpload, SIGNAL(triggered()), SLOT(uploadFiles()));
	connect(_ui->actionRename, SIGNAL(triggered()), SLOT(renameFile()));
	connect(_ui->actionDownload, SIGNAL(triggered()), SLOT(downloadFiles()));
	connect(_ui->actionDelete, SIGNAL(triggered()), SLOT(deleteFiles()));
	connect(_ui->storageList, SIGNAL(activated(int)), SLOT(onStorageChanged(int)));

	connect(_objectModel, SIGNAL(onFilesDropped(QStringList)), SLOT(uploadFiles(QStringList)));
	connect(_objectModel, SIGNAL(existingFileOverwrite(QString)), SLOT(confirmOverwrite(QString)), Qt::BlockingQueuedConnection);

	//fixme: find out how to specify alternative in designer
	_ui->actionBack->setShortcuts(_ui->actionBack->shortcuts() << QKeySequence("Alt+Up") << QKeySequence("Esc"));
	_ui->actionGo_Down->setShortcuts(_ui->actionGo_Down->shortcuts() << QKeySequence("Alt+Down") << QKeySequence("Enter"));
	_ui->actionRename->setShortcuts(_ui->actionRename->shortcuts() << QKeySequence("F3"));
	_ui->actionCreateDirectory->setShortcuts(_ui->actionRename->shortcuts() << QKeySequence("F7"));
	_ui->listView->setFocus();
}

MainWindow::~MainWindow()
{
	_proxyModel->setSourceModel(NULL);
	delete _objectModel;
	delete _ui;
}

void MainWindow::saveGeometry(const QString &name, const QWidget &widget)
{
	QSettings settings;
	settings.setValue("geometry/" + name, widget.saveGeometry());
}

void MainWindow::restoreGeometry(const QString &name, QWidget &widget)
{
	QSettings settings;
	QVariant geometry = settings.value("geometry/" + name);
	if (geometry.isValid())
		widget.restoreGeometry(geometry.toByteArray());
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	QSettings settings;
	saveGeometry("main-window", *this);
	settings.setValue("state/main-window", saveState());
	QMainWindow::closeEvent(event);
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
		QSettings settings;
		restoreGeometry("main-window", *this);
		restoreState(settings.value("state/main-window").toByteArray());

		qDebug() << "device found, opening session...";
		mtp::SessionPtr session;
		static const int MaxAttempts = 3;
		for(int attempt = 0; attempt < MaxAttempts; ++attempt)
		{
			try
			{
				session = _device->OpenSession(1);
				mtp::msg::DeviceInfo di = session->GetDeviceInfo();
				qDebug() << "device info" << fromUtf8(di.Manufacturer) << " " << fromUtf8(di.Model);
				break;
			}
			catch(const mtp::usb::TimeoutException &ex)
			{
				qDebug() << "timed out getting device info: " << fromUtf8(ex.what()) << ", retrying...";
				if (attempt + 1 == MaxAttempts)
				{
					QMessageBox::critical(this, tr("MTP"), tr("MTP device does not respond"));
					_device.reset();
					return;
				}
			}
		}

		_storageModel = new MtpStoragesModel(this);
		_storageModel->update(session);
		_ui->storageList->setModel(_storageModel);
		_objectModel->setSession(session);
		onStorageChanged(_ui->storageList->currentIndex());
		qDebug() << "session opened, starting";
		_proxyModel->setSourceModel(_objectModel);
	}
}

QModelIndex MainWindow::mapIndex(const QModelIndex &index)
{
	return _proxyModel->mapToSource(index);
}

void MainWindow::onStorageChanged(int idx)
{
	if (!_storageModel)
		return;
	mtp::u32 storageId;
	storageId = _storageModel->getStorageId(idx);
	qDebug() << "switching to storage id " << storageId;
	_objectModel->setStorageId(storageId);
	_history.clear();
	updateActionsState();
}

void MainWindow::onActivated ( const QModelIndex & index )
{
	if (_objectModel->enter(mapIndex(index).row()))
		_history.push_back(_objectModel->parentObjectId());
	updateActionsState();
}

void MainWindow::updateActionsState()
{
	QModelIndexList rows = _ui->listView->selectionModel()->selectedRows();
	_ui->actionDelete->setEnabled(!rows.empty());
	_ui->actionDownload->setEnabled(!rows.empty());
	_ui->actionRename->setEnabled(rows.size() == 1);
	_ui->actionGo_Down->setEnabled(rows.size() == 1);
	_ui->actionBack->setEnabled(!_history.empty());
}

void MainWindow::downloadFiles()
{
	QItemSelectionModel *selection =_ui->listView->selectionModel();
	QModelIndexList rows = selection->selectedRows();

	QVector<quint32> objects;
	for(QModelIndex row : rows)
	{
		row = mapIndex(row);
		objects.push_back(_objectModel->objectIdAt(row.row()));
	}
	downloadFiles(objects);
}

void MainWindow::renameFile()
{
	QItemSelectionModel *selection =_ui->listView->selectionModel();
	QModelIndexList rows = selection->selectedRows();
	if (rows.empty())
		return;

	QModelIndex row = mapIndex(rows.at(0));
	RenameDialog d(_objectModel->data(row).toString(), this);
	restoreGeometry("rename-dialog", d);
	if (d.exec())
		_objectModel->rename(row.row(), d.name());
	saveGeometry("rename-dialog", d);
}

void MainWindow::deleteFiles()
{
	QItemSelectionModel *selection =_ui->listView->selectionModel();
	QModelIndexList rows = selection->selectedRows();
	int r = QMessageBox::question(this,
		tr("Deleting file(s)"),
		tr("Are you sure?"),
		QMessageBox::Yes | QMessageBox::No,
		QMessageBox::No
	);
	if (r != QMessageBox::Yes)
		return;

	MtpObjectList objects;
	for(QModelIndex row : rows)
	{
		row = mapIndex(row);
		objects.push_back(_objectModel->objectIdAt(row.row()));
	}
	_objectModel->deleteObjects(objects);
}


void MainWindow::showContextMenu ( const QPoint & pos )
{
	QItemSelectionModel *selection =_ui->listView->selectionModel();
	QModelIndexList rows = selection->selectedRows();

	QMenu menu(this);
	menu.addAction(_ui->actionCreateDirectory);
	menu.addAction(_ui->actionRename);
	menu.addAction(_ui->actionDownload);
	menu.addAction(_ui->actionDelete);
	menu.exec(_ui->listView->mapToGlobal(pos));
}

void MainWindow::back()
{
	if (_history.empty())
		return;
	mtp::u32 oldParent = _objectModel->parentObjectId();
	_history.pop_back();
	mtp::u32 oid = _history.empty()? mtp::Session::Root: _history.back();
	_objectModel->setParent(oid);
	QModelIndex prevIndex = _objectModel->findObject(oldParent);
	if (prevIndex.isValid())
		_ui->listView->setCurrentIndex(_proxyModel->mapFromSource(prevIndex));
	updateActionsState();
}

void MainWindow::down()
{
	if (_objectModel->enter(mapIndex(_ui->listView->currentIndex()).row()))
		_history.push_back(_objectModel->parentObjectId());
	updateActionsState();
}

void MainWindow::createDirectory()
{
	CreateDirectoryDialog d(this);
	restoreGeometry("create-directory-dialog", d);
	if (d.exec() && !d.name().isEmpty())
		_objectModel->createDirectory(d.name(), mtp::AssociationType::GenericFolder);
	saveGeometry("create-directory-dialog", d);
}

void MainWindow::uploadFiles(const QStringList &files)
{
	if (files.isEmpty())
		return;

	qDebug() << "uploadFiles " << files;
	_uploadAnswer = 0;
	_proxyModel->setSourceModel(NULL);
	ProgressDialog progressDialog(this);
	progressDialog.setModal(true);
	progressDialog.setValue(0);

	connect(_uploader, SIGNAL(uploadProgress(float)), &progressDialog, SLOT(setValue(float)));
	connect(_uploader, SIGNAL(uploadSpeed(qint64)), &progressDialog, SLOT(setSpeed(qint64)));
	connect(_uploader, SIGNAL(uploadStarted(QString)), &progressDialog, SLOT(setFilename(QString)));
	connect(_uploader, SIGNAL(finished()), &progressDialog, SLOT(accept()));
	_uploader->upload(files);

	progressDialog.exec();

	_objectModel->moveToThread(QApplication::instance()->thread());
	_proxyModel->setSourceModel(_objectModel);
}


void MainWindow::downloadFiles(const QVector<quint32> &objects)
{
	if (objects.isEmpty())
		return;

	QFileDialog d(this);
	QSettings settings;
	{
		QVariant ld = settings.value("the-latest-download-directory");
		if (ld.isValid())
			d.setDirectory(ld.toString());
	}

	d.setAcceptMode(QFileDialog::AcceptSave);
	d.setFileMode(QFileDialog::Directory);
	d.setOption(QFileDialog::ShowDirsOnly, true);
	restoreGeometry("download-files", d);
	if (!d.exec())
		return;

	QString path = d.directory().path();
	if (path.isEmpty())
		return;

	saveGeometry("download-files", d);
	settings.setValue("the-latest-download-directory", path);
	downloadFiles(path, objects);
}

void MainWindow::downloadFiles(const QString & path, const QVector<quint32> &objects)
{
	qDebug() << "downloading to " << path;
	ProgressDialog progressDialog(this);
	progressDialog.show();

	connect(_uploader, SIGNAL(uploadProgress(float)), &progressDialog, SLOT(setValue(float)));
	connect(_uploader, SIGNAL(uploadSpeed(qint64)), &progressDialog, SLOT(setSpeed(qint64)));
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
	restoreGeometry("upload-files", d);
	if (!d.exec())
		return;

	saveGeometry("upload-files", d);
	settings.setValue("the-latest-directory", d.directory().absolutePath());
	uploadFiles(d.selectedFiles());
}

void MainWindow::uploadDirectories()
{
	QSettings settings;
	QFileDialog d(this);
	{
		QVariant ld = settings.value("the-latest-directory");
		if (ld.isValid())
			d.setDirectory(ld.toString());
	}

	d.setAcceptMode(QFileDialog::AcceptOpen);
	d.setFileMode(QFileDialog::Directory);
	d.setOption(QFileDialog::ShowDirsOnly, true);
	restoreGeometry("upload-directories", d);
	if (!d.exec())
		return;

	saveGeometry("upload-directories", d);
	settings.setValue("the-latest-directory", d.directory().absolutePath());

	QStringList dirs(d.selectedFiles());
	if (dirs.isEmpty())
		return;

	uploadFiles(dirs);
}

void MainWindow::uploadAlbum()
{
	QFileDialog d(this);
	QSettings settings;
	{
		QVariant ld = settings.value("the-latest-directory");
		if (ld.isValid())
			d.setDirectory(ld.toString());
	}

	d.setAcceptMode(QFileDialog::AcceptOpen);
	d.setFileMode(QFileDialog::Directory);
	d.setOption(QFileDialog::ShowDirsOnly, true);
	restoreGeometry("upload-albums", d);
	if (!d.exec())
		return;

	saveGeometry("upload-albums", d);
	settings.setValue("the-latest-directory", d.directory().absolutePath());
	uploadAlbum(d.directory().absolutePath());
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
		if (str.contains("cover"))
			score += 2;
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

	mtp::u32 dirId = _objectModel->createDirectory(dir.dirName(), mtp::AssociationType::Album);
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
	for(QString file : dir.entryList(QDir::Dirs | QDir::Files | QDir::NoDot | QDir::NoDotDot))
	{
		if (!covers.contains(file))
			files.push_back(dir.canonicalPath() + "/" + file);
	}
	uploadFiles(files);
}

bool MainWindow::confirmOverwrite(const QString &file)
{
	int b = _uploadAnswer? _uploadAnswer: QMessageBox::question(this, tr("Overwrite confirmation"), tr("You are about to overwrite file ") + file, QMessageBox::Yes | QMessageBox::YesToAll | QMessageBox::No | QMessageBox::NoToAll);
	switch(b)
	{
	case QMessageBox::YesToAll:
		_uploadAnswer = QMessageBox::Yes;
	case QMessageBox::Yes:
		return true;
	case QMessageBox::NoToAll:
		_uploadAnswer = QMessageBox::No;
	case QMessageBox::No:
		return false;
	default:
		return false;
	}
}
