/**
 * TAS tool
 * @author tweber
 * @date feb-2014
 * @license GPLv2
 */

#include "taz.h"
#include "tlibs/string/spec_char.h"
#include "tlibs/log/log.h"
#include "tlibs/log/debug.h"
#include "tlibs/time/chrono.h"
#include "tlibs/version.h"
#include "libs/version.h"
#include "libs/globals.h"
#include "dialogs/NetCacheDlg.h"

#include <system_error>
#include <boost/version.hpp>
#include <boost/system/system_error.hpp>

#include <locale>
#include <clocale>
#include <memory>

#include <QMetaType>
#include <QDir>
#include <QMessageBox>
#include <QCoreApplication>


#ifdef Q_WS_X11
	extern "C" int XInitThreads();
#endif


namespace chr = std::chrono;

static bool add_logfile(std::ofstream* postrLog, bool bAdd=1)
{
	if(!postrLog || !postrLog->is_open())
	{
		tl::log_err("Cannot open log file.");
		return 0;
	}

	for(tl::Log* plog : { &tl::log_info, &tl::log_warn, &tl::log_err, &tl::log_crit, &tl::log_debug })
	{
		if(bAdd)
			plog->AddOstr(postrLog, 0, 0);
		else
			plog->RemoveOstr(postrLog);
	}

	if(!bAdd) postrLog->operator<<(std::endl);
	return 1;
}

template<class SysErr = std::system_error>
static inline void sys_err(const SysErr& err)
{
	tl::log_crit("System error: ", err.what(),
		", category: ", err.code().category().name(),
		", value: ", err.code().value(), ".");
	tl::log_backtrace();
}


#define TAKIN_CHECK " Please check if Takin is correctly installed and the current working directory is set to the Takin main directory."

int main(int argc, char** argv)
{
	try
	{
		//std::string strLog = QDir::homePath().toStdString();
		std::string strLog = QDir::tempPath().toStdString();
		strLog += "/takin.log";
		std::ofstream ofstrLog(strLog, std::ios_base::out|std::ios_base::app);
		if(add_logfile(&ofstrLog, 1))
			tl::log_info("Logging to file \"", strLog, "\".");

		tl::log_info("Starting up Takin version ", TAKIN_VER, ".");
		tl::log_debug("Using ", sizeof(t_real_glob)*8, " bit ", tl::get_typename<t_real_glob>(), "s as internal data type.");


		#if defined Q_WS_X11 && !defined NO_3D
			XInitThreads();
			QGL::setPreferredPaintEngine(QPaintEngine::OpenGL);
		#endif

		// qt needs to be able to copy these structs when emitting signals from a different thread
		qRegisterMetaType<TriangleOptions>("TriangleOptions");
		qRegisterMetaType<CrystalOptions>("CrystalOptions");
		qRegisterMetaType<std::string>("std::string");
		qRegisterMetaType<CacheVal>("CacheVal");

		std::unique_ptr<QApplication> app(new QApplication(argc, argv));
		std::setlocale(LC_ALL, "C");
		std::locale::global(std::locale::classic());
		QLocale::setDefault(QLocale::English);

		QCoreApplication::setApplicationName("Takin");
		QCoreApplication::setApplicationVersion(TAKIN_VER);
		std::string strApp = QCoreApplication::applicationDirPath().toStdString();
		tl::log_info("Application path: ", strApp);

		add_resource_path(strApp);
		add_resource_path(strApp + "/..");
		add_resource_path(strApp + "/resources");
		add_resource_path(strApp + "/Resources");
		add_resource_path(strApp + "/../resources");
		add_resource_path(strApp + "/../Resources");


		// ------------------------------------------------------------
		// tlibs version check
		tl::log_info("Using tLibs version ", tl::get_tlibs_version(), ".");
		if(!tl::check_tlibs_version(TLIBS_VERSION))
		{
			tl::log_crit("Version mismatch in tLibs. Please recompile.");
			tl::log_crit("tLibs versions: library: ", tl::get_tlibs_version(),
				", headers: ", TLIBS_VERSION, ".");

			QMessageBox::critical(0, "Takin - Error", "Broken build: Mismatch in tlibs version.");
			return -1;
		}

		// check tables
		g_bHasScatlens = (find_resource("res/scatlens.xml") != "");
		g_bHasFormfacts = (find_resource("res/ffacts.xml") != "");
		g_bHasMagFormfacts = (find_resource("res/magffacts.xml") != "");
		g_bHasSpaceGroups = (find_resource("res/sgroups.xml") != "");

		if(!g_bHasScatlens)
		{
			const char* pcErr = "Scattering length table could not be found." TAKIN_CHECK;
			tl::log_err(pcErr);

			QMessageBox::critical(0, "Takin - Error", pcErr);
			return -1;
		}
		if(!g_bHasFormfacts)
		{
			const char* pcErr = "Atomic form factor coefficient table could not be found." TAKIN_CHECK;
			tl::log_err(pcErr);

			QMessageBox::critical(0, "Takin - Error", pcErr);
			return -1;
		}
		if(!g_bHasMagFormfacts)
		{
			const char* pcErr = "Magnetic form factor coefficient table could not be found." TAKIN_CHECK;
			tl::log_err(pcErr);

			QMessageBox::critical(0, "Takin - Error", pcErr);
			return -1;
		}

		if(!g_bHasSpaceGroups)
		{
			const char* pcErr = "Space group table could not be found!" TAKIN_CHECK;
			tl::log_err(pcErr);

			QMessageBox::critical(0, "Takin - Error", pcErr);
			return -1;
		}

		tl::init_spec_chars();


		// check if icons are available
		if(find_resource("res/document-new.svg") == "")
		{
			const char* pcErr = "Takin resources could not be found!" TAKIN_CHECK;
			tl::log_err(pcErr);

			QMessageBox::critical(0, "Takin - Error", pcErr);
			return -1;
		}


		// ------------------------------------------------------------

#ifdef IS_EXPERIMENTAL_BUILD
		{
			QSettings settings("tobis_stuff", "takin");
			int iPrevDaysSinceEpoch = 0;
			if(settings.contains("debug/last_warned"))
				iPrevDaysSinceEpoch = settings.value("debug/last_warned").toInt();
			int iDaysSinceEpoch = tl::epoch_dur<tl::t_dur_days<int>>().count();

			std::string strExp = "This " BOOST_PLATFORM " version of Takin is still experimental, "
				"does not include all features and may show unexpected behaviour. Please report "
				"bugs to tobias.weber@tum.de. Thanks!";
			tl::log_warn(strExp);
			//tl::log_debug("Days since last warning: ", iDaysSinceEpoch-iPrevDaysSinceEpoch, ".");

			// show warning message box every 5 days
			if(iDaysSinceEpoch - iPrevDaysSinceEpoch > 5)
			{
				QMessageBox::warning(0, "Takin", strExp.c_str());
				settings.setValue("debug/last_warned", iDaysSinceEpoch);
			}
		}
#endif

		std::unique_ptr<TazDlg> dlg(new TazDlg(0));
		if(argc > 1)
			dlg->Load(argv[1]);
		dlg->show();
		int iRet = app->exec();


		// ------------------------------------------------------------
		tl::deinit_spec_chars();
		tl::log_info("Shutting down Takin.");
		add_logfile(&ofstrLog, 0);

		return iRet;
	}
	catch(const std::system_error& err) { sys_err(err); }
	catch(const boost::system::system_error& err) { sys_err(err); }
	catch(const std::exception& ex)
	{
		tl::log_crit("Exception: ", ex.what());
		tl::log_backtrace();
	}
	return -1;
}
