/**
 * xmonteconvo
 * @author tweber
 * @date aug-2015
 * @copyright GPLv2
 */

#include <clocale>
#include <QLocale>
#include <QApplication>
#include <QDir>
#include <QSettings>

#include "tlibs/math/rand.h"
#include "libs/version.h"
#include "libs/globals.h"
#include "ConvoDlg.h"


int main(int argc, char** argv)
{
	tl::init_rand();

	QApplication app(argc, argv);
	app.setApplicationName("Takin/Monteconvo");
	app.setApplicationVersion(TAKIN_VER);

	std::string strHome = QDir::homePath().toStdString() + "/.takin";
	std::string strApp = app.applicationDirPath().toStdString();
	add_resource_path(strHome, 0);
	add_resource_path(strApp);

	std::setlocale(LC_ALL, "C");
	std::locale::global(std::locale::classic());
	QLocale::setDefault(QLocale::English);

	QSettings m_settings("tobis_stuff", "takin");
	ConvoDlg dlg(nullptr, &m_settings);
	dlg.setWindowFlags(Qt::Window);
	dlg.show();

	return app.exec();
}
