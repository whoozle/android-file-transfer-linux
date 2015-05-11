#ifndef MTPSTORAGESMODEL_H
#define MTPSTORAGESMODEL_H

#include <QAbstractListModel>
#include <mtp/ptp/Session.h>

class MtpStoragesModel : public QAbstractListModel
{
	QVector<QPair<mtp::u32, mtp::msg::StorageInfo> > _storages;

public:
	MtpStoragesModel(QObject *parent = 0);
	void update(const mtp::SessionPtr &session);

	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
};

#endif // MTPSTORAGESMODEL_H
