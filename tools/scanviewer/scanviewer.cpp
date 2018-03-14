/**
 * Scan viewer
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 2015-2016
 * @license GPLv2
 */

#include "scanviewer.h"

#include <QFileDialog>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QMessageBox>

#include <iostream>
#include <set>
#include <string>
#include <algorithm>
#include <iterator>

#include <boost/config.hpp>
#include <boost/version.hpp>
#include <boost/filesystem.hpp>

#include "tlibs/math/math.h"
#include "tlibs/math/stat.h"
#include "tlibs/string/string.h"
#include "tlibs/string/spec_char.h"
#include "tlibs/log/log.h"
#include "libs/version.h"

#ifndef NO_FIT
	#include "tlibs/fit/minuit.h"
	#include "tlibs/fit/swarm.h"
	using tl::t_real_min;
#endif


using t_real = t_real_glob;
namespace fs = boost::filesystem;


#ifndef QWT_VER
	#define QWT_VER 6
#endif


ScanViewerDlg::ScanViewerDlg(QWidget* pParent)
	: QDialog(pParent, Qt::WindowTitleHint|Qt::WindowCloseButtonHint|Qt::WindowMinMaxButtonsHint),
		m_settings("tobis_stuff", "scanviewer"),
		m_vecExts({	".dat", ".DAT", ".scn", ".SCN", ".ng0", ".NG0", ".log", ".LOG"/*, ""*/ }),
		m_pFitParamDlg(new FitParamDlg(this, &m_settings))
{
	this->setupUi(this);
	SetAbout();

	QFont font;
	if(m_settings.contains("main/font_gen") && font.fromString(m_settings.value("main/font_gen", "").toString()))
		setFont(font);

	splitter->setStretchFactor(0, 1);
	splitter->setStretchFactor(1, 2);


	// -------------------------------------------------------------------------
	// plot stuff
	QColor colorBck(240, 240, 240, 255);
	plot->setCanvasBackground(colorBck);

	m_plotwrap.reset(new QwtPlotWrapper(plot, 2, true));

	QPen penCurve;
	penCurve.setColor(QColor(0,0,0x99));
	penCurve.setWidth(2);
	m_plotwrap->GetCurve(0)->setPen(penCurve);
	m_plotwrap->GetCurve(0)->setStyle(QwtPlotCurve::CurveStyle::Lines);
	m_plotwrap->GetCurve(0)->setTitle("Scan Curve");

	QPen penPoints;
	penPoints.setColor(QColor(0xff,0,0));
	penPoints.setWidth(4);
	m_plotwrap->GetCurve(1)->setPen(penPoints);
	m_plotwrap->GetCurve(1)->setStyle(QwtPlotCurve::CurveStyle::Dots);
	m_plotwrap->GetCurve(1)->setTitle("Scan Points");
	// -------------------------------------------------------------------------


	// -------------------------------------------------------------------------
	// property map stuff
	tableProps->setColumnCount(2);
	tableProps->setColumnWidth(0, 150);
	tableProps->setColumnWidth(1, 350);
	//tableProps->sortByColumn(0);

	tableProps->setHorizontalHeaderItem(0, new QTableWidgetItem("Property"));
	tableProps->setHorizontalHeaderItem(1, new QTableWidgetItem("Value"));

	tableProps->verticalHeader()->setVisible(false);
	tableProps->verticalHeader()->setDefaultSectionSize(tableProps->verticalHeader()->minimumSectionSize()+4);
	// -------------------------------------------------------------------------

#if QT_VER>=5
	ScanViewerDlg *pThis = this;
	QObject::connect(editPath, &QLineEdit::textEdited, pThis, &ScanViewerDlg::ChangedPath);
	QObject::connect(listFiles, &QListWidget::itemSelectionChanged, pThis, &ScanViewerDlg::FileSelected);
	QObject::connect(editSearch, &QLineEdit::textEdited, pThis, &ScanViewerDlg::SearchProps);
	QObject::connect(btnBrowse, &QToolButton::clicked, pThis, &ScanViewerDlg::SelectDir);
#ifndef NO_FIT
	QObject::connect(btnParam, &QToolButton::clicked, pThis, &ScanViewerDlg::ShowFitParams);
	QObject::connect(btnGauss, &QToolButton::clicked, pThis, &ScanViewerDlg::FitGauss);
	QObject::connect(btnLorentz, &QToolButton::clicked, pThis, &ScanViewerDlg::FitLorentz);
	QObject::connect(btnVoigt, &QToolButton::clicked, pThis, &ScanViewerDlg::FitVoigt);
	QObject::connect(btnLine, &QToolButton::clicked, pThis, &ScanViewerDlg::FitLine);
	QObject::connect(btnSine, &QToolButton::clicked, pThis, &ScanViewerDlg::FitSine);
#endif
	QObject::connect(comboX, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentIndexChanged), pThis, &ScanViewerDlg::XAxisSelected);
	QObject::connect(comboY, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentIndexChanged), pThis, &ScanViewerDlg::YAxisSelected);
	QObject::connect(spinStart, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), pThis, &ScanViewerDlg::StartOrSkipChanged);
	QObject::connect(spinSkip, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), pThis, &ScanViewerDlg::StartOrSkipChanged);
	QObject::connect(tableProps, &QTableWidget::currentItemChanged, pThis, &ScanViewerDlg::PropSelected);
	QObject::connect(comboExport, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), pThis, &ScanViewerDlg::GenerateExternal);
#else
	QObject::connect(editPath, SIGNAL(textEdited(const QString&)),
		this, SLOT(ChangedPath()));
	QObject::connect(listFiles, SIGNAL(itemSelectionChanged()), this, SLOT(FileSelected()));
	QObject::connect(editSearch, SIGNAL(textEdited(const QString&)),
		this, SLOT(SearchProps(const QString&)));
	QObject::connect(btnBrowse, SIGNAL(clicked(bool)), this, SLOT(SelectDir()));
#ifndef NO_FIT
	QObject::connect(btnParam, SIGNAL(clicked(bool)), this, SLOT(ShowFitParams()));
	QObject::connect(btnGauss, SIGNAL(clicked(bool)), this, SLOT(FitGauss()));
	QObject::connect(btnLorentz, SIGNAL(clicked(bool)), this, SLOT(FitLorentz()));
	QObject::connect(btnVoigt, SIGNAL(clicked(bool)), this, SLOT(FitVoigt()));
	QObject::connect(btnLine, SIGNAL(clicked(bool)), this, SLOT(FitLine()));
	QObject::connect(btnSine, SIGNAL(clicked(bool)), this, SLOT(FitSine()));
#endif
	QObject::connect(comboX, SIGNAL(currentIndexChanged(const QString&)),
		this, SLOT(XAxisSelected(const QString&)));
	QObject::connect(comboY, SIGNAL(currentIndexChanged(const QString&)),
		this, SLOT(YAxisSelected(const QString&)));
	QObject::connect(spinStart, SIGNAL(valueChanged(int)), this, SLOT(StartOrSkipChanged(int)));
	QObject::connect(spinSkip, SIGNAL(valueChanged(int)), this, SLOT(StartOrSkipChanged(int)));
	QObject::connect(tableProps, SIGNAL(currentItemChanged(QTableWidgetItem*, QTableWidgetItem*)),
		this, SLOT(PropSelected(QTableWidgetItem*, QTableWidgetItem*)));
	QObject::connect(comboExport, SIGNAL(currentIndexChanged(int)),
		this, SLOT(GenerateExternal(int)));
	//QObject::connect(btnOpenExt, SIGNAL(clicked()), this, SLOT(openExternally()));
