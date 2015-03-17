#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "createdirectorydialog.h"
#include "mtpobjectsmodel.h"
#include <QDebug>
#include <QMessageBox>
#include <QKeyEvent>
#include <QFileDialog>
#include <QDir>

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	_ui(new Ui::MainWindow),
	_objectModel(new MtpObjectsModel)
{
	_ui->setupUi(this);
	connect(_ui->listView, SIGNAL(doubleClicked(QModelIndex)), SLOT(onActivated(QModelIndex)));
	connect(_ui->listView, SIGNAL(customContextMenuRequested(QPoint)), SLOT(customContextMenuRequested(QPoint)));
	connect(_ui->actionBack, SIGNAL(triggered()), SLOT(back()));
	connect(_ui->actionGo_Down, SIGNAL(triggered()), SLOT(down()));
	connect(_ui->actionCreateDirectory, SIGNAL(triggered()), SLOT(createDirectory()));
	connect(_ui->actionUploadDirectory, SIGNAL(triggered()), SLOT(uploadDirectories()));
	connect(_ui->actionUpload, SIGNAL(triggered()), SLOT(uploadFiles()));
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
	if (_history.empty())
		return;
	_history.pop_back();
	mtp::u32 oid = _history.empty()? mtp::Session::Root: _history.back();
	_objectModel->setParent(oid);
}

void MainWindow::down()
{
	if (_objectModel->enter(_ui->listView->currentIndex().row()))
		_history.push_back(_objectModel->parentObjectId());
}

void MainWindow::createDirectory()
{
	CreateDirectoryDialog d(this);
	if (d.exec() && !d.name().isEmpty())
		_objectModel->createDirectory(d.name());
}

void MainWindow::uploadFiles(const QStringList &files)
{
	for(QString file : files)
	{
		_objectModel->uploadFile(file);
	}
}

void MainWindow::uploadFiles()
{
	QFileDialog d(this);
	d.setAcceptMode(QFileDialog::AcceptOpen);
	d.setFileMode(QFileDialog::ExistingFiles);
	d.setOption(QFileDialog::ShowDirsOnly, false);
	d.exec();

	uploadFiles(d.selectedFiles());
}

void MainWindow::uploadDirectories()
{
	/*
	QFileDialog d(this);
	d.setAcceptMode(QFileDialog::AcceptOpen);
	d.setFileMode(QFileDialog::Directory);
	d.setOption(QFileDialog::ShowDirsOnly, true);
	d.exec();
*/
	QDir dir = QFileDialog::getExistingDirectory(this);
	qDebug() << "adding directory " << dir.dirName();
	mtp::u32 dirId = _objectModel->createDirectory(dir.dirName());
	_objectModel->setParent(dirId);
	_history.push_back(dirId);
	QStringList files;
	for(QString file : dir.entryList())
	{
		if (file == "." || file == "..")
			continue;
		files.push_back(dir.canonicalPath() + "/" + file);
	}
	uploadFiles(files);
}

