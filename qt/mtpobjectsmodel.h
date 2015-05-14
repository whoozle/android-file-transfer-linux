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
#ifndef MTPOBJECTSMODEL_H
#define MTPOBJECTSMODEL_H

#include <qabstractitemmodel.h>
#include <mtp/ptp/Device.h>
#include <QVector>
#include <QStringList>

typedef QVector<mtp::u32> MtpObjectList;

class MtpObjectsModel : public QAbstractListModel
{
	Q_OBJECT

private:
	mtp::SessionPtr		_session;
	mtp::u32			_storageId;
	mtp::u32			_parentObjectId;

	class Row
	{
		mtp::msg::ObjectInfoPtr					_info;
	public:
		mtp::u32								ObjectId;
		Row(mtp::u32 id = 0): ObjectId(id) { }
		void ResetInfo() { _info.reset(); }
		mtp::msg::ObjectInfoPtr GetInfo(mtp::SessionPtr session);
		bool IsAssociation(mtp::SessionPtr);
	};

	mutable QVector<Row>		_rows;

signals:
	void filePositionChanged(qint64, qint64);
	void onFilesDropped(QStringList);
	bool existingFileOverwrite(QString);

public:
	struct ObjectInfo
	{
		QString				Filename;
		mtp::ObjectFormat	Format;
		qint64				Size;

		ObjectInfo(): Filename(), Format(), Size(0) { }
		ObjectInfo(const QString &fname, mtp::ObjectFormat format, qint64 size): Filename(fname), Format(format), Size(size) { }
	};

	MtpObjectsModel(QObject *parent = 0);
	~MtpObjectsModel();

	void setSession(mtp::SessionPtr session);
	mtp::SessionPtr session() const
	{ return _session; }

	void setStorageId(mtp::u32 storageId);
	void setParent(mtp::u32 parentObjectId);
	void refresh()
	{ setParent(_parentObjectId); }

	bool enter(int idx);
	mtp::u32 objectIdAt(int idx);
	QModelIndex findObject(mtp::u32 objectId) const;
	QModelIndex findObject(const QString &filename) const;

	mtp::u32 parentObjectId() const
	{ return _parentObjectId; }

	mtp::u32 createDirectory(const QString &name, mtp::AssociationType type = mtp::AssociationType::GenericFolder);
	bool uploadFile(const QString &filePath, QString filename = QString());
	bool downloadFile(const QString &filePath, mtp::u32 objectId);
	void rename(int idx, const QString &fileName);
	ObjectInfo getInfoById(mtp::u32 objectId) const;
	void deleteObjects(const MtpObjectList &objects);

	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;

	virtual QStringList mimeTypes () const;
	virtual bool dropMimeData(const QMimeData *data,
		 Qt::DropAction action, int row, int column, const QModelIndex &parent);
};

#endif // MTPOBJECTSMODEL_H
