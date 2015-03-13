#ifndef MTPOBJECTSMODEL_H
#define MTPOBJECTSMODEL_H

#include <qabstractitemmodel.h>
#include <mtp/ptp/Device.h>

class MtpObjectsModel : public QAbstractListModel
{
	mtp::SessionPtr		_session;

public:
	MtpObjectsModel(QObject *parent = 0);
	~MtpObjectsModel();

	void setSession(mtp::SessionPtr session);

	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
};

#endif // MTPOBJECTSMODEL_H
