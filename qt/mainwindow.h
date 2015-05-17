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
	void downloadFiles(const QVector<quint32> &objects);
	void uploadFiles(const QStringList &files);
	void onStorageChanged(int idx);
	bool confirmOverwrite(const QString &file);

public slots:
	void downloadFiles(const QString & path, const QVector<quint32> &objects);

private:
	Ui::MainWindow *			_ui;
	QSortFilterProxyModel *		_proxyModel;
	MtpStoragesModel *			_storageModel;
	MtpObjectsModel *			_objectModel;
	FileUploader *				_uploader;
	QVector<mtp::u32>			_history;
	int							_uploadAnswer;

	mtp::DevicePtr				_device;
};

#endif // MAINWINDOW_H
