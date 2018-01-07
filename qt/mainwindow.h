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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <mtp/ptp/Device.h>
#include <QMainWindow>
#include <QModelIndex>
#include <QVector>

namespace Ui {
class MainWindow;
}

class MtpObjectsModel;
class MtpStoragesModel;
class FileUploader;

class QSortFilterProxyModel;
class QClipboard;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

	bool started() const
	{ return _device != 0; }

private:
	void showEvent(QShowEvent *e);
	void closeEvent(QCloseEvent *event);
	QModelIndex mapIndex(const QModelIndex &index);
	void saveGeometry(const QString &name, const QWidget &widget);
	void restoreGeometry(const QString &name, QWidget &widget);

private slots:
	bool reconnectToDevice();
	void back();
	void down();
	void onActivated ( const QModelIndex & index );
	void updateActionsState();
	void showContextMenu ( const QPoint & pos );
	void createDirectory();
	void refresh();
	void uploadFiles();
	void uploadDirectories();
	void uploadAlbum();
	void uploadAlbum(QString path);
	void downloadFiles();
	void renameFile();
	void deleteFiles();
	void downloadFiles(const QVector<mtp::ObjectId> &objects);
	void uploadFiles(const QStringList &files);
	void onStorageChanged(int idx);
	void validateClipboard();
	void pasteFromClipboard();
	bool confirmOverwrite(const QString &file);

public slots:
	void downloadFiles(const QString & path, const QVector<mtp::ObjectId> &objects);

private:
	Ui::MainWindow *			_ui;
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
};

#endif // MAINWINDOW_H
