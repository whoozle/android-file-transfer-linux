#include "mtpstoragesmodel.h"
#include "utils.h"
#include <QDebug>

MtpStoragesModel::MtpStoragesModel(QObject *parent): QAbstractListModel(parent)
{ }

bool MtpStoragesModel::update(const mtp::SessionPtr &session)
{
	beginResetModel();
	_storages.clear();
	mtp::msg::StorageIDs storages = session->GetStorageIDs();
	for(auto id : storages.StorageIDs)
	{
		mtp::msg::StorageInfo info;
		try { info = session->GetStorageInfo(id); }
		catch (const mtp::InvalidResponseException &ex)
		{
			if (ex.Type == mtp::ResponseType::InvalidStorageID)
				return false;
			else
				throw;
		}
		_storages.append(qMakePair(id, info));
	}
	mtp::msg::StorageInfo anyStorage;
	anyStorage.StorageDescription = toUtf8(tr("All storages (BUGS, BEWARE)"));
	_storages.append(qMakePair((mtp::u32)mtp::Session::AllStorages, anyStorage));
	endResetModel();
	return !storages.StorageIDs.empty();
}

mtp::u32 MtpStoragesModel::getStorageId(int idx) const
{
	return idx >= 0 && idx < _storages.size()? _storages[idx].first: mtp::Session::AllStorages;
}

int MtpStoragesModel::rowCount(const QModelIndex &parent) const
{
	return _storages.size();
}

QVariant MtpStoragesModel::data(const QModelIndex &index, int role) const
{
	const mtp::msg::StorageInfo & si = _storages[index.row()].second;
	switch(role)
	{
	case Qt::DisplayRole:
		{
			const std::string & title = !si.StorageDescription.empty()? si.StorageDescription:  si.VolumeLabel;
			return fromUtf8(title);
		}
	default:
		return QVariant();
	}
}
