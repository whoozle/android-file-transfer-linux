#ifndef DEVICESDIALOG_H
#define DEVICESDIALOG_H

#include <QDialog>
#include <mtp/ptp/Device.h>
#include <mtp/usb/DeviceBusyException.h>
#include <vector>

namespace Ui {
class DevicesDialog;
}
class QListWidgetItem;

class DevicesDialog : public QDialog
{
	Q_OBJECT

private:
	struct Row
	{
		mtp::usb::DeviceDescriptorPtr 	Descriptor;
		mtp::DevicePtr 					Device;
		std::vector<mtp::usb::DeviceBusyException::ProcessDescriptor> Processes;
	};

	bool 					_resetDevice;
	std::vector<Row> 		_devices;

public:
	explicit DevicesDialog(bool resetDevice, QWidget *parent = nullptr);
	~DevicesDialog();

	mtp::DevicePtr getDevice();

	int exec();

private slots:
	void scan();
	void kill();
	void itemClicked(QListWidgetItem *);
	void itemDoubleClicked(QListWidgetItem *);
	void updateButtons();

private:
	Ui::DevicesDialog *	ui;

	QPushButton * _buttonScan;
	QPushButton * _buttonKill;
};

#endif // DEVICESDIALOG_H
