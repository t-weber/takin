/*
 * reso tool
 * @author tweber
 * @date 2013, 2014
 * @copyright GPLv2
 */

#include <clocale>
#include "ResoDlg.h"
#include "tlibs/string/spec_char.h"
#include "tlibs/log/log.h"

#ifdef Q_WS_X11
	extern "C" int XInitThreads();
#endif

int main(int argc, char** argv)
{
	try
	{
		tl::log_info("Starting up resolution tool.");

		#ifdef Q_WS_X11
			XInitThreads();
		#endif

		tl::init_spec_chars();
		QApplication app(argc, argv);

		std::setlocale(LC_ALL, "C");
		QLocale::setDefault(QLocale::English);

		ResoDlg dlg(0);
		dlg.show();
		int iRet = app.exec();

		tl::deinit_spec_chars();
		tl::log_info("Shutting down resolution tool.");
		return iRet;
	}
	catch(const std::exception& ex)
	{
		tl::log_crit(ex.what());
	}

	return -1;
}
