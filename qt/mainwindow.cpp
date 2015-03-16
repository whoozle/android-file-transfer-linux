#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "createdirectorydialog.h"
#include "mtpobjectsmodel.h"
#include <QDebug>
#include <QMessageBox>
#include <QKeyEvent>

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	_ui(new Ui::MainWindow),
	_objectModel(new MtpObjectsModel)
{
	_ui->setupUi(this);
	connect(_ui->listView, SIGNAL(doubleClicked(QModelIndex)), SLOT(onActivated(QModelIndex)));
	connect(_ui->listView, SIGNAL(customContextMenuRequested(QPoint)), SLOT(customContextMenuRequested(QPoint)));
	connect(_ui->actionCreateDirectory, SIGNAL(triggered()), SLOT(createDirectory()));
}

MainWindow::~MainWindow()
{
	delete _ui;
}

void MainWindow::showEvent(QShowEvent *)
{
	if (!_device)
	{
		_device = mtp::Device::Find();
		if (!_device)
		{
			QMessageBox::critical(this, tr("MTP was not found"), tr("No MTP device found"));
			qFatal("device was not found");
		}
		_objectModel->setSession(_device->OpenSession(1));
		_ui->listView->setModel(_objectModel);
	}
}

void MainWindow::onActivated ( const QModelIndex & index )
{
	if (_objectModel->enter(index.row()))
		_history.push_back(_objectModel->parentObjectId());
}

void MainWindow::customContextMenuRequested ( const QPoint & pos )
{
	QItemSelectionModel *selection =_ui->listView->selectionModel();
	QModelIndexList rows = selection->selectedRows();

	QMenu menu(this);
	//http://standards.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html
	QAction * delete_objects = menu.addAction(QIcon::fromTheme("edit-delete"), "Delete");
	QAction * action = menu.exec(_ui->listView->mapToGlobal(pos));
	if (!action)
		return;

	for(int i = rows.size() - 1; i >= 0; --i)
	{
		if (action == delete_objects)
			_objectModel->removeRow(rows[i].row());
		else
			qDebug() << "unknown action!";
	}
}

void MainWindow::back()
{
	if (!_history.empty())
	{
		_history.pop_back();
		mtp::u32 oid = _history.empty()? mtp::Session::Root: _history.back();
		_objectModel->setParent(oid);
	}
}

void MainWindow::keyPressEvent ( QKeyEvent * event )
{
	switch(event->key())
	{
	case Qt::Key_Enter:
		qDebug() << "ENTER";
		{
			if (_objectModel->enter(_ui->listView->currentIndex().row()))
				_history.push_back(_objectModel->parentObjectId());
		}
		break;
	case Qt::Key_Escape:
		back();
		break;
	}
}

void MainWindow::createDirectory()
{
	CreateDirectoryDialog d;
	if (d.exec() && !d.name().isEmpty())
		_objectModel->createDirectory(d.name());
}

