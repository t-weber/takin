/**
 * xmonteconvo
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date aug-2015
 * @copyright GPLv2
 */

#include <clocale>
#include <QLocale>
#include <QApplication>
#include <QDir>
#include <QSettings>
#include <QMessageBox>

#include "tlibs/math/rand.h"
#include "tlibs/file/prop.h"
#include "libs/version.h"
#include "libs/globals.h"
#include "ConvoDlg.h"


int main(int argc, char** argv)
{
	tl::init_rand();

	QApplication app(argc, argv);
	app.setApplicationName("Takin/Monteconvo");
	app.setApplicationVersion(TAKIN_VER);

	// set up resource paths
	std::string strHome = QDir::homePath().toStdString() + "/.takin";
	std::string strApp = app.applicationDirPath().toStdString();
	add_resource_path(strHome, 0);
	add_resource_path(strApp);

	// set locale
	std::setlocale(LC_ALL, "C");
	std::locale::global(std::locale::classic());
	QLocale::setDefault(QLocale::English);

	QSettings m_settings("tobis_stuff", "takin");
	ConvoDlg dlg(nullptr, &m_settings);
	dlg.setWindowFlags(Qt::Window);

	// load a given file
	if(argc > 1)
	{
		const std::string strXmlRoot("taz/");
		tl::Prop<std::string> xml;
		if(xml.Load(argv[1], tl::PropType::XML))
			dlg.Load(xml, strXmlRoot);
		else
			QMessageBox::critical(nullptr, "Error", "Could not load convolution file.");
	}

	// run
	dlg.show();
	return app.exec();
}
