#include "devicesdialog.h"
#include "ui_devicesdialog.h"
#include "utils.h"
#include <usb/Context.h>
#include <mtp/ptp/Device.h>
#include <mtp/ptp/PipePacketer.h>
#include <QDebug>
#include <QMessageBox>
#include <QPushButton>
#include <algorithm>

DevicesDialog::DevicesDialog(bool resetDevice, QWidget *parent) :
	QDialog(parent),
	_resetDevice(resetDevice),
	ui(new Ui::DevicesDialog)
{
	ui->setupUi(this);
	_buttonScan = ui->buttonBox->addButton(tr("Rescan Devices"), QDialogButtonBox::ActionRole);
	_buttonKill = ui->buttonBox->addButton(tr("Kill Users"), QDialogButtonBox::ActionRole);
	connect(ui->listWidget, SIGNAL(itemClicked(QListWidgetItem *)), SLOT(itemClicked(QListWidgetItem *)));
	connect(ui->listWidget, SIGNAL(itemDoubleClicked(QListWidgetItem *)), SLOT(itemDoubleClicked(QListWidgetItem *)));
	connect(_buttonScan, SIGNAL(clicked()), SLOT(scan()));
	connect(_buttonKill, SIGNAL(clicked()), SLOT(kill()));
}

mtp::DevicePtr DevicesDialog::getDevice()
{
	int row = ui->listWidget->currentRow();
	if (row >= 0 && row < static_cast<int>(_devices.size()))
		return _devices[row].Device;
	else
		return nullptr;
}

void DevicesDialog::kill()
{
	qDebug("kill");

	int index = ui->listWidget->currentRow();
	if (index < 0 && index >= static_cast<int>(_devices.size()))
		return;

	auto & row = _devices[index];

	bool canKill = !row.Processes.empty();
	QString processList;
	for (auto & desc : row.Processes)
	{
		processList += QString("%1 (pid: %2)\n").arg(fromUtf8(desc.Name)).arg(desc.Id);
	}
	if (canKill)
		processList = tr("The following processes are keeping file descriptors for your device:\n") + processList;

	QMessageBox dialog(
		QMessageBox::Warning,
		tr("Device is busy"),
		tr("Device is busy, maybe another process is using it.\n\n") +
		processList +
		tr("Close other MTP applications and restart Android File Transfer.\n"
		"\nPress Abort to kill them or Ignore to try next device."),
		(canKill? QMessageBox::Ok: QMessageBox::StandardButton(0)) | QMessageBox::Cancel,
		this
	);

	dialog.setDefaultButton(QMessageBox::Ignore);
	dialog.setEscapeButton(QMessageBox::Ignore);
	auto r = dialog.exec();

	if ((r & QMessageBox::Ok) == QMessageBox::Ok)
	{
		qDebug("kill'em all");
		mtp::usb::DeviceBusyException::Kill(row.Processes);
		scan();
	}
}

void DevicesDialog::scan()
{
	qDebug("scan");
	bool claimInterface = true;
	bool resetDevice = _resetDevice;

	_devices.clear();

	mtp::usb::ContextPtr ctx(new mtp::usb::Context);

	auto devices = ctx->GetDevices();
	for (auto desc = devices.begin(); desc != devices.end(); ++desc)
	{
		qDebug("probing device...");
		try
		{
			auto device = mtp::Device::Open(ctx, *desc, claimInterface, resetDevice);
			_devices.push_back({ *desc, device, {} });
		}
		catch(const mtp::usb::DeviceBusyException &ex)
		{
			if (!ex.Processes.empty())
				_devices.push_back(Row { *desc, nullptr, ex.Processes });
		}
		catch(const std::exception &ex)
		{ qWarning("Device::Find failed: %s", ex.what()); }
	}

	std::stable_sort(_devices.begin(), _devices.end(), [](const Row & a, const Row & b) {
		return a.Device > b.Device;
	});

	auto it = std::remove_if(_devices.begin(), _devices.end(), [](const Row & row) {
		return !row.Device && row.Processes.empty();
	});
	_devices.erase(it, _devices.end());

	ui->listWidget->clear();
	for(auto & row : _devices)
	{
		QString name;
		if (row.Device)
		{
			auto info = row.Device->GetInfo();
			name = QString("%1 %2 %3")
				.arg(fromUtf8(info.Manufacturer))
				.arg(fromUtf8(info.Model))
				.arg(fromUtf8(info.SerialNumber));
		}
		else
		{
			auto & desc = row.Descriptor;
			name = QString(tr("USB Device %1:%2"))
				.arg(static_cast<int>(desc->GetVendorId()), 4, 16, QChar('0'))
				.arg(static_cast<int>(desc->GetProductId()), 4, 16, QChar('0'));
		}
		ui->listWidget->addItem(name);
	}

	if (!_devices.empty())
		ui->listWidget->setCurrentRow(0);

	updateButtons();
}


int DevicesDialog::exec()
{
	scan();

	if (_devices.empty())
		return QDialog::Rejected;

	if (_devices.size() == 1 && _devices.front().Device)
	{
		return QDialog::Accepted;
	}

	return QDialog::exec();
}

void DevicesDialog::updateButtons()
{
	_buttonKill->setEnabled(false);
	int row = ui->listWidget->currentRow();
	if (row >= 0 && row < static_cast<int>(_devices.size()))
	{
		_buttonKill->setEnabled(!_devices[row].Processes.empty());
	}
}

void DevicesDialog::itemClicked(QListWidgetItem *)
{ updateButtons(); }

void DevicesDialog::itemDoubleClicked(QListWidgetItem *)
{ updateButtons(); accept(); }

DevicesDialog::~DevicesDialog()
{
	delete ui;
}
