/*
 * xmonteconvo
 * @author tweber
 * @date aug-2015
 * @copyright GPLv2
 */

#include <clocale>
#include <QLocale>
#include <QApplication>
#include <QSettings>
#include "ConvoDlg.h"


int main(int argc, char** argv)
{
	QApplication app(argc, argv);

	std::setlocale(LC_ALL, "C");
	QLocale::setDefault(QLocale::English);

	QSettings m_settings("tobis_stuff", "takin");
	ConvoDlg dlg(nullptr, &m_settings);
	dlg.setWindowFlags(Qt::Window);
	dlg.show();

	int iRet = app.exec();
	return iRet;
}
