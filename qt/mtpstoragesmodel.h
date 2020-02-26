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

#ifndef AFTL_QT_MTPSTORAGESMODEL_H
#define AFTL_QT_MTPSTORAGESMODEL_H

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
