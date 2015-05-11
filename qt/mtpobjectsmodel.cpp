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
#include "mtpobjectsmodel.h"
#include "qtobjectstream.h"
#include "utils.h"
#include <QDebug>
#include <QBrush>
#include <QColor>
#include <QFile>
#include <QFileInfo>
#include <QMimeData>
#include <QUrl>

MtpObjectsModel::MtpObjectsModel(QObject *parent): QAbstractListModel(parent), _storageId(mtp::Session::AllStorages), _parentObjectId(mtp::Session::Root)
{ }

MtpObjectsModel::~MtpObjectsModel()
{ }

void MtpObjectsModel::setStorageId(mtp::u32 storageId)
{
	_storageId = storageId;
	setParent(mtp::Session::Root);
}

void MtpObjectsModel::setParent(mtp::u32 parentObjectId)
{
	beginResetModel();

	_parentObjectId = parentObjectId;
	mtp::msg::ObjectHandles handles = _session->GetObjectHandles(_storageId, mtp::Session::AllFormats, parentObjectId);
	_rows.clear();
	_rows.reserve(handles.ObjectHandles.size());
	for(size_t i = 0; i < handles.ObjectHandles.size(); ++i)
	{
		mtp::u32 oid = handles.ObjectHandles[i];
		_rows.append(Row(oid));
	}

	endResetModel();
}

bool MtpObjectsModel::enter(int idx)
{
	if (idx < 0 || idx >= _rows.size())
		return false;

	Row &row = _rows[idx];
	if (row.IsAssociation(_session))
	{
		setParent(row.ObjectId);
		return true;
	}
	else
		return false;
}

QModelIndex MtpObjectsModel::findObject(mtp::u32 objectId) const
{
	auto idx = std::find_if(_rows.begin(), _rows.end(), [objectId](const Row & row) { return row.ObjectId == objectId; } );
	return idx != _rows.end()? createIndex(std::distance(_rows.begin(), idx), 0): QModelIndex();
}

void MtpObjectsModel::setSession(mtp::SessionPtr session)
{
	beginResetModel();
	_session = session;
	setParent(mtp::Session::Root);
	endResetModel();
}

int MtpObjectsModel::rowCount(const QModelIndex &) const
{ return _rows.size(); }

mtp::msg::ObjectInfoPtr MtpObjectsModel::Row::GetInfo(mtp::SessionPtr session)
{
	if (!_info)
	{
		_info = std::make_shared<mtp::msg::ObjectInfo>();
		try
		{
			*_info = session->GetObjectInfo(ObjectId);
			//qDebug() << fromUtf8(row.Info->Filename);
		}
		catch(const std::exception &ex)
		{ qDebug() << "failed to get object info " << fromUtf8(ex.what()); }
	}
	return _info;
}

bool MtpObjectsModel::Row::IsAssociation(mtp::SessionPtr session)
{
	mtp::ObjectFormat format = GetInfo(session)->ObjectFormat;
	return format == mtp::ObjectFormat::Association || format == mtp::ObjectFormat::AudioAlbum;
}

void MtpObjectsModel::rename(int idx, const QString &fileName)
{
	qDebug() << "renaming row " << idx << " to " << fileName;
	_session->SetObjectProperty(objectIdAt(idx), mtp::ObjectProperty::ObjectFilename, toUtf8(fileName));
	_rows[idx].ResetInfo();
	emit dataChanged(createIndex(idx, 0), createIndex(idx, 0));
}

void MtpObjectsModel::deleteObjects(const MtpObjectList &objects)
{
	for(mtp::u32 objectId: objects)
	{
		qDebug() << "deleting object " << objectId;
		_session->DeleteObject(objectId);
	}
	refresh();
}


mtp::u32 MtpObjectsModel::objectIdAt(int idx)
{
	return (idx >= 0 && idx < _rows.size())? _rows[idx].ObjectId: 0;
}