#endif

	QString strDir = m_settings.value("last_dir", tl::wstr_to_str(fs::current_path().native()).c_str()).toString();
	editPath->setText(strDir);

	m_bDoUpdate = 1;
	ChangedPath();

#ifdef NO_FIT
	btnParam->setEnabled(false);
	btnGauss->setEnabled(false);
	btnLorentz->setEnabled(false);
	btnVoigt->setEnabled(false);
	btnLine->setEnabled(false);
	btnSine->setEnabled(false);
#endif

#ifndef HAS_COMPLEX_ERF
	btnVoigt->setEnabled(false);
#endif

	if(m_settings.contains("geo"))
		restoreGeometry(m_settings.value("geo").toByteArray());
}

ScanViewerDlg::~ScanViewerDlg()
{
	ClearPlot();
	tableProps->setRowCount(0);
	if(m_pFitParamDlg) { delete m_pFitParamDlg; m_pFitParamDlg = nullptr; }
}


void ScanViewerDlg::SetAbout()
{
	labelVersion->setText("Version " TAKIN_VER ".");
	labelWritten->setText("Written by Tobias Weber <tobias.weber@tum.de>.");
	labelYears->setText("Years: 2015 - 2018.");

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
	strCC += " on " + std::string(__DATE__) + ", " + std::string(__TIME__);
	strCC += ".";
	labelCC->setText(strCC.c_str());
}


void ScanViewerDlg::closeEvent(QCloseEvent* pEvt)
{
	m_settings.setValue("geo", saveGeometry());
	QDialog::closeEvent(pEvt);
}


void ScanViewerDlg::ClearPlot()
{
	if(m_pInstr)
	{
		delete m_pInstr;
		m_pInstr = nullptr;
	}

	m_vecX.clear();
	m_vecY.clear();
	m_vecFitX.clear();
	m_vecFitY.clear();

	set_qwt_data<t_real>()(*m_plotwrap, m_vecX, m_vecY, 0, 0);
	set_qwt_data<t_real>()(*m_plotwrap, m_vecX, m_vecY, 1, 0);

	m_strX = m_strY = m_strCmd = "";
	plot->setAxisTitle(QwtPlot::xBottom, "");
	plot->setAxisTitle(QwtPlot::yLeft, "");
	plot->setTitle("");

	auto edits = { editA, editB, editC,
		editAlpha, editBeta, editGamma,
		editPlaneX0, editPlaneX1, editPlaneX2,
		editPlaneY0, editPlaneY1, editPlaneY2,
		editTitle, editSample,
		editUser, editContact,
		editKfix, editTimestamp };
	for(auto* pEdit : edits)
		pEdit->setText("");

	comboX->clear();
	comboY->clear();
	textRoot->clear();
	spinStart->setValue(0);
	spinSkip->setValue(0);

	m_plotwrap->GetPlot()->replot();
}

/**
 * new scan directory selected
 */
void ScanViewerDlg::SelectDir()
{
	QFileDialog::Option fileopt = QFileDialog::Option(0);
	if(!m_settings.value("main/native_dialogs", 1).toBool())
		fileopt = QFileDialog::DontUseNativeDialog;

	QString strCurDir = (m_strCurDir==""?".":m_strCurDir.c_str());
	QString strDir = QFileDialog::getExistingDirectory(this, "Select directory",
		strCurDir, QFileDialog::ShowDirsOnly | fileopt);
	if(strDir != "")
	{
		editPath->setText(strDir);
		ChangedPath();
	}
}


void ScanViewerDlg::XAxisSelected(const QString& strLab) { PlotScan(); }
void ScanViewerDlg::YAxisSelected(const QString& strLab) { PlotScan(); }
void ScanViewerDlg::StartOrSkipChanged(int) { PlotScan(); }

/**
 * new file selected
 */
void ScanViewerDlg::FileSelected()
{
	// all selected items
	QList<QListWidgetItem*> lstSelected = listFiles->selectedItems();
	if(lstSelected.size() == 0)
		return;

	ClearPlot();
	m_strCurFile = lstSelected.first()->text().toStdString();

	std::vector<std::string> vecStrSelected;
	for(const QListWidgetItem *pLstItem : lstSelected)
	{
		if(!pLstItem) continue;
		if(pLstItem == lstSelected.first()) continue;

		vecStrSelected.push_back(m_strCurDir + pLstItem->text().toStdString());
	}

	// first file
	std::string strFile = m_strCurDir + m_strCurFile;
	m_pInstr = tl::FileInstrBase<t_real>::LoadInstr(strFile.c_str());
	if(!m_pInstr) return;

	// merge with other selected files
	for(const std::string& strOtherFile : vecStrSelected)
	{
		std::unique_ptr<tl::FileInstrBase<t_real>> pToMerge(
			tl::FileInstrBase<t_real>::LoadInstr(strOtherFile.c_str()));
		if(!pToMerge) continue;

		m_pInstr->MergeWith(pToMerge.get());
	}

	std::vector<std::string> vecScanVars = m_pInstr->GetScannedVars();
	std::string strCntVar = m_pInstr->GetCountVar();
	//std::string strMonVar = m_pInstr->GetMonVar();
	//tl::log_info("Count var: ", strCntVar, ", mon var: ", strMonVar);

	const std::wstring strPM = tl::get_spec_char_utf16("pm");

	m_bDoUpdate = 0;
	int iIdxX=-1, iIdxY=-1, iCurIdx=0;
	const tl::FileInstrBase<t_real>::t_vecColNames& vecColNames = m_pInstr->GetColNames();
	for(const tl::FileInstrBase<t_real>::t_vecColNames::value_type& strCol : vecColNames)
	{
		const tl::FileInstrBase<t_real>::t_vecVals& vecCol = m_pInstr->GetCol(strCol);

		t_real dMean = tl::mean_value(vecCol);
		t_real dStd = tl::std_dev(vecCol);
		bool bStdZero = tl::float_equal(dStd, t_real(0), g_dEpsGfx);

		std::wstring _strCol = tl::str_to_wstr(strCol);
		_strCol += bStdZero ? L" (value: " : L" (mean: ";
		_strCol += tl::var_to_str<t_real, std::wstring>(dMean, g_iPrecGfx);
		if(!bStdZero)
		{
			_strCol += L" " + strPM + L" ";
			_strCol += tl::var_to_str<t_real, std::wstring>(dStd, g_iPrecGfx);
		}
		_strCol += L")";

		comboX->addItem(QString::fromWCharArray(_strCol.c_str()), QString(strCol.c_str()));
		comboY->addItem(QString::fromWCharArray(_strCol.c_str()), QString(strCol.c_str()));

		if(vecScanVars.size() && vecScanVars[0]==strCol)
			iIdxX = iCurIdx;
		if(strCntVar==strCol)
			iIdxY = iCurIdx;

		++iCurIdx;
	}

	comboX->setCurrentIndex(iIdxX);
	comboY->setCurrentIndex(iIdxY);

	m_bDoUpdate = 1;

	ShowProps();
	PlotScan();
}


/**
 * highlights a scan property field
 */
void ScanViewerDlg::SearchProps(const QString& qstr)
{
	QList<QTableWidgetItem*> lstItems = tableProps->findItems(qstr, Qt::MatchContains);
	if(lstItems.size())
		tableProps->setCurrentItem(lstItems[0]);
}


