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

#ifndef AFTL_QT_MTPOBJECTSMODEL_H
#define AFTL_QT_MTPOBJECTSMODEL_H

#include <qabstractitemmodel.h>
#include <mtp/ptp/Device.h>
#include <QSize>
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
	bool				_enableThumbnails;
	QSize				_maxThumbnailSize;

	using ThumbnailPtr = std::shared_ptr<QPixmap>;

	class Row
	{
		mtp::msg::ObjectInfoPtr					_info;
		ThumbnailPtr							_thumbnail;

	public:
		mtp::ObjectId							ObjectId;

		Row(mtp::ObjectId id = mtp::ObjectId()): ObjectId(id) { }

		void ResetInfo() { _info.reset(); }
		mtp::msg::ObjectInfoPtr GetInfo(mtp::SessionPtr session);
		ThumbnailPtr GetThumbnail(mtp::SessionPtr session, QSize maxSize);
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

	void enableThumbnail(bool enable, QSize maxSize);

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

	mtp::ObjectId createDirectory(mtp::ObjectId parentObjectId, const QString &name, mtp::AssociationType type = mtp::AssociationType::GenericFolder);
	mtp::ObjectId createDirectory(const QString &name, mtp::AssociationType type = mtp::AssociationType::GenericFolder)
	{ return createDirectory(_parentObjectId, name, type); }

	bool uploadFile(mtp::ObjectId parentObjectId, const QString &filePath, QString filename = QString(), mtp::ObjectFormat format = mtp::ObjectFormat::Any);
	bool uploadFile(const QString &filePath, QString filename = QString(), mtp::ObjectFormat format = mtp::ObjectFormat::Any)
	{ return uploadFile(_parentObjectId, filePath, filename, format); }
	bool sendFile(const QString &filePath);
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
