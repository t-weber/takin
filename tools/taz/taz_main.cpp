/**
 * Takin (TAS tool)
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date feb-2014
 * @license GPLv2
 */

#include "taz.h"
#include "tlibs/string/spec_char.h"
#include "tlibs/log/log.h"
#include "tlibs/math/rand.h"
#include "tlibs/log/debug.h"
#include "tlibs/time/chrono.h"
#include "libs/globals.h"
#include "dialogs/NetCacheDlg.h"

#include "tlibs/version.h"
#include "libs/version.h"
#include "libcrystal/version.h"

#include <system_error>
#include <boost/version.hpp>
#include <boost/system/system_error.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/scope_exit.hpp>

#include <locale>
#include <clocale>
#include <memory>

#include <QMetaType>
#include <QDir>
#include <QMessageBox>
#include <QSplashScreen>
#include <QStyleFactory>

namespace chr = std::chrono;
namespace asio = boost::asio;
namespace sys = boost::system;


// ----------------------------------------------------------------------------
// hacks
#ifdef NON_STANDALONE_MINUIT
	//#include <root/TError.h>
	void SetErrorHandler(void (*)(int, bool, const char*, const char*));
#else
	// dummy handler
	void SetErrorHandler(void (*)(int, bool, const char*, const char*)) {}
#endif


//#ifdef Q_WS_X11
//	extern "C" int XInitThreads();
//#endif
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// logging
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


static void show_splash_msg(QApplication *pApp, QSplashScreen *pSplash, const std::string &strMsg)
{
	QColor colSplash(0xff, 0xcc, 0x00);
	pSplash->showMessage(strMsg.c_str(), Qt::AlignCenter, colSplash);
	pApp->processEvents();
}

#define TAKIN_CHECK " Please check if Takin is correctly installed and the current working directory is set to the Takin main directory."
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
class TakAppl : public QApplication
{
protected:
	std::shared_ptr<TazDlg> m_pTakDlg;
	std::string m_strToLoad;

protected:

public:
	TakAppl(int argc, char** argv) : QApplication(argc, argv, 1) {}
	virtual ~TakAppl() {}

	void SetTakDlg(std::shared_ptr<TazDlg> pDlg) { m_pTakDlg = pDlg; }

	virtual bool event(QEvent *pEvt) override
	{
		if(pEvt->type() == QEvent::FileOpen)
		{
			m_strToLoad = ((QFileOpenEvent*)pEvt)->file().toStdString();
			return true;
		}

		return QApplication::event(pEvt);
	}

	void DoPendingRequests()
	{
		if(!m_pTakDlg) return;

		// user clicked on an associated file to load?
		if(m_strToLoad != "")
		{
			//QMessageBox::information(0, "Takin - Info", ("File: " + m_strToLoad).c_str());
			m_pTakDlg->Load(m_strToLoad.c_str());
		}
	}
};

// ----------------------------------------------------------------------------



/**
 * main entry point
 */