void ScanViewerDlg::PlotScan()
{
	if(m_pInstr==nullptr || !m_bDoUpdate)
		return;

	m_strX = comboX->itemData(comboX->currentIndex(), Qt::UserRole).toString().toStdString();
	m_strY = comboY->itemData(comboY->currentIndex(), Qt::UserRole).toString().toStdString();
	const int iStartIdx = spinStart->value();
	const int iSkipRows = spinSkip->value();
	const std::string strTitle = m_pInstr->GetTitle();
	m_strCmd = m_pInstr->GetScanCommand();

	m_vecX = m_pInstr->GetCol(m_strX.c_str());
	m_vecY = m_pInstr->GetCol(m_strY.c_str());


	// remove points from start
	if(iStartIdx != 0)
	{
		if(std::size_t(iStartIdx) >= m_vecX.size())
			m_vecX.clear();
		else
			m_vecX.erase(m_vecX.begin(), m_vecX.begin()+iStartIdx);

		if(std::size_t(iStartIdx) >= m_vecY.size())
			m_vecY.clear();
		else
			m_vecY.erase(m_vecY.begin(), m_vecY.begin()+iStartIdx);
	}

	// interleave rows
	if(iSkipRows != 0)
	{
		decltype(m_vecX) vecXNew, vecYNew;

		for(std::size_t iRow=0; iRow<std::min(m_vecX.size(), m_vecY.size()); ++iRow)
		{
			vecXNew.push_back(m_vecX[iRow]);
			vecYNew.push_back(m_vecY[iRow]);

			iRow += iSkipRows;
		}

		m_vecX = std::move(vecXNew);
		m_vecY = std::move(vecYNew);
	}


	std::array<t_real, 3> arrLatt = m_pInstr->GetSampleLattice();
	std::array<t_real, 3> arrAng = m_pInstr->GetSampleAngles();
	std::array<t_real, 3> arrPlaneX = m_pInstr->GetScatterPlane0();
	std::array<t_real, 3> arrPlaneY = m_pInstr->GetScatterPlane1();

	editA->setText(tl::var_to_str(arrLatt[0], g_iPrec).c_str());
	editB->setText(tl::var_to_str(arrLatt[1], g_iPrec).c_str());
	editC->setText(tl::var_to_str(arrLatt[2], g_iPrec).c_str());
	editAlpha->setText(tl::var_to_str(tl::r2d(arrAng[0]), g_iPrec).c_str());
	editBeta->setText(tl::var_to_str(tl::r2d(arrAng[1]), g_iPrec).c_str());
	editGamma->setText(tl::var_to_str(tl::r2d(arrAng[2]), g_iPrec).c_str());

	editPlaneX0->setText(tl::var_to_str(arrPlaneX[0], g_iPrec).c_str());
	editPlaneX1->setText(tl::var_to_str(arrPlaneX[1], g_iPrec).c_str());
	editPlaneX2->setText(tl::var_to_str(arrPlaneX[2], g_iPrec).c_str());
	editPlaneY0->setText(tl::var_to_str(arrPlaneY[0], g_iPrec).c_str());
	editPlaneY1->setText(tl::var_to_str(arrPlaneY[1], g_iPrec).c_str());
	editPlaneY2->setText(tl::var_to_str(arrPlaneY[2], g_iPrec).c_str());

	labelKfix->setText(m_pInstr->IsKiFixed()
		? QString::fromWCharArray(L"ki (1/\x212b):")
		: QString::fromWCharArray(L"kf (1/\x212b):"));
	editKfix->setText(tl::var_to_str(m_pInstr->GetKFix()).c_str());

	editTitle->setText(strTitle.c_str());
	editSample->setText(m_pInstr->GetSampleName().c_str());
	editUser->setText(m_pInstr->GetUser().c_str());
	editContact->setText(m_pInstr->GetLocalContact().c_str());
	editTimestamp->setText(m_pInstr->GetTimestamp().c_str());


	plot->setAxisTitle(QwtPlot::xBottom, m_strX.c_str());
	plot->setAxisTitle(QwtPlot::yLeft, m_strY.c_str());
	plot->setTitle(m_strCmd.c_str());


	//if(m_vecX.size()==0 || m_vecY.size()==0)
	//	return;

	if(m_vecFitX.size())
		set_qwt_data<t_real>()(*m_plotwrap, m_vecFitX, m_vecFitY, 0, 0);
	else
		set_qwt_data<t_real>()(*m_plotwrap, m_vecX, m_vecY, 0, 0);
	set_qwt_data<t_real>()(*m_plotwrap, m_vecX, m_vecY, 1, 1);

	GenerateExternal(comboExport->currentIndex());
}


/**
 * convert to external plotter format
 */
void ScanViewerDlg::GenerateExternal(int iLang)
{
	textRoot->clear();
	if(!m_vecX.size() || !m_vecY.size())
		return;

	if(iLang == 0)
		GenerateForRoot();
	else if(iLang == 1)
		GenerateForGnuplot();
	else if(iLang == 2)
		GenerateForPython();
	else if(iLang == 3)
		GenerateForHermelin();
	else
		tl::log_err("Unknown external language.");
}


/**
 * convert to gnuplot
 */
void ScanViewerDlg::GenerateForGnuplot()
{
	const std::string& strTitle = m_strCmd;
	const std::string& strLabelX = m_strX;
	const std::string& strLabelY = m_strY;

	std::string strPySrc =
R"RAWSTR(set term wxt

set xlabel "%%LABELX%%"
set ylabel "%%LABELY%%"
set title "%%TITLE%%"
set grid

set xrange [%%MINX%%:%%MAXX%%]
set yrange [%%MINY%%:%%MAXY%%]

plot "-" using ($1):($2):(sqrt($2)) pointtype 7 with yerrorbars title "Data"
%%POINTS%%
end)RAWSTR";


	std::vector<t_real> vecYErr = m_vecY;
	std::for_each(vecYErr.begin(), vecYErr.end(), [](t_real& d)
	{
		if(tl::float_equal(d, t_real(0.), g_dEps))
			d = 1.;
		d = std::sqrt(d);
	});

	auto minmaxX = std::minmax_element(m_vecX.begin(), m_vecX.end());
	auto minmaxY = std::minmax_element(m_vecY.begin(), m_vecY.end());
	t_real dMaxErrY = *std::max_element(vecYErr.begin(), vecYErr.end());

	std::ostringstream ostrPoints;
	ostrPoints.precision(g_iPrec);

	for(std::size_t i=0; i<std::min(m_vecX.size(), m_vecY.size()); ++i)
	{
		ostrPoints
			<< std::left << std::setw(g_iPrec*2) << m_vecX[i] << " "
			<< std::left << std::setw(g_iPrec*2) << m_vecY[i] << "\n";
	}

	tl::find_and_replace<std::string>(strPySrc, "%%MINX%%", tl::var_to_str(*minmaxX.first, g_iPrec));
	tl::find_and_replace<std::string>(strPySrc, "%%MAXX%%", tl::var_to_str(*minmaxX.second, g_iPrec));
	tl::find_and_replace<std::string>(strPySrc, "%%MINY%%", tl::var_to_str(*minmaxY.first-dMaxErrY, g_iPrec));
	tl::find_and_replace<std::string>(strPySrc, "%%MAXY%%", tl::var_to_str(*minmaxY.second+dMaxErrY, g_iPrec));
	tl::find_and_replace<std::string>(strPySrc, "%%TITLE%%", strTitle);
	tl::find_and_replace<std::string>(strPySrc, "%%LABELX%%", strLabelX);
	tl::find_and_replace<std::string>(strPySrc, "%%LABELY%%", strLabelY);
	tl::find_and_replace<std::string>(strPySrc, "%%POINTS%%", ostrPoints.str());

	textRoot->setText(strPySrc.c_str());
}


/**
 * convert to python
 */
