/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2018  Vladimir Menshakov

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License,
    or (at your option) any later version.

    This library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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
#include <usb/Context.h>
#include <mtp/usb/TimeoutException.h>
#include <mtp/usb/DeviceBusyException.h>
#include <mtp/usb/DeviceNotFoundException.h>
#include <QClipboard>
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
	_clipboard(QApplication::clipboard()),
	_proxyModel(new QSortFilterProxyModel),
	_storageModel(),
	_objectModel(new MtpObjectsModel()),
	_uploader(new FileUploader(_objectModel, this))
{
	_ui->setupUi(this);
	setWindowIcon(QIcon(":/android-file-transfer.png"));

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
	connect(_ui->actionRefresh, SIGNAL(triggered()), SLOT(refresh()));
	connect(_ui->actionPaste, SIGNAL(triggered()), SLOT(pasteFromClipboard()));
	connect(_ui->actionShowThumbnails, SIGNAL(triggered(bool)), SLOT(showThumbnails(bool)));

	connect(_objectModel, SIGNAL(onFilesDropped(QStringList)), SLOT(uploadFiles(QStringList)));
	connect(_objectModel, SIGNAL(existingFileOverwrite(QString)), SLOT(confirmOverwrite(QString)), Qt::BlockingQueuedConnection);

	connect(_clipboard, SIGNAL(dataChanged()), SLOT(validateClipboard()));
	validateClipboard();

	//fixme: find out how to specify alternative in designer
	_ui->actionBack->setShortcuts(_ui->actionBack->shortcuts() << QKeySequence("Alt+Up") << QKeySequence("Esc"));
	_ui->actionGo_Down->setShortcuts(_ui->actionGo_Down->shortcuts() << QKeySequence("Alt+Down") << QKeySequence("Enter"));
	_ui->actionCreateDirectory->setShortcuts(_ui->actionCreateDirectory->shortcuts() << QKeySequence("F7"));
	_ui->actionRefresh->setShortcuts(_ui->actionRefresh->shortcuts() << QKeySequence("Ctrl+R"));
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

bool MainWindow::reconnectToDevice()
{
	_session.reset();
	_device.reset();
	bool claimInterface = true;
	bool resetDevice = false;

	mtp::usb::ContextPtr ctx(new mtp::usb::Context);

	auto devices = ctx->GetDevices();
	for (auto desc = devices.begin(); desc != devices.end();)
	{
		qDebug("probing device...");
		try
		{
			_device = mtp::Device::Open(ctx, *desc, claimInterface, resetDevice);
			++desc;
		}
		catch(const mtp::usb::DeviceBusyException &ex)
		{
			bool canKill = !ex.Processes.empty();
			QString processList;
			for (auto & desc : ex.Processes)
			{
				processList += QString("%1 (pid: %2)\n").arg(fromUtf8(desc.Name)).arg(desc.Id);
			}
			if (canKill)
				processList = tr("The following processes are keeping file descriptors for your device:\n") + processList;

			QMessageBox dialog(
				QMessageBox::Warning,
				tr("Device is busy"),
				tr("Device is busy, maybe another process is using it.\n\n") +
				processList +
				tr("Close other MTP applications and restart Android File Transfer.\n"
				"\nPress Abort to kill them or Ignore to try next device."),
				(canKill? QMessageBox::Abort: QMessageBox::StandardButton(0)) | QMessageBox::Ignore,
				this
			);

			dialog.setDefaultButton(QMessageBox::Ignore);
			dialog.setEscapeButton(QMessageBox::Ignore);
			auto r = dialog.exec();

			if ((r & QMessageBox::Abort) == QMessageBox::Abort)
			{
				qDebug("kill'em all");
				ex.Kill();
				qDebug("retrying..."); //do not increment desc, retry device
			}
			if ((r & QMessageBox::Ignore) == QMessageBox::Ignore)
				++desc;
		}
		catch(const std::exception &ex)
		{
			qWarning("Device::Find failed: %s", ex.what());
			QMessageBox dialog(
				QMessageBox::Warning,
				tr("Device::Find failed"),
				tr(
					"MTP device could not be opened at the moment\n\nFailure: %1\n"
					"Press Reset to reset them or Ignore to try next device.").arg(fromUtf8(ex.what())),
				QMessageBox::Reset | QMessageBox::Ignore,
				this
			);
			dialog.setDefaultButton(QMessageBox::Ignore);
			dialog.setEscapeButton(QMessageBox::Ignore);
			auto r = dialog.exec();

			if ((r & QMessageBox::Reset) == QMessageBox::Reset)
			{
				qDebug("retry with reset...");
				resetDevice = true;
			}
			if ((r & QMessageBox::Ignore) == QMessageBox::Ignore)
				++desc;
		}

		if (_device)
			break;
	}

	if (!_device)
	{
		QMessageBox::critical(this, tr("No MTP device found"), tr("No MTP device found"));
		return false;
	}

	qDebug() << "device found, opening session...";
	static const int MaxAttempts = 3;
	QString error;
	for(int attempt = 0; attempt < MaxAttempts; ++attempt)
	{
		try
		{
			_session = _device->OpenSession(1);
			mtp::msg::DeviceInfo di = _session->GetDeviceInfo();
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
				return false;
			}
		}
		catch(const mtp::usb::DeviceNotFoundException &ex)
		{
			qDebug() << "device disconnected, retrying...";
		}
		catch(const std::exception &ex)
		{
			error = fromUtf8(ex.what());
			qWarning() << "open session/device info failed: " << error;
		}
	}

	if (!_session)
	{
		_device.reset();
		QMessageBox::critical(this, tr("MTP"), tr("Could not open MTP session: %1").arg(error));
		return false;
	}

	return true;
}


