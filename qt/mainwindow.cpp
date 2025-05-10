/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2020  Vladimir Menshakov

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
#include "devicesdialog.h"
#include "utils.h"
#include <mtp/metadata/Library.h>
#include <mtp/mtpz/TrustedApp.h>
#include <mtp/ptp/Device.h>
#include <mtp/ptp/Session.h>
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
#include <QToolButton>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#if QT_VERSION >= 0x050000
#	include <QStandardPaths>
#else
#	include <QDesktopServices>
#endif
#include <QDir>

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	_ui(new Ui::MainWindow), _nam(),
	_clipboard(QApplication::clipboard()),
	_proxyModel(new QSortFilterProxyModel),
	_storageModel(),
	_objectModel(new MtpObjectsModel()),
	_uploader(new FileUploader(_objectModel, this)),
	_resetDevice(false),
	_networkReply()
{
	_ui->setupUi(this);
	setWindowIcon(QIcon(":/android-file-transfer.png"));

	QString theme;
	{
		auto value = palette().text().color().value();
		//trying to guess if it's dark or light theme
		theme = value > 128? "dark": "light";
		qDebug() << "current text color value: " << value << ", guessed theme: " << theme;
	}
	_ui->actionBack->setIcon(QIcon(":/icons/" + theme + "/go-previous.svg"));
	_ui->actionGoDown->setIcon(QIcon(":/icons/" + theme + "/go-next.svg"));
	_ui->actionCreateDirectory->setIcon(QIcon(":/icons/" + theme + "/folder-new.svg"));
	_ui->actionRefresh->setIcon(QIcon(":/icons/" + theme + "/view-refresh.svg"));

	_ui->listView->setModel(_proxyModel);

	_proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
	_proxyModel->sort(0);
	_proxyModel->setDynamicSortFilter(true);

	_objectModel->moveToThread(QApplication::instance()->thread());

	connect(_ui->listView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), SLOT(updateActionsState()));
	connect(_ui->listView, SIGNAL(doubleClicked(QModelIndex)), SLOT(onActivated(QModelIndex)));
	connect(_ui->listView, SIGNAL(customContextMenuRequested(QPoint)), SLOT(showContextMenu(QPoint)));
	connect(_ui->actionBack, SIGNAL(triggered()), SLOT(back()));
	connect(_ui->actionGoDown, SIGNAL(triggered()), SLOT(down()));
	connect(_ui->actionCreateDirectory, SIGNAL(triggered()), SLOT(createDirectory()));
	connect(_ui->actionUploadDirectory, SIGNAL(triggered()), SLOT(uploadDirectories()));
	connect(_ui->actionUploadAlbum, SIGNAL(triggered()), SLOT(uploadAlbum()));
	connect(_ui->actionImportMusic, SIGNAL(triggered()), SLOT(importMusic()));
	connect(_ui->actionImportMusicFiles, SIGNAL(triggered()), SLOT(importMusicFiles()));
	connect(_ui->actionUpload, SIGNAL(triggered()), SLOT(uploadFiles()));
	connect(_ui->actionRename, SIGNAL(triggered()), SLOT(renameFile()));
	connect(_ui->actionDownload, SIGNAL(triggered()), SLOT(downloadFiles()));
	connect(_ui->actionDelete, SIGNAL(triggered()), SLOT(deleteFiles()));
	connect(_ui->storageList, SIGNAL(activated(int)), SLOT(onStorageChanged(int)));
	connect(_ui->actionRefresh, SIGNAL(triggered()), SLOT(refresh()));
	connect(_ui->actionPaste, SIGNAL(triggered()), SLOT(pasteFromClipboard()));
	connect(_ui->actionShowThumbnails, SIGNAL(triggered(bool)), SLOT(showThumbnails(bool)));
	connect(_ui->actionRemoveCover, SIGNAL(triggered(bool)), SLOT(removeCover()));
	connect(_ui->actionAttachCover, SIGNAL(triggered(bool)), SLOT(attachCover()));
	connect(_ui->actionUploadFirmware, SIGNAL(triggered()), SLOT(uploadFirmware()));
	connect(_ui->actionRebootDevice, SIGNAL(triggered()), SLOT(rebootDevice()));

	connect(_objectModel, SIGNAL(onFilesDropped(QStringList)), SLOT(onFilesDropped(QStringList)));
	connect(_objectModel, SIGNAL(existingFileOverwrite(QString)), SLOT(confirmOverwrite(QString)), Qt::BlockingQueuedConnection);

	connect(_clipboard, SIGNAL(dataChanged()), SLOT(validateClipboard()));
	validateClipboard();
	QToolButton * importMusic = dynamic_cast<QToolButton *>(_ui->mainToolBar->widgetForAction(_ui->actionImportMusic));
	importMusic->setMenu(new QMenu(tr("Import Music")));
	importMusic->menu()->addAction(_ui->actionImportMusicFiles);

	//fixme: find out how to specify alternative in designer
	_ui->actionBack->setShortcuts(_ui->actionBack->shortcuts() << QKeySequence("Alt+Up") << QKeySequence("Esc"));
	_ui->actionGoDown->setShortcuts(_ui->actionGoDown->shortcuts() << QKeySequence("Alt+Down") << QKeySequence("Enter"));
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