void ScanViewerDlg::GenerateForPython()
{
	const std::string& strTitle = m_strCmd;
	const std::string& strLabelX = m_strX;
	const std::string& strLabelY = m_strY;

	std::string strPySrc =
R"RAWSTR(import numpy as np

x = np.array([ %%VECX%% ])
y = np.array([ %%VECY%% ])
yerr = np.array([ %%VECYERR%% ])

min = np.array([ %%MINX%%, %%MINY%% ])
max = np.array([ %%MAXX%%, %%MAXY%% ])
range = max-min
mid = min + range*0.5

yerr = [a if a!=0. else 0.001*range[1] for a in yerr]



import scipy.optimize as opt

def gauss_model(x, x0, sigma, amp, offs):
        return amp * np.exp(-0.5 * ((x-x0) / sigma)**2.) + offs

hints = [mid[0], range[0]*0.5, range[1]*0.5, min[1]]
popt, pcov = opt.curve_fit(gauss_model, x, y, sigma=yerr, absolute_sigma=True, p0=hints)

x_fine = np.linspace(min[0], max[0], 128)
y_fit = gauss_model(x_fine, *popt)



import matplotlib.pyplot as plt

plt.figure()

plt.xlim(min[0], max[0])
plt.ylim(min[1], max[1])

plt.title("%%TITLE%%")
plt.xlabel("%%LABELX%%")
plt.ylabel("%%LABELY%%")

plt.grid(True)
plt.errorbar(x,y,yerr, fmt="o")
plt.plot(x_fine, y_fit)
plt.show())RAWSTR";


	std::vector<t_real> vecYErr = m_vecY;
	std::for_each(vecYErr.begin(), vecYErr.end(), [](t_real& d)
	{
		if(tl::float_equal(d, t_real(0.), g_dEps))
			d = 1.;
		d = std::sqrt(d);
	});

	auto minmaxX = std::minmax_element(m_vecX.begin(), m_vecX.end());
	auto minmaxY = std::minmax_element(m_vecY.begin(), m_vecY.end());
	t_real dMaxErrY = *std::max_element(vecYErr.begin(), vecYErr.end());

	std::ostringstream ostrX, ostrY, ostrYErr;
	ostrX.precision(g_iPrec);
	ostrY.precision(g_iPrec);
	ostrYErr.precision(g_iPrec);

	for(std::size_t i=0; i<std::min(m_vecX.size(), m_vecY.size()); ++i)
	{
		ostrX << m_vecX[i] << ", ";
		ostrY << m_vecY[i] << ", ";
		ostrYErr << vecYErr[i] << ", ";
	}

	tl::find_and_replace<std::string>(strPySrc, "%%MINX%%", tl::var_to_str(*minmaxX.first, g_iPrec));
	tl::find_and_replace<std::string>(strPySrc, "%%MAXX%%", tl::var_to_str(*minmaxX.second, g_iPrec));
	tl::find_and_replace<std::string>(strPySrc, "%%MINY%%", tl::var_to_str(*minmaxY.first-dMaxErrY, g_iPrec));
	tl::find_and_replace<std::string>(strPySrc, "%%MAXY%%", tl::var_to_str(*minmaxY.second+dMaxErrY, g_iPrec));
	tl::find_and_replace<std::string>(strPySrc, "%%TITLE%%", strTitle);
	tl::find_and_replace<std::string>(strPySrc, "%%LABELX%%", strLabelX);
	tl::find_and_replace<std::string>(strPySrc, "%%LABELY%%", strLabelY);
	tl::find_and_replace<std::string>(strPySrc, "%%VECX%%", ostrX.str());
	tl::find_and_replace<std::string>(strPySrc, "%%VECY%%", ostrY.str());
	tl::find_and_replace<std::string>(strPySrc, "%%VECYERR%%", ostrYErr.str());

	textRoot->setText(strPySrc.c_str());
}


/**
 * convert to hermelin
 */
void ScanViewerDlg::GenerateForHermelin()
{
    std::string strStoatSrc =
R"RAWSTR(#!./hermelin -t

module_init()
{
	import("apps/instr.scr");

	global fit_dbg = 1;
	global theterm = "wxt";
	global norm_to_mon = 1;
}

scan_plot()
{
	scanfile = "%%FILE%%";
	[instr, datx, daty, datyerr, xlab, ylab] = load_instr(scanfile, norm_to_mon);
	title = "\"" + scanfile + "\"";

	maxx = max(datx); minx = min(datx);
	maxy = max(daty); miny = min(daty);
	rangex = maxx-minx; rangey = maxy-miny;
	midx = minx + rangex*0.5;


	gauss_pos = [midx];
	gauss_amp = [rangey*0.5];
	gauss_sig = [rangex*0.5];
	bckgrd = miny;
	fitsteps = ["xxx x"];

	outfile = "";
	#outfile = "scan_plot.pdf";

	thefit = fit_gauss_manual_singlestep(datx, daty, datyerr, gauss_pos, gauss_amp, gauss_sig, bckgrd, fitsteps);


	plotmap = map();
	plotmap += ["xlimits" : minx + " " + maxx];
	#plotmap += ["ylimits" : "0 0.5"];

	plot_gausses(1, thefit, [datx, daty, datyerr], title, xlab, ylab, outfile, plotmap);
}

main(args)
{
	scan_plot();
})RAWSTR";

	const std::string strFile = m_strCurDir + m_strCurFile;
	tl::find_and_replace<std::string>(strStoatSrc, "%%FILE%%", strFile);

	textRoot->setText(strStoatSrc.c_str());
}


/**
 * convert to Root
 */
void ScanViewerDlg::GenerateForRoot()
{
	const std::string& strTitle = m_strCmd;
	const std::string& strLabelX = m_strX;
	const std::string& strLabelY = m_strY;

	std::string strRootSrc =
R"RAWSTR(void scan_plot()
{
	const Double_t vecX[] = { %%VECX%% };
	const Double_t vecY[] = { %%VECY%% };
	const Double_t vecYErr[] = { %%VECYERR%% };

	const Double_t dMin[] = { %%MINX%%, %%MINY%% };
	const Double_t dMax[] = { %%MAXX%%, %%MAXY%% };
	const Int_t iSize = sizeof(vecX)/sizeof(*vecX);

	gStyle->SetOptFit(1);
	TCanvas *pCanvas = new TCanvas("canvas0", "Root Canvas", 800, 600);
	pCanvas->SetGrid(1,1);
	pCanvas->SetTicks(1,1);
	//pCanvas->SetLogy();

	TH1F *pFrame = pCanvas->DrawFrame(dMin[0], dMin[1], dMax[0], dMax[1], "");
	pFrame->SetTitle("%%TITLE%%");
	pFrame->SetXTitle("%%LABELX%%");
	pFrame->SetYTitle("%%LABELY%%");

	TGraphErrors *pGraph = new TGraphErrors(iSize, vecX, vecY, 0, vecYErr);
	pGraph->SetMarkerStyle(20);
	pGraph->Draw("P");
})RAWSTR";


	std::vector<t_real> vecYErr = m_vecY;
	std::for_each(vecYErr.begin(), vecYErr.end(), [](t_real& d)
	{
		if(tl::float_equal(d, t_real(0.), g_dEps))
			d = 1.;
		d = std::sqrt(d);
	});

	auto minmaxX = std::minmax_element(m_vecX.begin(), m_vecX.end());
	auto minmaxY = std::minmax_element(m_vecY.begin(), m_vecY.end());
	t_real dMaxErrY = *std::max_element(vecYErr.begin(), vecYErr.end());

	std::ostringstream ostrX, ostrY, ostrYErr;
	ostrX.precision(g_iPrec);
	ostrY.precision(g_iPrec);
	ostrYErr.precision(g_iPrec);

	for(std::size_t i=0; i<std::min(m_vecX.size(), m_vecY.size()); ++i)
	{
		ostrX << m_vecX[i] << ", ";
		ostrY << m_vecY[i] << ", ";
		ostrYErr << vecYErr[i] << ", ";
	}

	tl::find_and_replace<std::string>(strRootSrc, "%%MINX%%", tl::var_to_str(*minmaxX.first, g_iPrec));
	tl::find_and_replace<std::string>(strRootSrc, "%%MAXX%%", tl::var_to_str(*minmaxX.second, g_iPrec));
	tl::find_and_replace<std::string>(strRootSrc, "%%MINY%%", tl::var_to_str(*minmaxY.first-dMaxErrY, g_iPrec));
	tl::find_and_replace<std::string>(strRootSrc, "%%MAXY%%", tl::var_to_str(*minmaxY.second+dMaxErrY, g_iPrec));
	tl::find_and_replace<std::string>(strRootSrc, "%%TITLE%%", strTitle);
	tl::find_and_replace<std::string>(strRootSrc, "%%LABELX%%", strLabelX);
	tl::find_and_replace<std::string>(strRootSrc, "%%LABELY%%", strLabelY);
	tl::find_and_replace<std::string>(strRootSrc, "%%VECX%%", ostrX.str());
	tl::find_and_replace<std::string>(strRootSrc, "%%VECY%%", ostrY.str());
	tl::find_and_replace<std::string>(strRootSrc, "%%VECYERR%%", ostrYErr.str());

	textRoot->setText(strRootSrc.c_str());
}


