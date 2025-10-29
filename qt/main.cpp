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
#include <QFile>
#include <QDir>
#include <QTextStream>
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
	
#ifdef Q_OS_MACOS
	// On macOS, when launched from Finder, environment variables are often not set
	// This causes QLocale::system() to default to en_US even if system language is Chinese
	if (qgetenv("LANG").isEmpty() && qgetenv("LC_ALL").isEmpty()) {
		// Check if user has a locale preference file, otherwise default to detecting from available translations
		QFile prefsFile(QDir::home().filePath(".android-file-transfer-locale-prefs"));
		if (prefsFile.exists() && prefsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
			QTextStream in(&prefsFile);
			QString preferredLocale = in.readLine().trimmed();
			if (!preferredLocale.isEmpty()) {
				qputenv("LANG", preferredLocale.toUtf8());
				qputenv("LC_ALL", preferredLocale.toUtf8());
			}
			prefsFile.close();
		} else {
			// Auto-detect Chinese locale if available - this is the most common case for Chinese users
			if (QFile::exists(":/android-file-transfer-linux_zh_CN")) {
				qputenv("LANG", "zh_CN.UTF-8");
				qputenv("LC_ALL", "zh_CN.UTF-8");
			}
		}
	}
#endif

	QCoreApplication::setApplicationName("aft-linux-qt");
	QCoreApplication::setOrganizationDomain("whoozle.github.io");
	QCoreApplication::setOrganizationName("whoozle.github.io");

#if QT_VERSION >= 0x050000
	QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

	QTranslator qtTranslator;
	QString localeName = QLocale::system().name();
	
#ifdef Q_OS_MACOS
	// On macOS, check for user preference file first
	QFile prefsFile(QDir::home().filePath(".android-file-transfer-locale-prefs"));
	if (prefsFile.exists() && prefsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QTextStream in(&prefsFile);
		QString preferredLocale = in.readLine().trimmed();
		if (!preferredLocale.isEmpty()) {
			// Extract just the locale part (e.g., "zh_CN.UTF-8" -> "zh_CN")
			localeName = preferredLocale.split(".").first();
		}
		prefsFile.close();
	} else if (qgetenv("LANG").isEmpty() && qgetenv("LC_ALL").isEmpty()) {
		// When launched from Finder with no preference file, auto-detect Chinese
		if (QFile::exists(":/android-file-transfer-linux_zh_CN")) {
			localeName = "zh_CN";
		}
	}
#endif

	qtTranslator.load("qt_" + localeName,
					QLibraryInfo::location(QLibraryInfo::TranslationsPath));
	app.installTranslator(&qtTranslator);

	QTranslator aTranslator;
	
	// Try to load translation with multiple fallback strategies
	bool translationLoaded = false;
	QStringList localeAttempts;
	
	// Add all possible locale formats to try
	localeAttempts << localeName;  // Original: "zh_CN"
	localeAttempts << localeName.replace("_", "-");  // Hyphen: "zh-CN" 
	localeAttempts << localeName.split("-").first().split("_").first();  // Language only: "zh"
	
	// Also try some common variations for Chinese
	if (localeName.startsWith("zh")) {
		localeAttempts << "zh-CN";  // Simplified Chinese
		localeAttempts << "zh-TW";  // Traditional Chinese  
		localeAttempts << "zh_CN";  // Underscore variant
	}
	
	for (const QString& attempt : localeAttempts) {
		QString resourcePath = ":/android-file-transfer-linux_" + attempt;
		if (aTranslator.load(resourcePath)) {
			translationLoaded = true;
			break;
		}
	}
	
	if (translationLoaded) {
		app.installTranslator(&aTranslator);
	}

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
