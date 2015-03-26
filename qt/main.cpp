/*
 * Android File Transfer for Linux: MTP client for android devices
 * Copyright (C) 2015  Vladimir Menshakov

 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include "mainwindow.h"
#include <QApplication>
#include <QMessageBox>

namespace
{
	class Application : public QApplication
	{
	public:
		Application(int &argc, char **argv, int flags = ApplicationFlags):
			QApplication(argc, argv, flags)
		{ }

		virtual bool notify ( QObject * receiver, QEvent * e )
		{
			try {
				return QApplication::notify( receiver, e );
			} catch ( const std::exception& e ) {
				QMessageBox::warning(0, "Error", e.what());
				return false;
			}
		}
	};
}

int main(int argc, char *argv[])
{
	QCoreApplication::setApplicationName("mtp-ng-qt");
	QCoreApplication::setOrganizationDomain("svalko.org");
	QCoreApplication::setOrganizationName("svalko");

	Application a(argc, argv);
	MainWindow w;
	w.show();

	return a.exec();
}
