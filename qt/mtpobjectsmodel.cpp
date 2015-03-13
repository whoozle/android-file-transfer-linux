#include "mtpobjectsmodel.h"
#include <QDebug>

MtpObjectsModel::MtpObjectsModel(QObject *parent): QAbstractListModel(parent)
{

}

MtpObjectsModel::~MtpObjectsModel()
{

}

void MtpObjectsModel::setSession(mtp::SessionPtr session)
{
	beginResetModel();
	_session = session;
	endResetModel();
}

int MtpObjectsModel::rowCount(const QModelIndex &parent) const
{
	qDebug() << "ROW COUNT";
	return 0;
}

QVariant MtpObjectsModel::data(const QModelIndex &index, int role) const
{
	return QVariant();
}