void MainWindow::showEvent(QShowEvent *)
{
	if (!_device)
	{
		if (!reconnectToDevice())
			return;

		QSettings settings;
		restoreGeometry("main-window", *this);
		restoreState(settings.value("state/main-window").toByteArray());

		_storageModel = new MtpStoragesModel(this);
		while (true)
		{
			try
			{
				if (_storageModel->update(_session))
					break;
			}
			catch(const mtp::usb::DeviceNotFoundException &ex)
			{
				qDebug() << "device disconnected, retrying...";
				if (!reconnectToDevice())
					return;
			}

			int r = QMessageBox::warning(this, tr("No MTP Storages"),
				tr("No MTP storage found, your device might be locked.\nPlease unlock and press Retry to continue or Abort to exit."),
				QMessageBox::Retry | QMessageBox::Abort);

			if (r & QMessageBox::Abort)
			{
				_device.reset();
				return;
			}

			if (!reconnectToDevice())
				return;
		}
		_ui->storageList->setModel(_storageModel);
		_objectModel->setSession(_session);
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
	mtp::StorageId storageId = _storageModel->getStorageId(idx);
	qDebug() << "switching to storage id " << storageId.Id;
	_objectModel->setStorageId(storageId);
	_history.clear();
	updateActionsState();
}

void MainWindow::down()
{
	QModelIndex index = mapIndex(_ui->listView->currentIndex());
	activate(index);
}

void MainWindow::onActivated ( const QModelIndex & proxyIndex )
{
	QModelIndex index = mapIndex(proxyIndex);
	activate(index);
}

void MainWindow::activate(const QModelIndex & index)
{
	QString name = _objectModel->data(index).toString();
	if (_objectModel->enter(index.row()))
		_history.push_back(qMakePair(name, _objectModel->parentObjectId()));
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

	QStringList statusList;
	for(const auto & h : _history)
		statusList.push_back(h.first);
	_ui->statusBar->showMessage(statusList.join(QString::fromUtf8(" » ")));
}

void MainWindow::refresh()
{
	qDebug() << "refreshing object list";
	_objectModel->refresh();
}

void MainWindow::downloadFiles()
{
	QItemSelectionModel *selection =_ui->listView->selectionModel();
	QModelIndexList rows = selection->selectedRows();

	QVector<mtp::ObjectId> objects;
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

	mtp::ObjectId oldParent = _objectModel->parentObjectId();
	_history.pop_back();
	mtp::ObjectId oid = _history.empty()? mtp::Session::Root: _history.back().second;
	_objectModel->setParent(oid);
	QModelIndex prevIndex = _objectModel->findObject(oldParent);
	if (prevIndex.isValid())
		_ui->listView->setCurrentIndex(_proxyModel->mapFromSource(prevIndex));
	updateActionsState();
}

void MainWindow::createDirectory()
{
	CreateDirectoryDialog d(this);
	restoreGeometry("create-directory-dialog", d);
	if (d.exec() && !d.name().isEmpty())
	{
		try
		{ _objectModel->createDirectory(d.name(), mtp::AssociationType::GenericFolder); }
		catch(const std::exception &ex)
		{
			QMessageBox::warning(this, tr("Error"),
				tr("Failed to create new directory:\n") + fromUtf8(ex.what())
			);
		}
	}
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
	connect(&progressDialog, SIGNAL(abort()), _uploader, SLOT(abort()));
	_uploader->upload(files);

	progressDialog.exec();

	_objectModel->moveToThread(QApplication::instance()->thread());
	_proxyModel->setSourceModel(_objectModel);
	refresh();
}


void MainWindow::downloadFiles(const QVector<mtp::ObjectId> &objects)
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

	QStringList selected = d.selectedFiles();
	if (selected.isEmpty())
		return;

	QString path = selected.at(0);
	saveGeometry("download-files", d);
	settings.setValue("the-latest-download-directory", path);
	downloadFiles(path, objects);
}

