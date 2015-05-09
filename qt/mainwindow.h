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

#include <QMainWindow>
#include <QModelIndex>
#include <mtp/ptp/Device.h>

namespace Ui {
class MainWindow;
}

class MtpObjectsModel;
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
	QModelIndex mapIndex(const QModelIndex &index);

private slots:
	void back();
	void down();
	void onActivated ( const QModelIndex & index );
	void onSelectionChanged();
	void showContextMenu ( const QPoint & pos );
	void createDirectory();
	void uploadFiles();
	void uploadDirectories();
	void uploadAlbum();
	void uploadAlbum(QString path);
	void downloadFiles();
	void renameFile();
	void deleteFiles();
	void downloadFiles(const QList<quint32> &objects);
	void uploadFiles(const QStringList &files);

private:
	Ui::MainWindow *			_ui;
	QSortFilterProxyModel *		_proxyModel;
	MtpObjectsModel *			_objectModel;
	FileUploader *				_uploader;
	QVector<mtp::u32>			_history;

	mtp::DevicePtr				_device;
};

#endif // MAINWINDOW_H
