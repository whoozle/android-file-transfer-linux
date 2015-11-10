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

#ifndef MTPSTORAGESMODEL_H
#define MTPSTORAGESMODEL_H

#include <QAbstractListModel>
#include <QPair>
#include <QVector>
#include <mtp/ptp/Session.h>

class MtpStoragesModel : public QAbstractListModel
{
	QVector<QPair<mtp::StorageId, mtp::msg::StorageInfo> > _storages;

public:
	MtpStoragesModel(QObject *parent = 0);
	bool update(const mtp::SessionPtr &session);

	mtp::StorageId getStorageId(int idx) const;
	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
};

#endif // MTPSTORAGESMODEL_H
