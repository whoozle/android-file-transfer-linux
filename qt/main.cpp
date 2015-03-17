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
	Application a(argc, argv);
	MainWindow w;
	w.show();

	return a.exec();
}