/**
 * save selected property key for later
 */
void ScanViewerDlg::PropSelected(QTableWidgetItem *pItem, QTableWidgetItem *pItemPrev)
{
	if(!pItem)
		m_strSelectedKey = "";

	for(int iItem=0; iItem<tableProps->rowCount(); ++iItem)
	{
		const QTableWidgetItem *pKey = tableProps->item(iItem, 0);
		const QTableWidgetItem *pVal = tableProps->item(iItem, 1);

		if(pKey==pItem || pVal==pItem)
		{
			m_strSelectedKey = pKey->text().toStdString();
			break;
		}
	}
}


/**
 * save selected property key for later
 */
void ScanViewerDlg::ShowProps()
{
	if(m_pInstr==nullptr || !m_bDoUpdate)
		return;

	const tl::FileInstrBase<t_real>::t_mapParams& params = m_pInstr->GetAllParams();
	tableProps->setRowCount(params.size());

	const bool bSort = tableProps->isSortingEnabled();
	tableProps->setSortingEnabled(0);
	unsigned int iItem = 0;
	for(const tl::FileInstrBase<t_real>::t_mapParams::value_type& pair : params)
	{
		QTableWidgetItem *pItemKey = tableProps->item(iItem, 0);
		if(!pItemKey)
		{
			pItemKey = new QTableWidgetItem();
			tableProps->setItem(iItem, 0, pItemKey);
		}

		QTableWidgetItem* pItemVal = tableProps->item(iItem, 1);
		if(!pItemVal)
		{
			pItemVal = new QTableWidgetItem();
			tableProps->setItem(iItem, 1, pItemVal);
		}

		pItemKey->setText(pair.first.c_str());
		pItemVal->setText(pair.second.c_str());

		++iItem;
	}

	tableProps->setSortingEnabled(bSort);
	//tableProps->sortItems(0, Qt::AscendingOrder);


	// retain previous selection
	bool bHasSelection = 0;
	for(int iItem=0; iItem<tableProps->rowCount(); ++iItem)
	{
		const QTableWidgetItem *pItem = tableProps->item(iItem, 0);
		if(!pItem) continue;

		if(pItem->text().toStdString() == m_strSelectedKey)
		{
			tableProps->selectRow(iItem);
			bHasSelection = 1;
			break;
		}
	}

	if(!bHasSelection)
		tableProps->selectRow(0);
}


/**
 * new directory entered
 */
void ScanViewerDlg::ChangedPath()
{
	listFiles->clear();
	ClearPlot();
	tableProps->setRowCount(0);

	std::string strPath = editPath->text().toStdString();
	fs::path dir(strPath);
	if(fs::exists(dir) && fs::is_directory(dir))
	{
		m_strCurDir = tl::wstr_to_str(dir.native());
		tl::trim(m_strCurDir);
		if(*(m_strCurDir.begin()+m_strCurDir.length()-1) != fs::path::preferred_separator)
			m_strCurDir += fs::path::preferred_separator;
		UpdateFileList();

		// watch directory for changes
		m_pWatcher.reset(new QFileSystemWatcher(this));
		m_pWatcher->addPath(m_strCurDir.c_str());
		QObject::connect(m_pWatcher.get(), SIGNAL(directoryChanged(const QString&)),
			this, SLOT(DirWasModified(const QString&)));

		m_settings.setValue("last_dir", QString(m_strCurDir.c_str()));
	}
}


/**
 * the current directory has been modified externally
 */
void ScanViewerDlg::DirWasModified(const QString& strDir)
{
	// get currently selected item
	QString strTxt;
	const QListWidgetItem *pCur = listFiles->currentItem();
	if(pCur)
		strTxt = pCur->text();

	UpdateFileList();

	// re-select previously selected item
	if(pCur)
	{
		QList<QListWidgetItem*> lstItems = listFiles->findItems(
			strTxt, Qt::MatchExactly);
		if(lstItems.size())
			listFiles->setCurrentItem(*lstItems.begin(), QItemSelectionModel::SelectCurrent);
	}
}


/**
 * re-populate file list
 */
void ScanViewerDlg::UpdateFileList()
{
	listFiles->clear();

	try
	{
		fs::path dir(m_strCurDir);
		fs::directory_iterator dir_begin(dir), dir_end;

		std::set<fs::path> lst;
		std::copy_if(dir_begin, dir_end, std::insert_iterator<decltype(lst)>(lst, lst.end()),
			[this](const fs::path& p) -> bool
			{
				std::string strExt = tl::wstr_to_str(p.extension().native());
				if(strExt == ".bz2" || strExt == ".gz" || strExt == ".z")
					strExt = "." + tl::wstr_to_str(tl::get_fileext2(p.filename().native()));

				if(this->m_vecExts.size() == 0)
					return true;
				return std::find(this->m_vecExts.begin(), this->m_vecExts.end(),
					strExt) != this->m_vecExts.end();
			});

		for(const fs::path& d : lst)
			listFiles->addItem(tl::wstr_to_str(d.filename().native()).c_str());
	}
	catch(const std::exception& ex)
	{}
}



#ifndef NO_FIT

void ScanViewerDlg::ShowFitParams()
{
	focus_dlg(m_pFitParamDlg);
}


/**
 * fit a function to data points
 */