void MainWindow::downloadFiles(const QString & path, const QVector<mtp::ObjectId> &objects)
{
	qDebug() << "downloading to " << path;
	ProgressDialog progressDialog(this);
	progressDialog.setWindowTitle(tr("Download Progress"));
	progressDialog.show();

	connect(_uploader, SIGNAL(uploadProgress(float)), &progressDialog, SLOT(setValue(float)));
	connect(_uploader, SIGNAL(uploadSpeed(qint64)), &progressDialog, SLOT(setSpeed(qint64)));
	connect(_uploader, SIGNAL(uploadStarted(QString)), &progressDialog, SLOT(setFilename(QString)));
	connect(_uploader, SIGNAL(finished()), &progressDialog, SLOT(accept()));
	connect(&progressDialog, SIGNAL(abort()), _uploader, SLOT(abort()));
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
	d.setOption(QFileDialog::ReadOnly, true);
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
	d.setOption(QFileDialog::ReadOnly, true);
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
	d.setOption(QFileDialog::ReadOnly, true);
	restoreGeometry("upload-albums", d);
	if (!d.exec())
		return;

	saveGeometry("upload-albums", d);
	settings.setValue("the-latest-directory", d.directory().absolutePath());
	QStringList selected = d.selectedFiles();
	if (selected.isEmpty())
		return;

	for(const auto &path : selected)
	{
		uploadAlbum(path);
		back();
	}
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

	mtp::ObjectId dirId = _objectModel->createDirectory(dir.dirName(), mtp::AssociationType::Album);
	_objectModel->setParent(dirId);
	_history.push_back(qMakePair(dir.dirName(), dirId));

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

void MainWindow::validateClipboard()
{
	QStringList files = _objectModel->extractMimeData(_clipboard->mimeData());
	_ui->actionPaste->setEnabled(!files.isEmpty());
}

void MainWindow::pasteFromClipboard()
{
	//fixme: CHECK THAT THE MODEL IS NOT IN UPLOADER NOW
	uploadFiles(_objectModel->extractMimeData(_clipboard->mimeData()));
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

void MainWindow::showThumbnails(bool enable)
{
	QSize maxSize;
	if (enable)
		maxSize = QSize(192, 160);

	_ui->listView->setGridSize(maxSize);
	_ui->listView->setWrapping(enable);
	_ui->listView->setFlow(enable? QListView::LeftToRight: QListView::TopToBottom);
	_ui->listView->setViewMode(enable? QListView::IconMode: QListView::ListMode);
	_objectModel->enableThumbnail(enable, maxSize); //resets model/size hints, etc
}
