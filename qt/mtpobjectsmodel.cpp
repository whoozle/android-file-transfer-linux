#include "mtpobjectsmodel.h"
#include <QDebug>

MtpObjectsModel::MtpObjectsModel(QObject *parent): QAbstractListModel(parent)
{

}

MtpObjectsModel::~MtpObjectsModel()
{

}

void MtpObjectsModel::setParent(mtp::u32 parentObjectId)
{
	beginResetModel();

	_parentObjectId = parentObjectId;
	mtp::msg::ObjectHandles handles = _session->GetObjectHandles(mtp::Session::AllStorages, (mtp::u32)mtp::ObjectFormat::Association, parentObjectId);
	_rows.clear();
	_rows.reserve(handles.ObjectHandles.size());
	for(size_t i = 0; i < handles.ObjectHandles.size(); ++i)
	{
		mtp::u32 oid = handles.ObjectHandles[i];
		_rows.append(Row(oid));
	}

	endResetModel();
}

void MtpObjectsModel::setSession(mtp::SessionPtr session)
{
	beginResetModel();
	_session = session;
	setParent(mtp::Session::Root);
	endResetModel();
}

int MtpObjectsModel::rowCount(const QModelIndex &parent) const
{ return _rows.size(); }

QVariant MtpObjectsModel::data(const QModelIndex &index, int role) const
{
	int row_idx = index.row();
	if (row_idx < 0 || row_idx > _rows.size())
		return QVariant();

	switch(role)
	{
	case Qt::DisplayRole:
		{
			Row &row = _rows[row_idx];
			if (!row.Info)
			{
				row.Info = std::make_shared<mtp::msg::ObjectInfo>();
				try
				{
					*row.Info = _session->GetObjectInfo(row.ObjectId);
					//qDebug() << QString::fromUtf8(row.Info->Filename.c_str());
				}
				catch(const std::exception &ex)
				{ qDebug() << "failed to get object info " << ex.what(); }
			}
			return QString::fromUtf8(row.Info->Filename.c_str());
		}
		break;
	}
	return QVariant();
}