template<std::size_t iFuncArgs, class t_func>
bool ScanViewerDlg::Fit(t_func&& func,
	const std::vector<std::string>& vecParamNames,
	std::vector<t_real>& vecVals,
	std::vector<t_real>& vecErrs,
	const std::vector<bool>& vecFixed)
{
	m_vecFitX.clear();
	m_vecFitY.clear();

	if(std::min(m_vecX.size(), m_vecY.size()) == 0)
		return false;

	std::vector<t_real> m_vecYErr;
	m_vecYErr.reserve(m_vecY.size());
	for(t_real d : m_vecY)
	{
		if(tl::float_equal(d, t_real(0.), g_dEps))
			d = 1.;
		m_vecYErr.push_back(std::sqrt(d));
	}

	bool bOk = 0;
	try
	{
		std::vector<t_real_min>
			_vecVals = tl::container_cast<t_real_min, t_real, std::vector>()(vecVals),
			_vecErrs = tl::container_cast<t_real_min, t_real, std::vector>()(vecErrs);

		if(checkSwarm->isChecked())
		{
			bOk = tl::swarmfit<t_real, iFuncArgs>
				(func, m_vecX, m_vecY, m_vecYErr, vecParamNames, vecVals, vecErrs);
		}

		bOk = tl::fit<iFuncArgs>(func,
			tl::container_cast<t_real_min, t_real, std::vector>()(m_vecX),
			tl::container_cast<t_real_min, t_real, std::vector>()(m_vecY),
			tl::container_cast<t_real_min, t_real, std::vector>()(m_vecYErr),
			vecParamNames, _vecVals, _vecErrs, &vecFixed);
		vecVals = tl::container_cast<t_real, t_real_min, std::vector>()(_vecVals);
		vecErrs = tl::container_cast<t_real, t_real_min, std::vector>()(_vecErrs);
	}
	catch(const std::exception& ex)
	{
		tl::log_err(ex.what());
	}

	if(!bOk)
	{
		QMessageBox::critical(this, "Error", "Could not fit function. Please set or improve the initial parameters.");
		return false;
	}

	auto minmaxX = std::minmax_element(m_vecX.begin(), m_vecX.end());

	m_vecFitX.reserve(GFX_NUM_POINTS);
	m_vecFitY.reserve(GFX_NUM_POINTS);

	std::vector<t_real> vecValsWithX = { 0. };
	for(t_real dVal : vecVals) vecValsWithX.push_back(dVal);

	for(std::size_t i=0; i<GFX_NUM_POINTS; ++i)
	{
		t_real dX = t_real(i)*(*minmaxX.second - *minmaxX.first)/t_real(GFX_NUM_POINTS-1) + *minmaxX.first;
		vecValsWithX[0] = dX;
		t_real dY = tl::call<iFuncArgs, decltype(func), t_real, std::vector>(func, vecValsWithX);

		m_vecFitX.push_back(dX);
		m_vecFitY.push_back(dY);
	}

	PlotScan();
	m_pFitParamDlg->UnsetAllBold();
	return true;
}


void ScanViewerDlg::FitLine()
{
	if(std::min(m_vecX.size(), m_vecY.size()) == 0)
		return;

	auto func = [](t_real x, t_real m, t_real offs) -> t_real { return m*x + offs; };
	constexpr std::size_t iFuncArgs = 3;

	t_real_glob dSlope = m_pFitParamDlg->GetSlope(),	dSlopeErr = m_pFitParamDlg->GetSlopeErr();
	t_real_glob dOffs = m_pFitParamDlg->GetOffs(),		dOffsErr = m_pFitParamDlg->GetOffsErr();

	bool bSlopeFixed = m_pFitParamDlg->GetSlopeFixed();
	bool bOffsFixed = m_pFitParamDlg->GetOffsFixed();

	// automatic parameter determination
	if(!m_pFitParamDlg->WantParams())
	{
		auto minmaxX = std::minmax_element(m_vecX.begin(), m_vecX.end());
		auto minmaxY = std::minmax_element(m_vecY.begin(), m_vecY.end());

		dSlope = (*minmaxY.second - *minmaxY.first) / (*minmaxX.second - *minmaxX.first);
		dOffs = *minmaxY.first;

		dSlopeErr = dSlope * 0.1;
		dOffsErr = dOffs * 0.1;

		bSlopeFixed = bOffsFixed = 0;
	}

	std::vector<std::string> vecParamNames = { "slope", "offs" };
	std::vector<t_real> vecVals = { dSlope, dOffs };
	std::vector<t_real> vecErrs = { dSlopeErr, dOffsErr };
	std::vector<bool> vecFixed = { bSlopeFixed, bOffsFixed };

	if(!Fit<iFuncArgs>(func, vecParamNames, vecVals, vecErrs, vecFixed))
		return;

	for(t_real &d : vecErrs)
		d = std::abs(d);

	m_pFitParamDlg->SetSlope(vecVals[0]);	m_pFitParamDlg->SetSlopeErr(vecErrs[0]);
	m_pFitParamDlg->SetOffs(vecVals[1]);	m_pFitParamDlg->SetOffsErr(vecErrs[1]);
}


void ScanViewerDlg::FitSine()
{
	if(std::min(m_vecX.size(), m_vecY.size()) == 0)
		return;

	const bool bUseSlope = checkSloped->isChecked();

	auto func = [](t_real x, t_real amp, t_real freq, t_real phase, t_real offs) -> t_real
		{ return amp*std::sin(freq*x + phase) + offs; };
	constexpr std::size_t iFuncArgs = 5;

	auto funcSloped = [](t_real x, t_real amp, t_real freq, t_real phase, t_real offs, t_real slope) -> t_real
	{ return amp*std::sin(freq*x + phase) + slope*x + offs; };
	constexpr std::size_t iFuncArgsSloped = 6;


	t_real_glob dAmp = m_pFitParamDlg->GetAmp(),	dAmpErr = m_pFitParamDlg->GetAmpErr();
	t_real_glob dFreq = m_pFitParamDlg->GetFreq(),	dFreqErr = m_pFitParamDlg->GetFreqErr();
	t_real_glob dPhase = m_pFitParamDlg->GetPhase(),dPhaseErr = m_pFitParamDlg->GetPhaseErr();
	t_real_glob dOffs = m_pFitParamDlg->GetOffs(),	dOffsErr = m_pFitParamDlg->GetOffsErr();
	t_real_glob dSlope = m_pFitParamDlg->GetSlope(),	dSlopeErr = m_pFitParamDlg->GetSlopeErr();

	bool bAmpFixed = m_pFitParamDlg->GetAmpFixed();
	bool bFreqFixed = m_pFitParamDlg->GetFreqFixed();
	bool bPhaseFixed = m_pFitParamDlg->GetPhaseFixed();
	bool bOffsFixed = m_pFitParamDlg->GetOffsFixed();
	bool bSlopeFixed = m_pFitParamDlg->GetSlopeFixed();

	// automatic parameter determination
	if(!m_pFitParamDlg->WantParams())
	{
		auto minmaxX = std::minmax_element(m_vecX.begin(), m_vecX.end());
		auto minmaxY = std::minmax_element(m_vecY.begin(), m_vecY.end());

		dFreq = t_real(2.*M_PI) / (*minmaxX.second - *minmaxX.first);
		//dOffs = *minmaxY.first + (*minmaxY.second - *minmaxY.first)*0.5;
		dOffs = tl::mean_value(m_vecY);
		dAmp = (std::abs(*minmaxY.second - dOffs) + std::abs(dOffs - *minmaxY.first)) * 0.5;
		dPhase = 0.;

		dFreqErr = dFreq * 0.1;
		dOffsErr = tl::std_dev(m_vecY);;
		dAmpErr = dAmp * 0.1;
		dPhaseErr = M_PI;

		bAmpFixed = bFreqFixed = bPhaseFixed = bOffsFixed = 0;
	}

	std::vector<std::string> vecParamNames = { "amp", "freq", "phase",  "offs" };
	std::vector<t_real> vecVals = { dAmp, dFreq, dPhase, dOffs };
	std::vector<t_real> vecErrs = { dAmpErr, dFreqErr, dPhaseErr, dOffsErr };
	std::vector<bool> vecFixed = { bAmpFixed, bFreqFixed, bPhaseFixed, bOffsFixed };

	if(bUseSlope)
	{
		vecParamNames.push_back("slope");
		vecVals.push_back(dSlope);
		vecErrs.push_back(dSlopeErr);
		vecFixed.push_back(bSlopeFixed);
	}

	bool bOk = false;
	if(bUseSlope)
		bOk = Fit<iFuncArgsSloped>(funcSloped, vecParamNames, vecVals, vecErrs, vecFixed);
	else
		bOk = Fit<iFuncArgs>(func, vecParamNames, vecVals, vecErrs, vecFixed);
	if(!bOk)
		return;

	for(t_real &d : vecErrs)
		d = std::abs(d);

	m_pFitParamDlg->SetAmp(vecVals[0]);		m_pFitParamDlg->SetAmpErr(vecErrs[0]);
	m_pFitParamDlg->SetFreq(vecVals[1]);	m_pFitParamDlg->SetFreqErr(vecErrs[1]);
	m_pFitParamDlg->SetPhase(vecVals[2]);	m_pFitParamDlg->SetPhaseErr(vecErrs[2]);
	m_pFitParamDlg->SetOffs(vecVals[3]);	m_pFitParamDlg->SetOffsErr(vecErrs[3]);

	if(bUseSlope)
	{
		m_pFitParamDlg->SetSlope(vecVals[4]);
		m_pFitParamDlg->SetSlopeErr(vecErrs[4]);
	}
}


