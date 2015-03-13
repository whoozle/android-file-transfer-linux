#ifndef MTPOBJECTSMODEL_H
#define MTPOBJECTSMODEL_H

#include <qabstractitemmodel.h>
#include <mtp/ptp/Device.h>
#include <QVector>

class MtpObjectsModel : public QAbstractListModel
{
	mtp::SessionPtr		_session;
	mtp::u32			_parentObjectId;

	struct Row
	{
		mtp::u32								ObjectId;
		std::shared_ptr<mtp::msg::ObjectInfo>	Info;
		Row(mtp::u32 id = 0): ObjectId(id) { }
	};

	mutable QVector<Row>		_rows;

public:
	MtpObjectsModel(QObject *parent = 0);
	~MtpObjectsModel();

	void setSession(mtp::SessionPtr session);
	void setParent(mtp::u32 parentObjectId);

	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
};

#endif // MTPOBJECTSMODEL_H
