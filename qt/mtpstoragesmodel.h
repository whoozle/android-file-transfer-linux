#ifndef MTPSTORAGESMODEL_H
#define MTPSTORAGESMODEL_H

#include <QAbstractListModel>
#include <QPair>
#include <QVector>
#include <mtp/ptp/Session.h>

class MtpStoragesModel : public QAbstractListModel
{
	QVector<QPair<mtp::u32, mtp::msg::StorageInfo> > _storages;

public:
	MtpStoragesModel(QObject *parent = 0);
	bool update(const mtp::SessionPtr &session);

	mtp::u32 getStorageId(int idx) const;
	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
};

#endif // MTPSTORAGESMODEL_H
