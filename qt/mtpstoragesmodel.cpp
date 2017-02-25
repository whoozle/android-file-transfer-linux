/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2017  Vladimir Menshakov

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
	_storages.append(qMakePair(mtp::Session::AllStorages, anyStorage));
	endResetModel();
	return !storages.StorageIDs.empty();
}

mtp::StorageId MtpStoragesModel::getStorageId(int idx) const
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
