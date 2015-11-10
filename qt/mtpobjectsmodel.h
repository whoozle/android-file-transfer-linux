/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015  Vladimir Menshakov

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

#ifndef MTPOBJECTSMODEL_H
#define MTPOBJECTSMODEL_H

#include <qabstractitemmodel.h>
#include <mtp/ptp/Device.h>
#include <QVector>
#include <QStringList>

typedef QVector<mtp::ObjectId> MtpObjectList;

class MtpObjectsModel : public QAbstractListModel
{
	Q_OBJECT

private:
	mtp::SessionPtr		_session;
	mtp::StorageId		_storageId;
	mtp::ObjectId		_parentObjectId;

	class Row
	{
		mtp::msg::ObjectInfoPtr					_info;

	public:
		mtp::ObjectId							ObjectId;

		Row(): ObjectId() { }
		Row(mtp::ObjectId id): ObjectId(id) { }

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

	void setStorageId(mtp::StorageId storageId);
	void setParent(mtp::ObjectId parentObjectId);
	void refresh()
	{ setParent(_parentObjectId); }

	bool enter(int idx);
	mtp::ObjectId objectIdAt(int idx);
	QModelIndex findObject(mtp::ObjectId objectId) const;
	QModelIndex findObject(const QString &filename) const;

	mtp::ObjectId parentObjectId() const
	{ return _parentObjectId; }

	mtp::ObjectId createDirectory(const QString &name, mtp::AssociationType type = mtp::AssociationType::GenericFolder);
	bool uploadFile(const QString &filePath, QString filename = QString());
	bool downloadFile(const QString &filePath, mtp::ObjectId objectId);
	void rename(int idx, const QString &fileName);
	ObjectInfo getInfoById(mtp::ObjectId objectId) const;
	void deleteObjects(const MtpObjectList &objects);

	QStringList extractMimeData(const QMimeData *data);

	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;

protected:
	virtual QStringList mimeTypes () const;
	virtual bool dropMimeData(const QMimeData *data,
		 Qt::DropAction action, int row, int column, const QModelIndex &parent);
};

#endif // MTPOBJECTSMODEL_H