int main(int argc, char** argv)
{
	try
	{
		std::ios_base::sync_with_stdio(0);

#ifdef NO_TERM_CMDS
		tl::Log::SetUseTermCmds(0);
#endif

		// install exit signal handlers
		asio::io_service ioSrv;
		asio::signal_set sigInt(ioSrv, SIGABRT, SIGTERM, SIGINT);
		sigInt.async_wait([&ioSrv](const sys::error_code& err, int iSig)
		{
			tl::log_warn("Hard exit requested via signal ", iSig, ". This may cause a fault.");
			if(err) tl::log_err("Error: ", err.message(), ", error category: ", err.category().name(), ".");
			ioSrv.stop();
#ifdef SIGKILL
			std::raise(SIGKILL);
#endif
			exit(-1);
		});
		std::thread thSig([&ioSrv]() { ioSrv.run(); });
		BOOST_SCOPE_EXIT(&ioSrv, &thSig)
		{
			ioSrv.stop();
			thSig.join();
		}
		BOOST_SCOPE_EXIT_END


		// only for non-standalone minuit
		SetErrorHandler([](int, bool, const char*, const char* pcMsg) { tl::log_err(pcMsg); });


		std::string strLog = QDir::tempPath().toStdString();
		strLog += "/takin.log";
		std::ofstream ofstrLog(strLog, std::ios_base::out|std::ios_base::app);
		if(add_logfile(&ofstrLog, 1))
			tl::log_info("Logging to file \"", strLog, "\".");

		const std::string strStarting = "Starting up Takin version " TAKIN_VER ".";
		tl::log_info(strStarting);
		tl::log_debug("Using ", sizeof(t_real_glob)*8, " bit ", tl::get_typename<t_real_glob>(), "s as internal data type.");


		#if defined Q_WS_X11 && !defined NO_3D
			//XInitThreads();
			QCoreApplication::setAttribute(Qt::AA_X11InitThreads, true);
			QGL::setPreferredPaintEngine(QPaintEngine::OpenGL);
		#endif

		// qt needs to be able to copy these structs when emitting signals from a different thread
		qRegisterMetaType<TriangleOptions>("TriangleOptions");
		qRegisterMetaType<CrystalOptions>("CrystalOptions");
		qRegisterMetaType<std::string>("std::string");
		qRegisterMetaType<CacheVal>("CacheVal");

		std::unique_ptr<TakAppl> app(new TakAppl(argc, argv));

		std::setlocale(LC_ALL, "C");
		std::locale::global(std::locale::classic());
		QLocale::setDefault(QLocale::English);

		tl::init_rand();

		app->setApplicationName("Takin");
		app->setApplicationVersion(TAKIN_VER);

		QSettings settings("tobis_stuff", "takin");


		// set user-selected GUI style
		if(settings.contains("main/gui_style_value"))
		{
			QString strStyle = settings.value("main/gui_style_value", "").toString();
			QStyle *pStyle = QStyleFactory::create(strStyle);
			if(pStyle)
				QApplication::setStyle(pStyle);
			else
				tl::log_err("Style \"", strStyle.toStdString(), "\" was not found.");
		}


		std::string strHome = QDir::homePath().toStdString() + "/.takin";
		std::string strApp = app->applicationDirPath().toStdString();
		tl::log_info("Program path: ", strApp);
		tl::log_info("Home path: ", strHome);

		add_resource_path(strHome, 0);
		add_resource_path(strApp);
		add_resource_path(strApp + "/..");
		add_resource_path(strApp + "/resources");
		add_resource_path(strApp + "/Resources");
		add_resource_path(strApp + "/../resources");
		add_resource_path(strApp + "/../Resources");

		app->addLibraryPath((strApp + "/../lib/plugins").c_str());
		app->addLibraryPath((strApp + "/lib/plugins").c_str());


		// ------------------------------------------------------------
		// splash screen
		QPixmap pixSplash = load_pixmap("res/icons/takin.svg");
		pixSplash = pixSplash.scaled(pixSplash.size().width()*0.55, pixSplash.size().height()*0.55);
		std::unique_ptr<QSplashScreen> pSplash(new QSplashScreen(pixSplash));
		QFont fontSplash = pSplash->font();
		fontSplash.setPixelSize(14);
		fontSplash.setBold(1);
		pSplash->setFont(fontSplash);

		pSplash->show();
		show_splash_msg(app.get(), pSplash.get(), strStarting);


		// ------------------------------------------------------------
		// tlibs & libcrystal version checks
		tl::log_info("Using tlibs version ", tl::get_tlibs_version(), ".");
		if(!tl::check_tlibs_version(TLIBS_VERSION))
		{
			tl::log_crit("Version mismatch in tLibs. Please recompile.");
			tl::log_crit("tLibs versions: library: ", tl::get_tlibs_version(),
				", headers: ", TLIBS_VERSION, ".");

			QMessageBox::critical(0, "Takin - Error", "Broken build: Mismatch in tlibs version.");
			return -1;
		}

		tl::log_info("Using libcrystal version ", xtl::get_libcrystal_version(), ".");
		if(!xtl::check_libcrystal_version(LIBCRYSTAL_VERSION))
		{
			tl::log_crit("Version mismatch in LibCrystal. Please recompile.");
			tl::log_crit("LibCrystal versions: library: ", xtl::get_libcrystal_version(),
				", headers: ", LIBCRYSTAL_VERSION, ".");

			QMessageBox::critical(0, "Takin - Error", "Broken build: Mismatch in libcrystal version.");
			return -1;
		}


		show_splash_msg(app.get(), pSplash.get(), strStarting + "\nChecking resources ...");

		// check tables
		g_bHasElements = (find_resource("res/data/elements.xml") != "");
		g_bHasScatlens = (find_resource("res/data/scatlens.xml") != "");
		g_bHasFormfacts = (find_resource("res/data/ffacts.xml") != "");
		g_bHasMagFormfacts = (find_resource("res/data/magffacts.xml") != "");
		g_bHasSpaceGroups = (find_resource("res/data/sgroups.xml") != "");

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
			tl::log_warn(pcErr);

			//QMessageBox::warning(0, "Takin - Warning", pcErr);
			//return -1;
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
		if(find_resource("res/icons/document-new.svg") == "" ||
			find_resource("res/icons/takin.svg") == "")
		{
			const char* pcErr = "Takin resources could not be found!" TAKIN_CHECK;
			tl::log_err(pcErr);

			QMessageBox::critical(0, "Takin - Error", pcErr);
			return -1;
		}

		// ------------------------------------------------------------


		{
#ifdef IS_EXPERIMENTAL_BUILD
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
			if(iDaysSinceEpoch - iPrevDaysSinceEpoch >= 5)
			{
				QMessageBox::warning(0, "Takin", strExp.c_str());
				settings.setValue("debug/last_warned", iDaysSinceEpoch);
			}
#endif

			// Warnings due to version changes
			if(settings.value("debug/last_warning_shown", 0).toInt() < 1)
			{
				QMessageBox::warning(0, "Takin", "Please beware that in this version "
					"the correction factors for the resolution convolution have been re-worked. "
					"Any global scaling factors will have changed.");
				settings.setValue("debug/last_warning_shown", 1);
			}
		}


		show_splash_msg(app.get(), pSplash.get(), strStarting + "\nLoading 1/2 ...");
		std::shared_ptr<TazDlg> pTakDlg(new TazDlg(nullptr, strLog));
		app->SetTakDlg(pTakDlg);
		show_splash_msg(app.get(), pSplash.get(), strStarting + "\nLoading 2/2 ...");
		app->DoPendingRequests();

		pSplash->finish(pTakDlg.get());
		if(argc > 1)
			pTakDlg->Load(argv[1]);
		pTakDlg->show();
		int iRet = app->exec();


		// ------------------------------------------------------------
		tl::deinit_spec_chars();
		tl::log_info("Shutting down Takin.");
		add_logfile(&ofstrLog, 0);

		return iRet;
	}
	//catch(const std::bad_alloc& err) { tl::log_crit(err.what()); }
	catch(const std::system_error& err) { sys_err(err); }
	catch(const boost::system::system_error& err) { sys_err(err); }
	catch(const std::exception& ex)
	{
		tl::log_crit("Exception: ", ex.what());
		tl::log_backtrace();
	}

	return -1;
}