void MainWindow::replyReadyRead()
{
	if (!_networkReply) {
		qWarning() << "replyReadyRead called without network reply object";
		return;
	}

	auto data = _networkReply->read(128 * 1024);
	qDebug() << "read: " << data.size() << " bytes...";
	_networkReplyBody.append(data);
}

void MainWindow::replyFinished(QNetworkReply * reply)
{
	_networkReply = NULL;
	qDebug() << "got reply " << reply << " with status " << reply->error();
	auto title = tr("MTPZ Keys Download");
	if (reply->error() != 0)
	{
		QMessageBox::warning(this, title, tr("Could not download keys, please find the error below:\n\n%1\n\nPlease look for .mtpz-data file on the internet and manually install it to your home directory.").arg(reply->errorString()));
		return;
	}
	reply->open(QIODevice::ReadOnly);
	auto mtpzDataPath = getMtpzDataPath();
	QFile destination(mtpzDataPath);
	qDebug() << "writing to " << destination.fileName();
	if (!destination.open(QIODevice::WriteOnly)) {
		QMessageBox::warning(this, title, tr("Could not write keys to %1").arg(mtpzDataPath));
		return;
	}

	if (destination.write(_networkReplyBody) == -1)
		QMessageBox::warning(this, title, tr("Could not write keys, please find the error below:\n\n%1\n\nPlease look for .mtpz-data file on the internet and manually install it to your home directory.").arg(destination.errorString()));

	destination.close();
	reply->close();
	reply->deleteLater();
	try
	{
		_trustedApp = mtp::TrustedApp::Create(_session, toUtf8(mtpzDataPath));
		if (!_trustedApp->KeysLoaded())
			throw std::runtime_error("failed to load new keys");

		qDebug() << "new keys loaded, authenticating...";

		_trustedApp->Authenticate();

		QMessageBox::information(this, title, tr("MTPZ keys have been installed to your system."));
		tryCreateLibrary();
	}
	catch (const std::exception & ex)
	{
		qWarning() << "failed to recreate session: " << ex.what();
		QMessageBox::warning(this, title, tr("Your MTPZ keys failed to install or load.\n\nPlease restart the application to try again.\n\nException: %1").arg(ex.what()));
	}

}

QString MainWindow::getMtpzDataPath()
{
#if QT_VERSION >= 0x050000
	auto path = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
#else
	auto path = QDesktopServices::storageLocation(QDesktopServices::HomeLocation);
#endif
	path += "/.mtpz-data";
	return path;
}