QVariant MtpObjectsModel::data(const QModelIndex &index, int role) const
{
	int row_idx = index.row();
	if (row_idx < 0 || row_idx > _rows.size())
		return QVariant();

	Row &row = _rows[row_idx];

	switch(role)
	{
	case Qt::DisplayRole:
		return fromUtf8(row.GetInfo(_session)->Filename);

	case Qt::ForegroundRole:
		return row.IsAssociation(_session)? QBrush(QColor(0, 0, 128)): QBrush(Qt::black);

	default:
		return QVariant();
	}
}

mtp::u32 MtpObjectsModel::createDirectory(const QString &name, mtp::AssociationType type)
{
	mtp::Session::NewObjectInfo noi = _session->CreateDirectory(toUtf8(name), _parentObjectId, mtp::Session::Device, type);
	beginInsertRows(QModelIndex(), _rows.size(), _rows.size());
	_rows.push_back(Row(noi.ObjectId));
	endInsertRows();
	return noi.ObjectId;
}

bool MtpObjectsModel::uploadFile(const QString &filePath, QString filename)
{
	QFileInfo fileInfo(filePath);
	mtp::ObjectFormat objectFormat = mtp::ObjectFormatFromFilename(toUtf8(filePath));

	if (filename.isEmpty())
		filename = fileInfo.fileName();
	qDebug() << "uploadFile " << fileInfo.fileName() << " as " << filename;

	std::shared_ptr<QtObjectInputStream> object(new QtObjectInputStream(filePath));
	if (!object->Valid())
	{
		qWarning() << "file " << filePath << " could not be opened";
		return false;
	}
	qDebug() << "sending " << fileInfo.size() << " bytes";
	connect(object.get(), SIGNAL(positionChanged(qint64,qint64)), this, SIGNAL(filePositionChanged(qint64,qint64)));

	mtp::msg::ObjectInfo oi;
	oi.Filename = toUtf8(filename);
	oi.ObjectFormat = objectFormat;
	oi.SetSize(fileInfo.size());
	mtp::Session::NewObjectInfo noi = _session->SendObjectInfo(oi, _storageId, _parentObjectId);
	qDebug() << "new object id: " << noi.ObjectId << ", sending...";
	_session->SendObject(object);
	qDebug() << "ok";
	beginInsertRows(QModelIndex(), _rows.size(), _rows.size());
	_rows.push_back(Row(noi.ObjectId));
	endInsertRows();
	return true;
}

bool MtpObjectsModel::downloadFile(const QString &filePath, mtp::u32 objectId)
{
	std::shared_ptr<QtObjectOutputStream> object(new QtObjectOutputStream(filePath));
	if (!object->Valid())
	{
		qWarning() << "cannot open file " << filePath;
		return false;
	}
	connect(object.get(), SIGNAL(positionChanged(qint64,qint64)), this, SIGNAL(filePositionChanged(qint64,qint64)));
	_session->GetObject(objectId, object);
	return true;
}

MtpObjectsModel::ObjectInfo MtpObjectsModel::getInfoById(mtp::u32 objectId) const
{
	mtp::msg::ObjectInfo oi(_session->GetObjectInfo(objectId));
	qint64 size = oi.ObjectCompressedSize;
	if (size == mtp::MaxObjectSize)
		size = _session->GetObjectIntegerProperty(objectId, mtp::ObjectProperty::ObjectSize);
	return ObjectInfo(fromUtf8(oi.Filename), oi.ObjectFormat, size);
}

bool MtpObjectsModel::dropMimeData(const QMimeData *data,
	 Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
	qDebug() << "data: " << data << action << row << column;
	if (action != Qt::CopyAction || !data)
		return false;

	QList<QUrl> urls = data->urls();
	QStringList files;
	for (auto url : urls)
	{
		//qDebug() << "url " << url;
		if (url.isLocalFile())
			files.push_back(url.toLocalFile());
		else
			qWarning() << "skipping non-local url" << url;
	}
	qDebug() << "files dropped: " << files;
	emit onFilesDropped(files);
	return true;
}

QStringList MtpObjectsModel::mimeTypes () const
{ return QStringList("text/uri-list"); }

Qt::ItemFlags MtpObjectsModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags defaultFlags = QAbstractListModel::flags(index);
	return defaultFlags | Qt::ItemIsDropEnabled;
}
