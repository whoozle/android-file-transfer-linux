#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <mtp/ptp/Device.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

private:
	void showEvent(QShowEvent *e);
	Ui::MainWindow *ui;

	mtp::DevicePtr _device;
};

#endif // MAINWINDOW_H