bool MainWindow::reconnectToDevice()
{
	_session.reset();
	_device.reset();

	{
		DevicesDialog dialog(_resetDevice, this);
		int r = dialog.exec();
		if (r == QDialog::Accepted)
			_device = dialog.getDevice();
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
			auto path = getMtpzDataPath();
			qDebug() << "mtpz-data path: " << path;
			_trustedApp = mtp::TrustedApp::Create(_session, toUtf8(path));
			if (_trustedApp && !_trustedApp->KeysLoaded()) {
				QString title = tr("MTPZ Keys are Missing");
				QString header = tr(
						"It seems your computer is missing an important bit for talking with MTPZ device: "
						"private keys and certificate material from Microsoft.\n"
						"This means that you can't download and upload files from/to such devices.\n\n"
						"Microsoft could have released that key material and documentation for MTPZ devices "
						"as they are not interested in those anymore.\n\n"
						"Because of the legal risks we can't bundle those keys, even though in some countries it's lawful to modify things to make them working again, "
						"just because you own it.\n\n"
				);
#ifdef MTPZ_DATA_SOURCE
				QMessageBox downloadKeys(QMessageBox::Question,
					title,
					header + tr(
						"Alternatively I (as an app) can offer you to download keys from the Internet.\n"
						"Can I download keys for you?\n\n"

						"(Please press Yes only if all of the above is legal in your country or you just don't care)."
					),
					QMessageBox::Yes | QMessageBox::No
				);
				int r = downloadKeys.exec();
				if (r & QMessageBox::Yes) {
					qDebug() << "downloading keys";
					if (!_nam) {
						_nam = new QNetworkAccessManager(this);
						connect(_nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
					}
					_networkReply = _nam->get(QNetworkRequest(QUrl(MTPZ_DATA_SOURCE)));
					connect(_networkReply, SIGNAL(readyRead()), this, SLOT(replyReadyRead()));
				}
#else
				QMessageBox downloadKeys(QMessageBox::Warning,
					title,
					header + tr(
						"You can look for .mtpz-data file on the internet, download it and place it in your home directory."
					));
				downloadKeys.exec();
#endif
			}
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
		if (_trustedApp && _trustedApp->KeysLoaded())
		{
			qDebug() << "keys loaded, authenticated";
			_trustedApp->Authenticate();
		}

		tryCreateLibrary();

		bool canUpload = _session->GetDeviceInfo().Supports(mtp::OperationCode::SendObjectInfo);
		_ui->actionCreateDirectory->setVisible(canUpload);
		_ui->actionUploadAlbum->setVisible(canUpload);
		_ui->actionUploadDirectory->setVisible(canUpload);
		_ui->actionUpload->setVisible(canUpload);
	}
}

void MainWindow::tryCreateLibrary()
{
	if (_uploader->library())
		return;

	_ui->actionUploadAlbum->setVisible(true);
	_ui->actionImportMusic->setVisible(false);

	if (mtp::Library::Supported(_session)) {
		ProgressDialog progressDialog(this, false);
		progressDialog.setWindowTitle(tr("Loading Media Library"));
		progressDialog.setModal(true);
		progressDialog.setValue(0);

		connect(_uploader, SIGNAL(uploadProgress(float)), &progressDialog, SLOT(setValue(float)));
		connect(_uploader, SIGNAL(uploadSpeed(qint64)), &progressDialog, SLOT(setSpeed(qint64)));
		connect(_uploader, SIGNAL(uploadStarted(QString)), &progressDialog, SLOT(setFilename(QString)));
		connect(_uploader, SIGNAL(finished()), &progressDialog, SLOT(accept()));
		connect(&progressDialog, SIGNAL(abort()), &progressDialog, SLOT(reject()));

		try
		{
			_uploader->tryCreateLibrary();
			_ui->actionUploadAlbum->setVisible(false);
			_ui->actionImportMusic->setVisible(true);
		}
		catch (const std::exception & ex)
		{
			qWarning() << "importing music disabled: " << ex.what();
		}

		progressDialog.exec();
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
	_ui->actionGoDown->setEnabled(rows.size() == 1);
	_ui->actionBack->setEnabled(!_history.empty());

	_ui->actionUploadFirmware->setEnabled(_session? _session->GetDeviceInfo().Supports(mtp::ObjectFormat::UndefinedFirmware): false);
	_ui->actionRebootDevice->setEnabled(_session? _session->GetDeviceInfo().Supports(mtp::OperationCode::RebootDevice): false);

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

	bool showRSMenu = false;

	std::unordered_set<mtp::ObjectFormat> visited;
	for(QModelIndex row : rows)
	{
		row = mapIndex(row);
		auto id = _objectModel->objectIdAt(row.row());
		try {
			mtp::ObjectFormat format = static_cast<mtp::ObjectFormat>(_session->GetObjectIntegerProperty(id, mtp::ObjectProperty::ObjectFormat));
			if (visited.find(format) != visited.end())
				continue;

			visited.insert(format);
			auto supportedProperties = _session->GetObjectPropertiesSupported(format);
			showRSMenu = supportedProperties.Supports(mtp::ObjectProperty::RepresentativeSampleData);
		} catch (const std::exception & ex) {
			qWarning() << "checking representative sample failed";
		}
	}
	if (showRSMenu) {
		menu.addSeparator();
		if (rows.size() == 1)
			menu.addAction(_ui->actionAttachCover);
		menu.addAction(_ui->actionRemoveCover);
	}

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

void MainWindow::uploadFiles(const QStringList &files, mtp::ObjectFormat format)
{
	if (files.isEmpty())
		return;

	qDebug() << "uploadFiles " << files << ", format: " << fromUtf8(mtp::ToString(format));
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
	_uploader->upload(files, format);

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

void MainWindow::importMusic()
{ importMusic(true); }

void MainWindow::importMusicFiles()
{ importMusic(false); }

void MainWindow::importMusic(bool directoryMode)
{
	qDebug() << "import music, directory mode: " << directoryMode;

	QFileDialog d(this);
	QSettings settings;
	{
		QVariant ld = settings.value("the-latest-directory");
		if (ld.isValid())
			d.setDirectory(ld.toString());
	}

	d.setAcceptMode(QFileDialog::AcceptOpen);
	d.setOption(QFileDialog::ReadOnly, true);
	if (directoryMode) {
		d.setFileMode(QFileDialog::Directory);
		d.setOption(QFileDialog::ShowDirsOnly, true);
	} else {
		d.setFileMode(QFileDialog::ExistingFiles);
	}
	restoreGeometry("import-music", d);
	if (!d.exec())
		return;

	saveGeometry("import-music", d);
	settings.setValue("the-latest-directory", d.directory().absolutePath());
	QStringList selected = d.selectedFiles();
	if (selected.isEmpty())
		return;

	for(const auto &path : selected)
	{
		importMusic(path);
	}
}

void MainWindow::onFilesDropped(const QStringList &files)
{
	if (_uploader->library())
	{
		for(auto & file : files)
		{
			importMusic(file);
		}
	}
	else
		uploadFiles(files);
}

namespace
{
	bool HeuristicLess(const QString &s1, const QString &s2)
	{
		return GetCoverScore(s1) > GetCoverScore(s2);
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
		std::sort(covers.begin(), covers.end(), &HeuristicLess);
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

void MainWindow::importMusic(QString path)
{
	qDebug() << "import music from " << path;

	ProgressDialog progressDialog(this);
	progressDialog.setWindowTitle(tr("Importing Progress"));
	progressDialog.show();

	connect(_uploader, SIGNAL(uploadProgress(float)), &progressDialog, SLOT(setValue(float)));
	connect(_uploader, SIGNAL(uploadSpeed(qint64)), &progressDialog, SLOT(setSpeed(qint64)));
	connect(_uploader, SIGNAL(uploadStarted(QString)), &progressDialog, SLOT(setFilename(QString)));
	connect(_uploader, SIGNAL(finished()), &progressDialog, SLOT(accept()));
	connect(&progressDialog, SIGNAL(abort()), _uploader, SLOT(abort()));
	_uploader->importMusic(path);

	progressDialog.exec();
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

void MainWindow::attachCover()
{
	QItemSelectionModel *selection =_ui->listView->selectionModel();
	QModelIndexList rows = selection->selectedRows();
	if (rows.empty())
		return;

	auto row = mapIndex(rows.at(0));
	auto targetObjectId = _objectModel->objectIdAt(row.row());
	qDebug() << "attaching cover to object " << targetObjectId.Id;

	QFileDialog d(this);

	QSettings settings;
	{
		QVariant ld = settings.value("the-latest-directory");
		if (ld.isValid())
			d.setDirectory(ld.toString());
	}

	d.setAcceptMode(QFileDialog::AcceptOpen);
	d.setFileMode(QFileDialog::ExistingFile);
	d.setOption(QFileDialog::ShowDirsOnly, false);
	d.setOption(QFileDialog::ReadOnly, true);
	restoreGeometry("upload-files", d);
	if (!d.exec())
		return;

	saveGeometry("upload-files", d);
	settings.setValue("the-latest-directory", d.directory().absolutePath());
	auto files = d.selectedFiles();
	if (files.empty())
		return;

	QFile file(files.at(0));
	if (!file.open(QIODevice::ReadOnly)) {
		QMessageBox::warning(this, tr("Attach Cover"), tr("Could not open selected file"));
		return;
	}
	auto buffer = file.readAll();
	qDebug() << "read " << buffer.size() << " bytes of cover file";
	file.close();

	mtp::ByteArray value(buffer.begin(), buffer.end());
	try { _session->SetObjectPropertyAsArray(targetObjectId, mtp::ObjectProperty::RepresentativeSampleData, value); }
	catch (const std::exception & ex)
	{
		QMessageBox::warning(this, tr("Attach Cover"), tr("Could not attach cover: %1").arg(ex.what()));
	}
}

void MainWindow::removeCover()
{
	QItemSelectionModel *selection =_ui->listView->selectionModel();
	QModelIndexList rows = selection->selectedRows();

	for(QModelIndex row : rows)
	{
		row = mapIndex(row);
		auto id = _objectModel->objectIdAt(row.row());
		try
		{ _session->SetObjectPropertyAsArray(id, mtp::ObjectProperty::RepresentativeSampleData, {}); }
		catch(const std::exception & ex)
		{ qWarning() << "failed to remove cover:" << ex.what(); }
	}
}

void MainWindow::uploadFirmware()
{
	QFileDialog d(this);

	QSettings settings;
	{
		QVariant ld = settings.value("the-latest-firmware-directory");
		if (ld.isValid())
			d.setDirectory(ld.toString());
	}

	d.setAcceptMode(QFileDialog::AcceptOpen);
	d.setFileMode(QFileDialog::ExistingFiles);
	d.setOption(QFileDialog::ShowDirsOnly, false);
	d.setOption(QFileDialog::ReadOnly, true);
	restoreGeometry("upload-firmware", d);
	if (!d.exec())
		return;

	saveGeometry("upload-firmware", d);
	settings.setValue("the-latest-firmware-directory", d.directory().absolutePath());
	uploadFiles(d.selectedFiles(), mtp::ObjectFormat::UndefinedFirmware);
}

void MainWindow::rebootDevice()
{ _session->RebootDevice(); }
