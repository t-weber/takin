/**
 * Scan viewer
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date mar-2015
 * @license GPLv2
 */

#include <clocale>
#include <QLocale>
#include <QApplication>

#include "scanviewer.h"
#include "tlibs/math/rand.h"
#include "libs/version.h"


int main(int argc, char** argv)
{
	tl::init_rand();

	QApplication app(argc, argv);

	app.setApplicationName("Takin/Scanviewer");
	app.setApplicationVersion(TAKIN_VER);

	std::setlocale(LC_ALL, "C");
	QLocale::setDefault(QLocale::English);

	ScanViewerDlg dlg(0);
	dlg.setWindowFlags(Qt::Window);
	dlg.show();

	int iRet = app.exec();
	return iRet;
}
