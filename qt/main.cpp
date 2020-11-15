/*
    This file is part of Android File Transfer For Linux.
    Copyright (C) 2015-2020  Vladimir Menshakov

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License,
    or (at your option) any later version.

    This library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include "mainwindow.h"
#include "utils.h"
#include <QApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QMessageBox>
#include <QTranslator>
#include <mtp/log.h>
#if QT_VERSION >= 0x050000
#	include <QGuiApplication>
#endif

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
	QApplication app(argc, argv);
	Q_INIT_RESOURCE(android_file_transfer);

	QCoreApplication::setApplicationName("aft-linux-qt");
	QCoreApplication::setOrganizationDomain("whoozle.github.io");
	QCoreApplication::setOrganizationName("whoozle.github.io");

#if QT_VERSION >= 0x050000
	QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

	QTranslator qtTranslator;

	qtTranslator.load("qt_" + QLocale::system().name(),
					QLibraryInfo::location(QLibraryInfo::TranslationsPath));
	app.installTranslator(&qtTranslator);

	QTranslator aTranslator;
	aTranslator.load(":/android-file-transfer-linux_" + QLocale::system().name());
	app.installTranslator(&aTranslator);

	MainWindow w;

	for (int i = 0; i < argc; ++i) {
		if (strcmp(argv[i], "-R") == 0)
			w.enableDeviceReset(true);
		else if (strcmp(argv[i], "-v") == 0)
			mtp::g_debug = true;
	}

	w.show();

	if (!w.started())
		return 1;

	return app.exec();
}
