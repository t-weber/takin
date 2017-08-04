/**
 * About Dialog
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date nov-2015
 * @license GPLv2
 */

#include "AboutDlg.h"

#include <boost/config.hpp>
#include <boost/version.hpp>
#include <qwt_global.h>
#include "tlibs/version.h"

#include "tlibs/string/string.h"
#include "libs/formfactors/formfact.h"
#include "libs/spacegroups/spacegroup.h"
#include "libs/globals.h"
#include "libs/version.h"
#include <sstream>

//#define PRIVATE_SRC_VERSION


AboutDlg::AboutDlg(QWidget* pParent, QSettings *pSett)
	: QDialog(pParent), m_pSettings(pSett)
{
	setupUi(this);
	if(m_pSettings)
	{
		QFont font;
		if(m_pSettings->contains("main/font_gen") && font.fromString(m_pSettings->value("main/font_gen", "").toString()))
			setFont(font);

		if(m_pSettings->contains("about/geo"))
			restoreGeometry(m_pSettings->value("about/geo").toByteArray());
	}

	labelVersion->setText("Version " TAKIN_VER ".");
	labelWritten->setText("Written by Tobias Weber <tobias.weber@tum.de>.");
	labelYears->setText("Years: 2014 - 2017.");

#ifdef PRIVATE_SRC_VERSION
	labelRepo->setText("Source repo: <a href=\"https://github.com/t-weber/takin\">https://github.com/t-weber/takin</a>.");
#else
	labelRepo->setText("Source repo: <br><a href=\"https://forge.frm2.tum.de/cgit/cgit.cgi/frm2/mira/tastools.git\">https://forge.frm2.tum.de/cgit/cgit.cgi/frm2/mira/tastools.git</a>.");
#endif

	labelDesc->setText("Overviews of Takin can be found here:"
		"<ul>"
		"<li><a href=\"http://dx.doi.org/10.1016/j.softx.2017.06.002\">doi:10.1016/j.softx.2017.06.002</a>,</li>"
		"<li><a href=\"http://dx.doi.org/10.1016/j.softx.2016.06.002\">doi:10.1016/j.softx.2016.06.002</a>.</li>"
		"</ul>");
	labelDesc->setOpenExternalLinks(1);
	labelLicense->setOpenExternalLinks(1);

	std::string strCC = "Built";
#ifdef BOOST_PLATFORM
		strCC += " for " + std::string(BOOST_PLATFORM);
#endif
	strCC += " using " + std::string(BOOST_COMPILER);
#ifdef __cplusplus
	strCC += " (standard: " + tl::var_to_str(__cplusplus) + ")";
#endif
#ifdef BOOST_STDLIB
		strCC += " with " + std::string(BOOST_STDLIB);
#endif
	strCC += ".";
	labelCC->setText(strCC.c_str());
	labelBuildDate->setText(QString("Build date: ") +
		QString(__DATE__) + ", " + QString(__TIME__));


	// -------------------------------------------------------------------------
	// Libraries

	std::ostringstream ostrLibs;
	ostrLibs << "<html><body>";
	ostrLibs << "<dl>";

	ostrLibs << "<dt>Uses Qt version " << QT_VERSION_STR << ".</dt>";
	ostrLibs << "<dd><a href=\"http://qt-project.org\">http://qt-project.org</a><br></dd>";

	ostrLibs << "<dt>Uses Qwt version " << QWT_VERSION_STR << ".</dt>";
	ostrLibs << "<dd><a href=\"http://qwt.sourceforge.net\">http://qwt.sourceforge.net</a><br></dd>";

	std::string strBoost = BOOST_LIB_VERSION;
	tl::find_all_and_replace<std::string>(strBoost, "_", ".");
	ostrLibs << "<dt>Uses Boost version " << strBoost << ".</dt>";
	ostrLibs << "<dd><a href=\"http://www.boost.org\">http://www.boost.org</a><br></dd>";

	ostrLibs << "<dt>Uses TLibs version " << TLIBS_VERSION << ".</dt>";
#ifdef PRIVATE_SRC_VERSION
	ostrLibs << "<dd><a href=\"https://github.com/t-weber/tlibs\">https://github.com/t-weber/tlibs</a><br></dd>";
#else
	ostrLibs << "<dd><a href=\"https://forge.frm2.tum.de/cgit/cgit.cgi/frm2/mira/tlibs.git\">https://forge.frm2.tum.de/cgit/cgit.cgi/frm2/mira/tlibs.git</a><br></dd>";
#endif

#ifndef NO_LAPACK
	ostrLibs << "<dt>Uses Lapack/e version 3.</dt>";
	ostrLibs << "<dd><a href=\"http://www.netlib.org/lapack\">http://www.netlib.org/lapack</a><br></dd>";
#endif

#ifndef DNO_FIT
	ostrLibs << "<dt>Uses Minuit version 2.</dt>";
	ostrLibs << "<dd><a href=\"https://root.cern.ch\">https://root.cern.ch</a><br></dd>";
#endif

	ostrLibs << "<dt>Uses Clipper crystallography library.</dt>";
	ostrLibs << "<dd><a href=\"http://www.ysbl.york.ac.uk/~cowtan/clipper\">http://www.ysbl.york.ac.uk/~cowtan/clipper</a><br></dd>";

#ifdef HAS_COMPLEX_ERF
	ostrLibs << "<dt>Uses Faddeeva library.</dt>";
	ostrLibs << "<dd><a href=\"http://ab-initio.mit.edu/wiki/index.php/Faddeeva_Package\">http://ab-initio.mit.edu/wiki/index.php/Faddeeva_Package</a><br></dd>";
#endif

	ostrLibs << "<dt>Uses resolution algorithms ported from Rescal version 5.</dt>";
	ostrLibs << "<dd><a href=\"http://www.ill.eu/en/html/instruments-support/computing-for-science/cs-software/all-software/matlab-ill/rescal-for-matlab\">http://www.ill.eu/en/html/instruments-support/computing-for-science/cs-software/all-software/matlab-ill/rescal-for-matlab</a><br></dd>";

	ostrLibs << "<dt>Uses Tango icons.</dt>";
	ostrLibs << "<dd><a href=\"http://tango.freedesktop.org\">http://tango.freedesktop.org</a><br></dd>";

	ostrLibs << "</dl>";
	//ostrLibs << "<p>See the LICENSES file in the Takin root directory.</p>";
	ostrLibs << "</body></html>";

	editLibs->setText(ostrLibs.str().c_str());



	// -------------------------------------------------------------------------
	// Features

	std::ostringstream ostrFeat;
	ostrFeat << "<html><body>";
	ostrFeat << "<h3>Compiled-in features for this version:</h3>";
	ostrFeat << "<ul>";

	ostrFeat << "<li>";
#if defined NO_NET || !defined USE_NET
	ostrFeat << "<font color=\"#ff0000\"><b>Disabled</b></font>";
#else
	ostrFeat << "<b>Enabled</b>";
#endif
	ostrFeat << " support for networking.";
	ostrFeat << "</li>";

	ostrFeat << "<li>";
#if defined NO_PLUGINS || !defined USE_PLUGINS
	ostrFeat << "<font color=\"#ff0000\"><b>Disabled</b></font>";
#else
	ostrFeat << "<b>Enabled</b>";
#endif
	ostrFeat << " support for plugins.";
	ostrFeat << "</li>";

	ostrFeat << "<li>";
#if defined NO_PY || !defined USE_PY
	ostrFeat << "<font color=\"#ff0000\"><b>Disabled</b></font>";
#else
	ostrFeat << "<b>Enabled</b>";
#endif
	ostrFeat << " support for Python scripting.";
	ostrFeat << "</li>";

	ostrFeat << "<li>";
#if defined NO_JL || !defined USE_JL
	ostrFeat << "<font color=\"#ff0000\"><b>Disabled</b></font>";
#else
	ostrFeat << "<b>Enabled</b>";
#endif
	ostrFeat << " support for Julia scripting.";
	ostrFeat << "</li>";

	ostrFeat << "<li>";
#if defined NO_3D || !defined USE_3D
	ostrFeat << "<font color=\"#ff0000\"><b>Disabled</b></font>";
#else
	ostrFeat << "<b>Enabled</b>";
#endif
	ostrFeat << " support for 3D drawing.";
	ostrFeat << "</li>";

	ostrFeat << "<li>";
#if defined NO_GIL || !defined USE_GIL
	ostrFeat << "<font color=\"#ff0000\"><b>Disabled</b></font>";
#else
	ostrFeat << "<b>Enabled</b>";
#endif
	ostrFeat << " support for GIL drawing.";
	ostrFeat << "</li>";

	ostrFeat << "<li>";
#if defined NO_FIT
	ostrFeat << "<font color=\"#ff0000\"><b>Disabled</b></font>";
#else
	ostrFeat << "<b>Enabled</b>";
#endif
	ostrFeat << " support for fitting.";
	ostrFeat << "</li>";

	ostrFeat << "<li>";
#if defined NO_IOSTR || !defined USE_IOSTR
	ostrFeat << "<font color=\"#ff0000\"><b>Disabled</b></font>";
#else
	ostrFeat << "<b>Enabled</b>";
#endif
	ostrFeat << " support for compression.";
	ostrFeat << "</li>";

#if defined USE_BOOST_REX
	ostrFeat << "<li>";
	ostrFeat << "Switched to Boost regex instead of standard.";
	ostrFeat << "</li>";
#endif

#if defined _GLIBCXX_USE_CXX11_ABI
	ostrFeat << "<li>";
	ostrFeat << "Compiled for C++11 binary interface.";
	ostrFeat << "</li>";
#endif

#if defined IS_EXPERIMENTAL_BUILD
	ostrFeat << "<li>";
	ostrFeat << "<font color=\"#ff0000\"><b>Experimental build.</b></font>";
	ostrFeat << "</li>";
#endif

	ostrFeat << "</ul>";
	ostrLibs << "</body></html>";
	labelFeatures->setText(ostrFeat.str().c_str());



	// -------------------------------------------------------------------------
	// Tables

	std::ostringstream ostrConst;
	ostrConst << "<html><body>";
	ostrConst << "<dl>";

	ostrConst << "<dt>Physical constants from Boost Units.</dt>";
	ostrConst << "<dd><a href=\"http://www.boost.org/doc/libs/release/libs/units/\">http://www.boost.org/doc/libs/release/libs/units/</a><br></dd>";

	std::shared_ptr<const SpaceGroups<t_real_glob>> sgs = SpaceGroups<t_real_glob>::GetInstance();
	ostrConst << "<dt>" << sgs->get_sgsource(0) <<"</dt>";
	ostrConst << "<dd><a href=\"" << sgs->get_sgsource(1) << "\">" << sgs->get_sgsource(1) << "</a><br></dd>";

	std::shared_ptr<const FormfactList<t_real_glob>> ff = FormfactList<t_real_glob>::GetInstance();
	std::shared_ptr<const MagFormfactList<t_real_glob>> mff = MagFormfactList<t_real_glob>::GetInstance();
	std::shared_ptr<const ScatlenList<t_real_glob>> sl = ScatlenList<t_real_glob>::GetInstance();
	std::shared_ptr<const PeriodicSystem<t_real_glob>> pt = PeriodicSystem<t_real_glob>::GetInstance();

	if(g_bHasFormfacts)
	{
		ostrConst << "<dt>" << ff->GetSource() << "</dt>";
		ostrConst << "<dd><a href=\"" << ff->GetSourceUrl() << "\">" << ff->GetSourceUrl() << "</a><br></dd>";
	}
	if(g_bHasMagFormfacts)
	{
		ostrConst << "<dt>" << mff->GetSource() << "</dt>";
		ostrConst << "<dd><a href=\"" << mff->GetSourceUrl() << "\">" << mff->GetSourceUrl() << "</a><br></dd>";
	}
	if(g_bHasScatlens)
	{
		ostrConst << "<dt>" << sl->GetSource() << "</dt>";
		ostrConst << "<dd><a href=\"" << sl->GetSourceUrl() << "\">" << sl->GetSourceUrl() << "</a><br></dd>";
	}
	if(g_bHasElements)
	{
		ostrConst << "<dt>" << pt->GetSource() << "</dt>";
		ostrConst << "<dd><a href=\"" << pt->GetSourceUrl() << "\">" << pt->GetSourceUrl() << "</a><br></dd>";
	}

	ostrConst << "</dl>";
	ostrConst << "</body></html>";
	editTables->setText(ostrConst.str().c_str());



	std::string strLicensesFile = find_resource("LICENSES");
	std::ifstream ifstrLicenses(strLicensesFile);
	std::string strLicenses;
	while(ifstrLicenses)
	{
		std::string strLic;
		std::getline(ifstrLicenses, strLic);
		strLicenses += strLic + "\n";
	}
	editAllLicenses->setPlainText(strLicenses.c_str());



	std::string strLitFile = find_resource("LITERATURE");
	std::ifstream ifstrLit(strLitFile);
	std::string strLit;
	while(ifstrLit)
	{
		std::string _strLit;
		std::getline(ifstrLit, _strLit);
		strLit += _strLit + "\n";
	}
	editLiterature->setPlainText(strLit.c_str());
}


void AboutDlg::accept()
{
	if(m_pSettings)
		m_pSettings->setValue("about/geo", saveGeometry());

	QDialog::accept();
}


#include "AboutDlg.moc"