void ScanViewerDlg::FitGauss()
{
	if(std::min(m_vecX.size(), m_vecY.size()) == 0)
		return;

	const bool bUseSlope = checkSloped->isChecked();

	auto func = tl::gauss_model_amp<t_real>;
	auto funcSloped = tl::gauss_model_amp_slope<t_real>;
	constexpr std::size_t iFuncArgs = 5;
	constexpr std::size_t iFuncArgsSloped = iFuncArgs+1;

	t_real_glob dAmp = m_pFitParamDlg->GetAmp(),	dAmpErr = m_pFitParamDlg->GetAmpErr();
	t_real_glob dSig = m_pFitParamDlg->GetSig(),	dSigErr = m_pFitParamDlg->GetSigErr();
	t_real_glob dX0 = m_pFitParamDlg->GetX0(),		dX0Err = m_pFitParamDlg->GetX0Err();
	t_real_glob dOffs = m_pFitParamDlg->GetOffs(),	dOffsErr = m_pFitParamDlg->GetOffsErr();
	t_real_glob dSlope = m_pFitParamDlg->GetSlope(),	dSlopeErr = m_pFitParamDlg->GetSlopeErr();

	bool bAmpFixed = m_pFitParamDlg->GetAmpFixed();
	bool bSigFixed = m_pFitParamDlg->GetSigFixed();
	bool bX0Fixed = m_pFitParamDlg->GetX0Fixed();
	bool bOffsFixed = m_pFitParamDlg->GetOffsFixed();
	bool bSlopeFixed = m_pFitParamDlg->GetSlopeFixed();

	// automatic parameter determination
	if(!m_pFitParamDlg->WantParams())
	{
		auto minmaxX = std::minmax_element(m_vecX.begin(), m_vecX.end());
		auto minmaxY = std::minmax_element(m_vecY.begin(), m_vecY.end());

		dX0 = m_vecX[minmaxY.second - m_vecY.begin()];
		dSig = std::abs((*minmaxX.second-*minmaxX.first) * 0.5);
		dAmp = std::abs(*minmaxY.second-*minmaxY.first);
		dOffs = *minmaxY.first;

		dX0Err = dX0 * 0.1;
		dSigErr = dSig * 0.1;
		dAmpErr = dAmp * 0.1;
		dOffsErr = dOffs * 0.1;

		bAmpFixed = bSigFixed = bX0Fixed = bOffsFixed = 0;
	}

	std::vector<std::string> vecParamNames = { "x0", "sig", "amp", "offs" };
	std::vector<t_real> vecVals = { dX0, dSig, dAmp, dOffs };
	std::vector<t_real> vecErrs = { dX0Err, dSigErr, dAmpErr, dOffsErr };
	std::vector<bool> vecFixed = { bX0Fixed, bSigFixed, bAmpFixed, bOffsFixed };

	if(bUseSlope)
	{
		vecParamNames.push_back("slope");
		vecVals.push_back(dSlope);
		vecErrs.push_back(dSlopeErr);
		vecFixed.push_back(bSlopeFixed);
	}

	bool bOk = false;
	if(bUseSlope)
		bOk = Fit<iFuncArgsSloped>(funcSloped, vecParamNames, vecVals, vecErrs, vecFixed);
	else
		bOk = Fit<iFuncArgs>(func, vecParamNames, vecVals, vecErrs, vecFixed);
	if(!bOk)
		return;

	for(t_real &d : vecErrs) d = std::abs(d);
	vecVals[1] = std::abs(vecVals[1]);

	m_pFitParamDlg->SetX0(vecVals[0]);		m_pFitParamDlg->SetX0Err(vecErrs[0]);
	m_pFitParamDlg->SetSig(vecVals[1]);		m_pFitParamDlg->SetSigErr(vecErrs[1]);
	m_pFitParamDlg->SetAmp(vecVals[2]);		m_pFitParamDlg->SetAmpErr(vecErrs[2]);
	m_pFitParamDlg->SetOffs(vecVals[3]);	m_pFitParamDlg->SetOffsErr(vecErrs[3]);

	if(bUseSlope)
	{
		m_pFitParamDlg->SetSlope(vecVals[4]);
		m_pFitParamDlg->SetSlopeErr(vecErrs[4]);
	}
}


