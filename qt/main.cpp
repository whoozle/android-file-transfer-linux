/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015  Vladimir Menshakov

    Android File Transfer For Linux is free software: you can redistribute
    it and/or modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    Android File Transfer For Linux is distributed in the hope that it will
    be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Android File Transfer For Linux.
    If not, see <http://www.gnu.org/licenses/>.
 */

#include "mainwindow.h"
#include "utils.h"
#include <QApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QMessageBox>
#include <QTranslator>

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
				QMessageBox::warning(0, "Error", fromUtf8(e.what()));
				return false;
			}
		}
	};
}

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	QCoreApplication::setApplicationName("mtp-ng-qt");
	QCoreApplication::setOrganizationDomain("whoozle.github.io");
	QCoreApplication::setOrganizationName("whoozle.github.io");

	QTranslator qtTranslator;

	qtTranslator.load("qt_" + QLocale::system().name(),
					QLibraryInfo::location(QLibraryInfo::TranslationsPath));
	a.installTranslator(&qtTranslator);

	QTranslator aTranslator;
	aTranslator.load(":/translations/android-file-transfer-linux_" + QLocale::system().name());
	a.installTranslator(&aTranslator);

	MainWindow w;
	w.show();

	if (!w.started())
		return 1;

	return a.exec();
}
