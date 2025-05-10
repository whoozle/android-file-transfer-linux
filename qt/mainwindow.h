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

#ifndef AFTL_QT_MAINWINDOW_H
#define AFTL_QT_MAINWINDOW_H

#include <QMainWindow>
#include <QModelIndex>
#include <QVector>
#include <QByteArray>
#include <mtp/ptp/ObjectId.h>
#include <mtp/ptp/ObjectFormat.h>
#include <mtp/types.h>

namespace Ui {
	class MainWindow;
}
namespace mtp {
	class Device;
	DECLARE_PTR(Device);
	class TrustedApp;
	DECLARE_PTR(TrustedApp);
	class Library;
	DECLARE_PTR(Library);
	class Session;
	DECLARE_PTR(Session);
}

class MtpObjectsModel;
class MtpStoragesModel;
class FileUploader;

class QSortFilterProxyModel;
class QClipboard;
class QNetworkAccessManager;
class QNetworkReply;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

	bool started() const
	{ return _device != 0; }

	void enableDeviceReset(bool enable)
	{ _resetDevice = enable; }

private:
	void showEvent(QShowEvent *e);
	void closeEvent(QCloseEvent *event);
	QModelIndex mapIndex(const QModelIndex &index);
	void saveGeometry(const QString &name, const QWidget &widget);
	void restoreGeometry(const QString &name, QWidget &widget);
	static QString getMtpzDataPath();
	void tryCreateLibrary();
	void importMusic(bool directoryMode);

private slots:
	bool reconnectToDevice();
	void back();
	void down();
	void onActivated ( const QModelIndex & index );
	void activate(const QModelIndex & index);
	void updateActionsState();
	void showContextMenu ( const QPoint & pos );
	void createDirectory();
	void refresh();
	void uploadFiles();
	void uploadDirectories();
	void uploadAlbum();
	void uploadAlbum(QString path);
	void importMusic();
	void importMusicFiles();
	void importMusic(QString path);
	void downloadFiles();
	void renameFile();
	void deleteFiles();
	void downloadFiles(const QVector<mtp::ObjectId> &objects);
	void uploadFiles(const QStringList &files, mtp::ObjectFormat format = mtp::ObjectFormat::Any);
	void onFilesDropped(const QStringList &files);
	void onStorageChanged(int idx);
	void validateClipboard();
	void pasteFromClipboard();
	bool confirmOverwrite(const QString &file);
	void showThumbnails(bool enable);
	void attachCover();
	void removeCover();
	void uploadFirmware();
	void rebootDevice();

public slots:
	void downloadFiles(const QString & path, const QVector<mtp::ObjectId> &objects);
	void replyReadyRead();
	void replyFinished(QNetworkReply*);

private:
	Ui::MainWindow *			_ui;
	QNetworkAccessManager *		_nam;
	QClipboard *				_clipboard;
	QSortFilterProxyModel *		_proxyModel;
	MtpStoragesModel *			_storageModel;
	MtpObjectsModel *			_objectModel;
	FileUploader *				_uploader;
	typedef QVector<QPair<QString, mtp::ObjectId>> History;
	History						_history;
	int							_uploadAnswer;

	mtp::DevicePtr				_device;
	mtp::SessionPtr				_session;
	mtp::TrustedAppPtr			_trustedApp;
	mtp::LibraryPtr				_mediaLibrary;
	bool						_resetDevice;
	QNetworkReply *				_networkReply;
	QByteArray					_networkReplyBody;
};

#endif // MAINWINDOW_H