void ScanViewerDlg::FitLorentz()
{
	if(std::min(m_vecX.size(), m_vecY.size()) == 0)
		return;

	const bool bUseSlope = checkSloped->isChecked();

	auto func = tl::lorentz_model_amp<t_real>;
	auto funcSloped = tl::lorentz_model_amp_slope<t_real>;
	constexpr std::size_t iFuncArgs = 5;
	constexpr std::size_t iFuncArgsSloped = iFuncArgs+1;

	t_real_glob dAmp = m_pFitParamDlg->GetAmp(),	dAmpErr = m_pFitParamDlg->GetAmpErr();
	t_real_glob dHWHM = m_pFitParamDlg->GetHWHM(),	dHWHMErr = m_pFitParamDlg->GetHWHMErr();
	t_real_glob dX0 = m_pFitParamDlg->GetX0(),		dX0Err = m_pFitParamDlg->GetX0Err();
	t_real_glob dOffs = m_pFitParamDlg->GetOffs(),	dOffsErr = m_pFitParamDlg->GetOffsErr();
	t_real_glob dSlope = m_pFitParamDlg->GetSlope(),	dSlopeErr = m_pFitParamDlg->GetSlopeErr();

	bool bAmpFixed = m_pFitParamDlg->GetAmpFixed();
	bool bHWHMFixed = m_pFitParamDlg->GetHWHMFixed();
	bool bX0Fixed = m_pFitParamDlg->GetX0Fixed();
	bool bOffsFixed = m_pFitParamDlg->GetOffsFixed();
	bool bSlopeFixed = m_pFitParamDlg->GetSlopeFixed();

	// automatic parameter determination
	if(!m_pFitParamDlg->WantParams())
	{
		auto minmaxX = std::minmax_element(m_vecX.begin(), m_vecX.end());
		auto minmaxY = std::minmax_element(m_vecY.begin(), m_vecY.end());

		dX0 = m_vecX[minmaxY.second - m_vecY.begin()];
		dHWHM = std::abs((*minmaxX.second-*minmaxX.first) * 0.5);
		dAmp = std::abs(*minmaxY.second-*minmaxY.first);
		dOffs = *minmaxY.first;

		dX0Err = dX0 * 0.1;
		dHWHMErr = dHWHM * 0.1;
		dAmpErr = dAmp * 0.1;
		dOffsErr = dOffs * 0.1;

		bAmpFixed = bHWHMFixed = bX0Fixed = bOffsFixed = 0;
	}

	std::vector<std::string> vecParamNames = { "x0", "hwhm", "amp", "offs" };
	std::vector<t_real> vecVals = { dX0, dHWHM, dAmp, dOffs };
	std::vector<t_real> vecErrs = { dX0Err, dHWHMErr, dAmpErr, dOffsErr  };
	std::vector<bool> vecFixed = { bX0Fixed, bHWHMFixed, bAmpFixed, bOffsFixed };

	if(bUseSlope)
	{
		vecParamNames.push_back("slope");
		vecVals.push_back(dSlope);
		vecErrs.push_back(dSlopeErr);
		vecFixed.push_back(bSlopeFixed);
	}

	bool bOk = false;
	if(bUseSlope)
		bOk = Fit<iFuncArgsSloped>(funcSloped, vecParamNames, vecVals, vecErrs, vecFixed);
	else
		bOk = Fit<iFuncArgs>(func, vecParamNames, vecVals, vecErrs, vecFixed);
	if(!bOk)
		return;

	for(t_real &d : vecErrs) d = std::abs(d);
	vecVals[1] = std::abs(vecVals[1]);

	m_pFitParamDlg->SetX0(vecVals[0]);		m_pFitParamDlg->SetX0Err(vecErrs[0]);
	m_pFitParamDlg->SetHWHM(vecVals[1]);	m_pFitParamDlg->SetHWHMErr(vecErrs[1]);
	m_pFitParamDlg->SetAmp(vecVals[2]);		m_pFitParamDlg->SetAmpErr(vecErrs[2]);
	m_pFitParamDlg->SetOffs(vecVals[3]);	m_pFitParamDlg->SetOffsErr(vecErrs[3]);

	if(bUseSlope)
	{
		m_pFitParamDlg->SetSlope(vecVals[4]);
		m_pFitParamDlg->SetSlopeErr(vecErrs[4]);
	}
}


#ifndef HAS_COMPLEX_ERF

void ScanViewerDlg::FitVoigt() {}

#else

void ScanViewerDlg::FitVoigt()
{
	if(std::min(m_vecX.size(), m_vecY.size()) == 0)
		return;

	const bool bUseSlope = checkSloped->isChecked();

	auto func = tl::voigt_model_amp<t_real>;
	auto funcSloped = tl::voigt_model_amp_slope<t_real>;
	constexpr std::size_t iFuncArgs = 6;
	constexpr std::size_t iFuncArgsSloped = iFuncArgs+1;

	t_real_glob dAmp = m_pFitParamDlg->GetAmp(),	dAmpErr = m_pFitParamDlg->GetAmpErr();
	t_real_glob dSig = m_pFitParamDlg->GetSig(),	dSigErr = m_pFitParamDlg->GetSigErr();
	t_real_glob dHWHM = m_pFitParamDlg->GetHWHM(),	dHWHMErr = m_pFitParamDlg->GetHWHMErr();
	t_real_glob dX0 = m_pFitParamDlg->GetX0(),		dX0Err = m_pFitParamDlg->GetX0Err();
	t_real_glob dOffs = m_pFitParamDlg->GetOffs(),	dOffsErr = m_pFitParamDlg->GetOffsErr();
	t_real_glob dSlope = m_pFitParamDlg->GetSlope(),	dSlopeErr = m_pFitParamDlg->GetSlopeErr();

	bool bAmpFixed = m_pFitParamDlg->GetAmpFixed();
	bool bSigFixed = m_pFitParamDlg->GetSigFixed();
	bool bHWHMFixed = m_pFitParamDlg->GetHWHMFixed();
	bool bX0Fixed = m_pFitParamDlg->GetX0Fixed();
	bool bOffsFixed = m_pFitParamDlg->GetOffsFixed();
	bool bSlopeFixed = m_pFitParamDlg->GetSlopeFixed();

	// automatic parameter determination
	if(!m_pFitParamDlg->WantParams())
	{
		auto minmaxX = std::minmax_element(m_vecX.begin(), m_vecX.end());
		auto minmaxY = std::minmax_element(m_vecY.begin(), m_vecY.end());

		dX0 = m_vecX[minmaxY.second - m_vecY.begin()];
		dHWHM = std::abs((*minmaxX.second-*minmaxX.first) * 0.25);
		dSig = std::abs((*minmaxX.second-*minmaxX.first) * 0.25);
		dAmp = std::abs(*minmaxY.second-*minmaxY.first);
		dOffs = *minmaxY.first;

		dX0Err = dX0 * 0.1;
		dHWHMErr = dHWHM * 0.1;
		dAmpErr = dAmp * 0.1;
		dOffsErr = dOffs * 0.1;

		bAmpFixed = bHWHMFixed = bSigFixed = bX0Fixed = bOffsFixed = 0;
	}

	std::vector<std::string> vecParamNames = { "x0", "sig", "hwhm", "amp", "offs" };
	std::vector<t_real> vecVals = { dX0, dSig, dHWHM, dAmp, dOffs };
	std::vector<t_real> vecErrs = { dX0Err, dSigErr, dHWHMErr, dAmpErr, dOffsErr };
	std::vector<bool> vecFixed = { bX0Fixed, bSigFixed, bHWHMFixed, bAmpFixed, bOffsFixed };

	if(bUseSlope)
	{
		vecParamNames.push_back("slope");
		vecVals.push_back(dSlope);
		vecErrs.push_back(dSlopeErr);
		vecFixed.push_back(bSlopeFixed);
	}

	bool bOk = false;
	if(bUseSlope)
		bOk = Fit<iFuncArgsSloped>(funcSloped, vecParamNames, vecVals, vecErrs, vecFixed);
	else
		bOk = Fit<iFuncArgs>(func, vecParamNames, vecVals, vecErrs, vecFixed);
	if(!bOk)
		return;

	for(t_real &d : vecErrs) d = std::abs(d);
	vecVals[1] = std::abs(vecVals[1]);
	vecVals[2] = std::abs(vecVals[2]);

	m_pFitParamDlg->SetX0(vecVals[0]);		m_pFitParamDlg->SetX0Err(vecErrs[0]);
	m_pFitParamDlg->SetSig(vecVals[1]);		m_pFitParamDlg->SetSigErr(vecErrs[1]);
	m_pFitParamDlg->SetHWHM(vecVals[2]);	m_pFitParamDlg->SetHWHMErr(vecErrs[2]);
	m_pFitParamDlg->SetAmp(vecVals[3]);		m_pFitParamDlg->SetAmpErr(vecErrs[3]);
	m_pFitParamDlg->SetOffs(vecVals[4]);	m_pFitParamDlg->SetOffsErr(vecErrs[4]);

	if(bUseSlope)
	{
		m_pFitParamDlg->SetSlope(vecVals[5]);
		m_pFitParamDlg->SetSlopeErr(vecErrs[5]);
	}
}
#endif

#else	// NO_FIT

void ScanViewerDlg::ShowFitParams() {}
void ScanViewerDlg::FitGauss() {}
void ScanViewerDlg::FitLorentz() {}
void ScanViewerDlg::FitVoigt() {}
void ScanViewerDlg::FitLine() {}
void ScanViewerDlg::FitSine() {}

#endif


#include "scanviewer.moc"
