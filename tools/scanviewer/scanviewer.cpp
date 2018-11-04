/**
 * Scan viewer
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 2015-2018
 * @license GPLv2
 */

#include "scanviewer.h"

#include <QFileDialog>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QMessageBox>

#include <iostream>
#include <set>
#include <unordered_set>
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
#include "tlibs/file/file.h"
#include "tlibs/log/log.h"
#include "tlibs/helper/misc.h"
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
		m_vecExts({	".dat", ".DAT", ".scn", ".SCN", ".ng0", ".NG0", ".log", ".LOG", "" }),
		m_pFitParamDlg(new FitParamDlg(this, &m_settings))
{
	this->setupUi(this);
	this->setFocusPolicy(Qt::StrongFocus);
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
	QObject::connect(comboPath, &QComboBox::editTextChanged, pThis, &ScanViewerDlg::ChangedPath);
	QObject::connect(listFiles, &QListWidget::itemSelectionChanged, pThis, &ScanViewerDlg::FileSelected);
	QObject::connect(editSearch, &QLineEdit::textEdited, pThis, &ScanViewerDlg::SearchProps);
	QObject::connect(btnBrowse, &QToolButton::clicked, pThis, &ScanViewerDlg::SelectDir);
	for(QLineEdit* pEdit : {editPolVec1, editPolVec2, editPolCur1, editPolCur2})
		QObject::connect(pEdit, &QLineEdit::textEdited, pThis, &ScanViewerDlg::CalcPol);
#ifndef NO_FIT
	QObject::connect(btnParam, &QToolButton::clicked, pThis, &ScanViewerDlg::ShowFitParams);
	QObject::connect(btnGauss, &QToolButton::clicked, pThis, &ScanViewerDlg::FitGauss);
	QObject::connect(btnLorentz, &QToolButton::clicked, pThis, &ScanViewerDlg::FitLorentz);
	QObject::connect(btnVoigt, &QToolButton::clicked, pThis, &ScanViewerDlg::FitVoigt);
	QObject::connect(btnLine, &QToolButton::clicked, pThis, &ScanViewerDlg::FitLine);
	QObject::connect(btnParabola, &QToolButton::clicked, pThis, &ScanViewerDlg::FitParabola);
	QObject::connect(btnSine, &QToolButton::clicked, pThis, &ScanViewerDlg::FitSine);
#endif
	QObject::connect(comboX, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentIndexChanged), pThis, &ScanViewerDlg::XAxisSelected);
	QObject::connect(comboY, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentIndexChanged), pThis, &ScanViewerDlg::YAxisSelected);
	QObject::connect(comboMon, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::currentIndexChanged), pThis, &ScanViewerDlg::MonAxisSelected);
	QObject::connect(checkNorm, static_cast<void (QCheckBox::*)(int)>(&QCheckBox::stateChanged), pThis, &ScanViewerDlg::NormaliseStateChanged);
	//QObject::connect(checkLog, static_cast<void (QCheckBox::*)(int)>(&QCheckBox::stateChanged), pThis, &ScanViewerDlg::LogStateChanged);
	QObject::connect(spinStart, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), pThis, &ScanViewerDlg::StartOrSkipChanged);
	QObject::connect(spinStop, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), pThis, &ScanViewerDlg::StartOrSkipChanged);
	QObject::connect(spinSkip, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), pThis, &ScanViewerDlg::StartOrSkipChanged);
	QObject::connect(tableProps, &QTableWidget::currentItemChanged, pThis, &ScanViewerDlg::PropSelected);
	QObject::connect(comboExport, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), pThis, &ScanViewerDlg::GenerateExternal);
#else
	QObject::connect(comboPath, SIGNAL(editTextChanged(const QString&)),
		this, SLOT(ChangedPath()));
	QObject::connect(listFiles, SIGNAL(itemSelectionChanged()), this, SLOT(FileSelected()));
	QObject::connect(editSearch, SIGNAL(textEdited(const QString&)),
		this, SLOT(SearchProps(const QString&)));
	QObject::connect(btnBrowse, SIGNAL(clicked(bool)), this, SLOT(SelectDir()));
	for(QLineEdit* pEdit : {editPolVec1, editPolVec2, editPolCur1, editPolCur2})
		QObject::connect(pEdit, SIGNAL(textEdited(const QString&)), this, SLOT(CalcPol()));
#ifndef NO_FIT
	QObject::connect(btnParam, SIGNAL(clicked(bool)), this, SLOT(ShowFitParams()));
	QObject::connect(btnGauss, SIGNAL(clicked(bool)), this, SLOT(FitGauss()));
	QObject::connect(btnLorentz, SIGNAL(clicked(bool)), this, SLOT(FitLorentz()));
	QObject::connect(btnVoigt, SIGNAL(clicked(bool)), this, SLOT(FitVoigt()));
	QObject::connect(btnLine, SIGNAL(clicked(bool)), this, SLOT(FitLine()));
	QObject::connect(btnParabola, SIGNAL(clicked(bool)), this, SLOT(FitParabola()));
	QObject::connect(btnSine, SIGNAL(clicked(bool)), this, SLOT(FitSine()));
#endif
	QObject::connect(comboX, SIGNAL(currentIndexChanged(const QString&)),
		this, SLOT(XAxisSelected(const QString&)));
	QObject::connect(comboY, SIGNAL(currentIndexChanged(const QString&)),
		this, SLOT(YAxisSelected(const QString&)));
	QObject::connect(comboMon, SIGNAL(currentIndexChanged(const QString&)),
		this, SLOT(MonAxisSelected(const QString&)));
	QObject::connect(checkNorm, SIGNAL(stateChanged(int)), this, SLOT(NormaliseStateChanged(int)));
	//QObject::connect(checkLog, SIGNAL(stateChanged(int)), this, SLOT(LogStateChanged(int)));
	QObject::connect(spinStart, SIGNAL(valueChanged(int)), this, SLOT(StartOrSkipChanged(int)));
	QObject::connect(spinStop, SIGNAL(valueChanged(int)), this, SLOT(StartOrSkipChanged(int)));
	QObject::connect(spinSkip, SIGNAL(valueChanged(int)), this, SLOT(StartOrSkipChanged(int)));
	QObject::connect(tableProps, SIGNAL(currentItemChanged(QTableWidgetItem*, QTableWidgetItem*)),
		this, SLOT(PropSelected(QTableWidgetItem*, QTableWidgetItem*)));
	QObject::connect(comboExport, SIGNAL(currentIndexChanged(int)),
		this, SLOT(GenerateExternal(int)));
	//QObject::connect(btnOpenExt, SIGNAL(clicked()), this, SLOT(openExternally()));
#endif


	// fill recent paths combobox
	QStringList lstDirs = m_settings.value("recent_dirs").toStringList();
	for(const QString& strDir : lstDirs)
	{
		fs::path dir(strDir.toStdString());
		if(*tl::wstr_to_str(dir.native()).rbegin() != fs::path::preferred_separator)
			dir /= std::string("")+fs::path::preferred_separator;

		if(HasRecentPath(strDir) < 0)
			comboPath->addItem(tl::wstr_to_str(dir.native()).c_str());
	}

	// last selected dir
	QString strDir = m_settings.value("last_dir", tl::wstr_to_str(fs::current_path().native()).c_str()).toString();

	int idx = HasRecentPath(strDir);
	if(idx < 0)
	{
		fs::path dir(strDir.toStdString());
		if(*tl::wstr_to_str(dir.native()).rbegin() != fs::path::preferred_separator)
			dir /= std::string("")+fs::path::preferred_separator;

		comboPath->addItem(tl::wstr_to_str(dir.native()).c_str());
		idx = comboPath->findText(strDir);
	}

	comboPath->setCurrentIndex(idx);
	//comboPath->setEditText(strDir);


	if(m_settings.contains("pol/vec1"))
		editPolVec1->setText(m_settings.value("pol/vec1").toString());
	if(m_settings.contains("pol/vec2"))
		editPolVec2->setText(m_settings.value("pol/vec2").toString());
	if(m_settings.contains("pol/cur1"))
		editPolCur1->setText(m_settings.value("pol/cur1").toString());
	if(m_settings.contains("pol/cur2"))
		editPolCur2->setText(m_settings.value("pol/cur2").toString());

	m_bDoUpdate = 1;
	ChangedPath();

#ifdef NO_FIT
	btnParam->setEnabled(false);
	btnGauss->setEnabled(false);
	btnLorentz->setEnabled(false);
	btnVoigt->setEnabled(false);
	btnLine->setEnabled(false);
	btnParabola->setEnabled(false);
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
	// save settings
	m_settings.setValue("pol/vec1", editPolVec1->text());
	m_settings.setValue("pol/vec2", editPolVec2->text());
	m_settings.setValue("pol/cur1", editPolCur1->text());
	m_settings.setValue("pol/cur2", editPolCur2->text());
	m_settings.setValue("geo", saveGeometry());
	m_settings.setValue("last_dir", QString(m_strCurDir.c_str()));

	QStringList lstDirs;
	for(int iDir=0; iDir<comboPath->count(); ++iDir)
	{
		QString qstrPath = comboPath->itemText(iDir);

		// clean up recent path list
		fs::path dir(qstrPath.toStdString());
		if(!fs::exists(dir) || !fs::is_directory(dir) || fs::is_empty(dir))
			continue;

		lstDirs << qstrPath;
	}
	m_settings.setValue("recent_dirs", lstDirs);

	QDialog::closeEvent(pEvt);
}


void ScanViewerDlg::keyPressEvent(QKeyEvent* pEvt)
{
	if(pEvt->key() == Qt::Key_R)
	{
		tl::log_debug("Refreshing file list...");
		ChangedPath();
	}

	QDialog::keyPressEvent(pEvt);
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
	m_vecYErr.clear();
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
	comboMon->clear();
	textRoot->clear();
	spinStart->setValue(0);
	spinStop->setValue(0);
	spinSkip->setValue(0);

	m_plotwrap->GetPlot()->replot();
}


/**
 * check if given path is already in the "recent paths" list
 */
int ScanViewerDlg::HasRecentPath(const QString& strPath)
{
	fs::path dir(strPath.toStdString());
	if(*tl::wstr_to_str(dir.native()).rbegin() != fs::path::preferred_separator)
		dir /= std::string("")+fs::path::preferred_separator;

	for(int iDir=0; iDir<comboPath->count(); ++iDir)
	{
		QString strOtherPath = comboPath->itemText(iDir);
		fs::path dirOther(strOtherPath.toStdString());
		if(*tl::wstr_to_str(dirOther.native()).rbegin() != fs::path::preferred_separator)
			dirOther /= std::string("")+fs::path::preferred_separator;

		if(dir == dirOther)
			return iDir;
	}

	return -1;
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
		int idx = HasRecentPath(strDir);
		if(idx < 0)
		{
			fs::path dir(strDir.toStdString());
			if(*tl::wstr_to_str(dir.native()).rbegin() != fs::path::preferred_separator)
				dir /= std::string("")+fs::path::preferred_separator;

			comboPath->addItem(tl::wstr_to_str(dir.native()).c_str());
			idx = comboPath->findText(tl::wstr_to_str(dir.native()).c_str());
		}
		comboPath->setCurrentIndex(idx);
		//comboPath->setEditText(strDir);
		ChangedPath();
	}
}


void ScanViewerDlg::XAxisSelected(const QString& strLab) { PlotScan(); }
void ScanViewerDlg::YAxisSelected(const QString& strLab) { PlotScan(); }
void ScanViewerDlg::MonAxisSelected(const QString& strLab) { PlotScan(); }
void ScanViewerDlg::NormaliseStateChanged(int iState) { PlotScan(); }
//void ScanViewerDlg::LogStateChanged(int iState) { PlotScan(); }
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
	std::string strMonVar = m_pInstr->GetMonVar();
	//tl::log_info("Count var: ", strCntVar, ", mon var: ", strMonVar);

	const std::wstring strPM = tl::get_spec_char_utf16("pm");

	m_bDoUpdate = 0;
	int iIdxX=-1, iIdxY=-1, iIdxMon=-1, iCurIdx=0;
	int iAlternateX = -1;
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
		comboMon->addItem(QString::fromWCharArray(_strCol.c_str()), QString(strCol.c_str()));

		std::string strFirstScanVar = tl::str_to_lower(vecScanVars[0]);
		std::string strColLower = tl::str_to_lower(strCol);

		if(vecScanVars.size() && strFirstScanVar == strColLower)
			iIdxX = iCurIdx;
		if(vecScanVars.size() && strFirstScanVar.substr(0, strCol.length()) == strColLower)
			iIdxX = iCurIdx;
		if(tl::str_to_lower(strCntVar) == strColLower)
			iIdxY = iCurIdx;
		if(tl::str_to_lower(strMonVar) == strColLower)
			iIdxMon = iCurIdx;

		++iCurIdx;
	}

	if(iIdxX < 0 && iAlternateX >= 0)
		iIdxX = iAlternateX;
	comboX->setCurrentIndex(iIdxX);
	comboY->setCurrentIndex(iIdxY);
	comboMon->setCurrentIndex(iIdxMon);

	CalcPol();

	int iNumPol = static_cast<int>(m_pInstr->NumPolChannels()) - 1;
	if(iNumPol < 0) iNumPol = 0;
	spinSkip->setValue(iNumPol);

	m_bDoUpdate = 1;

	ShowProps();
	PlotScan();
}


/**
 * polarisation device named changed
 */
void ScanViewerDlg::CalcPol()
{
	editPolMat->clear();
	if(!m_pInstr)
		return;

	const std::string strPolVec1 = editPolVec1->text().toStdString();
	const std::string strPolVec2 = editPolVec2->text().toStdString();
	const std::string strPolCur1 = editPolCur1->text().toStdString();
	const std::string strPolCur2 = editPolCur2->text().toStdString();

	m_pInstr->SetPolNames(strPolVec1.c_str(), strPolVec2.c_str(), strPolCur1.c_str(), strPolCur2.c_str());
	m_pInstr->ParsePolData();

	const std::vector<std::array<t_real, 6>>& vecPolStates = m_pInstr->GetPolStates();
	const std::size_t iNumPolStates = vecPolStates.size();
	if(iNumPolStates == 0)
	{
		editPolMat->setHtml("<html><body><font size=\"5\" color=\"#ff0000\">No polarisation data.</font></body></html>");
		return;
	}


	// get the SF state to a given NSF state
	auto find_spinflip_state_idx = [&vecPolStates, iNumPolStates]
		(const std::array<t_real, 6>& state) -> std::size_t
	{
		t_real dPix1 = state[0], dPiy1 = state[1], dPiz1 = state[2];
		t_real dPfx1 = state[3], dPfy1 = state[4], dPfz1 = state[5];

		for(std::size_t iPol=0; iPol<iNumPolStates; ++iPol)
		{
			t_real dPix2 = vecPolStates[iPol][0];
			t_real dPiy2 = vecPolStates[iPol][1];
			t_real dPiz2 = vecPolStates[iPol][2];
			t_real dPfx2 = vecPolStates[iPol][3];
			t_real dPfy2 = vecPolStates[iPol][4];
			t_real dPfz2 = vecPolStates[iPol][5];

			if(tl::float_equal(dPix1, dPix2, g_dEps) &&
				tl::float_equal(dPiy1, dPiy2, g_dEps) &&
				tl::float_equal(dPiz1, dPiz2, g_dEps) &&
				tl::float_equal(dPfx1, -dPfx2, g_dEps) &&
				tl::float_equal(dPfy1, -dPfy2, g_dEps) &&
				tl::float_equal(dPfz1, -dPfz2, g_dEps))
			{
				return iPol;
			}
		}

		// none found
		return iNumPolStates;
	};


	auto propagate_err = [](t_real x, t_real y, t_real dx, t_real dy) -> t_real
	{
		// d((x-y)/(x+y)) = dx * 2*y/(x+y)^2 - dy * 2*x/(x+y)^2
		return std::sqrt(dx*2.*y/((x+y)*(x+y))*dx*2.*y/((x+y)*(x+y))
			+ dy*2.*x/((x+y)*(x+y))*dy*2.*x/((x+y)*(x+y)));
	};


	// convert polarisation vector to string representation
	auto polvec_str = [](t_real x, t_real y, t_real z) -> std::string
	{
		std::ostringstream ostr;
		ostr.precision(g_iPrec);

		if(tl::float_equal<t_real>(x, 1., g_dEps) &&
			tl::float_equal<t_real>(y, 0., g_dEps) &&
			tl::float_equal<t_real>(z, 0., g_dEps))
			ostr << "x";
		else if(tl::float_equal<t_real>(x, -1., g_dEps) &&
			tl::float_equal<t_real>(y, 0., g_dEps) &&
			tl::float_equal<t_real>(z, 0., g_dEps))
			ostr << "-x";
		else if(tl::float_equal<t_real>(x, 0., g_dEps) &&
			tl::float_equal<t_real>(y, 1., g_dEps) &&
			tl::float_equal<t_real>(z, 0., g_dEps))
			ostr << "y";
		else if(tl::float_equal<t_real>(x, 0., g_dEps) &&
			tl::float_equal<t_real>(y, -1., g_dEps) &&
			tl::float_equal<t_real>(z, 0., g_dEps))
			ostr << "-y";
		else if(tl::float_equal<t_real>(x, 0., g_dEps) &&
			tl::float_equal<t_real>(y, 0., g_dEps) &&
			tl::float_equal<t_real>(z, 1., g_dEps))
			ostr << "z";
		else if(tl::float_equal<t_real>(x, 0., g_dEps) &&
			tl::float_equal<t_real>(y, 0., g_dEps) &&
			tl::float_equal<t_real>(z, -1., g_dEps))
			ostr << "-z";
		else
			ostr << "[" << x << " " << y << " " << z << "]";

		return ostr.str();
	};


	std::vector<bool> vecHasSFPartner;
	// indices to spin-flipped states
	std::vector<std::size_t> vecSFIdx;

	for(std::size_t iPol=0; iPol<iNumPolStates; ++iPol)
	{
		const std::array<t_real, 6>& state = vecPolStates[iPol];
		std::size_t iIdx = find_spinflip_state_idx(state);

		vecHasSFPartner.push_back(iIdx < iNumPolStates);
		vecSFIdx.push_back(iIdx);
	}


	const std::vector<std::string> vecScanVars = m_pInstr->GetScannedVars();
	std::string strX;
	if(vecScanVars.size())
		strX = vecScanVars[0];
	const std::vector<t_real>& vecX = m_pInstr->GetCol(strX.c_str());
	const std::vector<t_real>& vecCnts = m_pInstr->GetCol(m_pInstr->GetCountVar().c_str());


	// raw counts per polarisation channel
	std::ostringstream ostrCnts;
	ostrCnts.precision(g_iPrec);
	ostrCnts << "<p><h2>Counts in Polarisation Channels</h2>";

	// iterate over scan points
	for(std::size_t iPt=0; iPt<vecCnts.size();)
	{
		ostrCnts << "<p><b>Scan Point " << (iPt/iNumPolStates+1);
		if(strX != "" && iPt < vecX.size())
			ostrCnts << " (" << strX << " = " << vecX[iPt + 0] << ")";
		ostrCnts << "</b>";
		ostrCnts << "<table border=\"1\" cellpadding=\"0\">";
		ostrCnts << "<tr><th>Init. Pol. Vec.</th>";
		ostrCnts << "<th>Fin. Pol. Vec.</th>";
		ostrCnts << "<th>Counts</th>";
		ostrCnts << "<th>Error</th></tr>";

		// iterate over polarisation states
		for(std::size_t iPol=0; iPol<iNumPolStates; ++iPol, ++iPt)
		{
			t_real dPix = vecPolStates[iPol][0];
			t_real dPiy = vecPolStates[iPol][1];
			t_real dPiz = vecPolStates[iPol][2];

			t_real dPfx = vecPolStates[iPol][3];
			t_real dPfy = vecPolStates[iPol][4];
			t_real dPfz = vecPolStates[iPol][5];

			std::size_t iCnts = std::size_t(vecCnts[iPt]);
			t_real dErr = (iCnts==0 ? 1 : std::sqrt(vecCnts[iPt]));

			ostrCnts << "<tr><td>" << polvec_str(dPix, dPiy, dPiz) << "</td>"
				<< "<td>" << polvec_str(dPfx, dPfy, dPfz) << "</td>"
				<< "<td><b>" << iCnts << "</b></td>"
				<< "<td><b>" << dErr << "</b></td>"
				<< "</tr>";
		}
		ostrCnts << "</table></p>";
	}
	ostrCnts << "</p>";


	// polarisation matrix elements
	std::ostringstream ostrPol;
	ostrPol.precision(g_iPrec);
	ostrPol << "<p><h2>Polarisation Matrix Elements</h2>";
	bool bHasAnyData = false;

	// iterate over scan points
	for(std::size_t iPt=0; iPt<vecCnts.size()/iNumPolStates; ++iPt)
	{
		ostrPol << "<p><b>Scan Point " << (iPt+1);
		if(strX != "" && iPt*iNumPolStates < vecX.size())
			ostrPol << " (" << strX << " = " << vecX[iPt*iNumPolStates + 0] << ")";
		ostrPol << "</b>";
		ostrPol << "<table border=\"1\" cellpadding=\"0\">";
		ostrPol << "<tr><th>Index 1</th>";
		ostrPol << "<th>Index 2</th>";
		ostrPol << "<th>Polarisation</th>";
		ostrPol << "<th>Error</th></tr>";

		// iterate over all polarisation states which have a SF partner
		std::unordered_set<std::size_t> setPolAlreadySeen;
		for(std::size_t iPol=0; iPol<iNumPolStates; ++iPol)
		{
			if(!vecHasSFPartner[iPol]) continue;
			if(setPolAlreadySeen.find(iPol) != setPolAlreadySeen.end())
				continue;

			const std::size_t iSF = vecSFIdx[iPol];
			const std::array<t_real, 6>& state = vecPolStates[iPol];
			//const std::array<t_real, 6>& stateSF = vecPolStates[iSF];

			setPolAlreadySeen.insert(iPol);
			setPolAlreadySeen.insert(iSF);

			const t_real dCntsNSF = vecCnts[iPt*iNumPolStates + iPol];
			const t_real dCntsSF = vecCnts[iPt*iNumPolStates + iSF];
			t_real dNSFErr = std::sqrt(dCntsNSF);
			t_real dSFErr = std::sqrt(dCntsSF);
			if(tl::float_equal(dCntsNSF, t_real(0), g_dEps))
				dNSFErr = 1.;
			if(tl::float_equal(dCntsSF, t_real(0), g_dEps))
				dSFErr = 1.;

			bool bInvalid = tl::float_equal(dCntsNSF+dCntsSF, t_real(0), g_dEps);
			t_real dPolElem = 0., dPolErr = 1.;
			if(!bInvalid)
			{
				dPolElem = /*std::abs*/((dCntsSF-dCntsNSF) / (dCntsSF+dCntsNSF));
				dPolErr = propagate_err(dCntsNSF, dCntsSF, dNSFErr, dSFErr);
			}

			// polarisation matrix elements, e.g. <[100] | P | [010]> = <x|P|y>
			ostrPol << "<tr><td>" << polvec_str(state[0], state[1], state[2]) << "</td>"
				<< "<td>" << polvec_str(state[3], state[4], state[5]) << "</td>"
				<< "<td><b>" << (bInvalid ? "--- ": tl::var_to_str(dPolElem, g_iPrec)) << "</b></td>"
				<< "<td><b>" << (bInvalid ? "--- ": tl::var_to_str(dPolErr, g_iPrec)) << "</b></td>"
				<< "</tr>";

			bHasAnyData = true;
		}
		ostrPol << "</table></p>";
	}
	ostrPol << "</p>";

	if(!bHasAnyData)
		ostrPol << "<font size=\"5\" color=\"#ff0000\">Insufficient Data.</font>";

	std::string strHtml = "<html><body>" + ostrPol.str() + "<br><hr><br>"
		+ ostrCnts.str() + "</body></html>";
	editPolMat->setHtml(QString::fromUtf8(strHtml.c_str()));
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

	bool bNormalise = checkNorm->isChecked();
	//const bool bLog = checkLog->isChecked();

	m_strX = comboX->itemData(comboX->currentIndex(), Qt::UserRole).toString().toStdString();
	m_strY = comboY->itemData(comboY->currentIndex(), Qt::UserRole).toString().toStdString();
	m_strMon = comboMon->itemData(comboMon->currentIndex(), Qt::UserRole).toString().toStdString();

	const int iStartIdx = spinStart->value();
	const int iEndSkip = spinStop->value();
	const int iSkipRows = spinSkip->value();
	const std::string strTitle = m_pInstr->GetTitle();
	m_strCmd = m_pInstr->GetScanCommand();

	m_vecX = m_pInstr->GetCol(m_strX.c_str());
	m_vecY = m_pInstr->GetCol(m_strY.c_str());
	std::vector<t_real> vecMon = m_pInstr->GetCol(m_strMon.c_str());

	bool bYIsACountVar = (m_strY == m_pInstr->GetCountVar() || m_strY == m_pInstr->GetMonVar());
	m_plotwrap->GetCurve(1)->SetShowErrors(bYIsACountVar);


	// remove points from start
	if(iStartIdx != 0)
	{
		if(std::size_t(iStartIdx) >= m_vecX.size())
			m_vecX.clear();
		else
			m_vecX.erase(m_vecX.begin(), m_vecX.begin()+iStartIdx);

		if(std::size_t(iStartIdx) >= m_vecY.size())
		{
			m_vecY.clear();
			vecMon.clear();
		}
		else
		{
			m_vecY.erase(m_vecY.begin(), m_vecY.begin()+iStartIdx);
			vecMon.erase(vecMon.begin(), vecMon.begin()+iStartIdx);
		}
	}

	// remove points from end
	if(iEndSkip != 0)
	{
		if(std::size_t(iEndSkip) >= m_vecX.size())
			m_vecX.clear();
		else
			m_vecX.erase(m_vecX.end()-iEndSkip, m_vecX.end());

		if(std::size_t(iEndSkip) >= m_vecY.size())
		{
			m_vecY.clear();
			vecMon.clear();
		}
		else
		{
			m_vecY.erase(m_vecY.end()-iEndSkip, m_vecY.end());
			vecMon.erase(vecMon.end()-iEndSkip, vecMon.end());
		}
	}

	// interleave rows
	if(iSkipRows != 0)
	{
		decltype(m_vecX) vecXNew, vecYNew, vecMonNew;

		for(std::size_t iRow=0; iRow<std::min(m_vecX.size(), m_vecY.size()); ++iRow)
		{
			vecXNew.push_back(m_vecX[iRow]);
			vecYNew.push_back(m_vecY[iRow]);
			vecMonNew.push_back(vecMon[iRow]);

			iRow += iSkipRows;
		}

		m_vecX = std::move(vecXNew);
		m_vecY = std::move(vecYNew);
		vecMon = std::move(vecMonNew);
	}


	// errors
	if(vecMon.size() < m_vecY.size())
	{
		bNormalise = 0;
		tl::log_err("Counter and monitor data count mismatch.");
	}

	m_vecYErr.clear();
	m_vecYErr.reserve(m_vecY.size());
	for(std::size_t iY=0; iY<m_vecY.size(); ++iY)
	{
		// normalise to monitor?
		if(bNormalise)
		{
			t_real y = m_vecY[iY];
			t_real m = vecMon[iY];
			t_real dy = tl::float_equal(y, 0., g_dEps) ? 1. : std::sqrt(y);
			t_real dm = tl::float_equal(m, 0., g_dEps) ? 1. : std::sqrt(m);

			// y_new = y/m
			// dy_new = 1/m dy - y/m^2 dm
			m_vecY[iY] = y/m;
			m_vecYErr.push_back(std::sqrt(std::pow(dy/m, 2.) + std::pow(dm*y/(m*m), 2.)));
		}
		else
		{
			t_real err = tl::float_equal(m_vecY[iY], 0., g_dEps) ? 1. : std::sqrt(m_vecY[iY]);
			m_vecYErr.push_back(err);
		}
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


	QString strY = m_strY.c_str();
	if(bNormalise)
	{
		strY += " / ";
		strY += m_strMon.c_str();
	}
	plot->setAxisTitle(QwtPlot::xBottom, m_strX.c_str());
	plot->setAxisTitle(QwtPlot::yLeft, strY);
	plot->setTitle(m_strCmd.c_str());


	//if(m_vecX.size()==0 || m_vecY.size()==0)
	//	return;

	if(m_vecFitX.size())
		set_qwt_data<t_real>()(*m_plotwrap, m_vecFitX, m_vecFitY, 0, 0);
	else
		set_qwt_data<t_real>()(*m_plotwrap, m_vecX, m_vecY, 0, 0);
	set_qwt_data<t_real>()(*m_plotwrap, m_vecX, m_vecY, 1, 1, &m_vecYErr);

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
R"RAWSTR(#
# Created with Takin version %%TAKIN_VER%%.
#

# --------------------------------------------------------------------------------
# choose an output terminal
#set term wxt
set term qt
#set term pdf color enhanced font "Helvetica, 14" size 4,3.5
#set output "plot.pdf"
# --------------------------------------------------------------------------------


# --------------------------------------------------------------------------------
# parameters
maxx = %%MAXX%%
minx = %%MINX%%
maxy = %%MAXY%%
miny = %%MINY%%
rangex = maxx - minx
rangey = maxy - miny

rangex_tics = rangex / 5.
rangey_tics = rangey / 5.
# --------------------------------------------------------------------------------


# --------------------------------------------------------------------------------
# data points
$datapoints << ENDPTS
%%POINTS%%
ENDPTS
# --------------------------------------------------------------------------------


# --------------------------------------------------------------------------------
# functions for fitting
gauss(x, a, s, x0, y0) = a*exp(-0.5 * ((x-x0)/s)**2.) + y0
gauss2(x, a1,s1,x01, a2,s2,x02, y0) = \
	gauss(x, a1,s1,x01, 0) + \
	gauss(x, a2,s2,x02, 0) + \
	y0
gauss3(x, a1,s1,x01, a2,s2,x02, a3,s3,x03, y0) = \
	gauss(x, a1,s1,x01, 0) + \
	gauss(x, a2,s2,x02, 0) + \
	gauss(x, a3,s3,x03, 0) + \
	y0

lorentz(x, a, h, x0, y0) = a*h**2. / ((x-x0)**2. + h**2.) + y0
lorentz2(x, a1,h1,x01, a2,h2,x02, y0) = \
	lorentz(x, a1,h1,x01, 0) + \
	lorentz(x, a2,h2,x02, 0) + \
	y0
lorentz3(x, a1,h1,x01, a2,h2,x02, a3,h3,x03, y0) = \
	lorentz(x, a1,h1,x01, 0) + \
	lorentz(x, a2,h2,x02, 0) + \
	lorentz(x, a3,h3,x03, 0) + \
	y0

parabola(x, a, x0, y0) = a * (x-x0)**2. + y0
line(x, m, y0) = m*x + y0
sine(x, a, f, p, y0) = a*sin(f*x + p) + y0
# --------------------------------------------------------------------------------


# --------------------------------------------------------------------------------
# fitting

# initial guesses for gauss and others ...
a1 = rangey
s1 = rangex*0.5		# sigma
x01 = minx + rangex*0.5
y0 = miny

# ... and for line
m1 = rangey / rangex

# ... and for sine
f1 = 1.		# frequency
p1 = 0.		# phase

# ... and for lorentz
h1 = rangex*0.5		# HWHM

# actual fitting
#fit line(x, m1,y0) "-" using ($1):($2):($3) yerrors via m1,y0
#fit parabola(x, a1,x01, y0) "-" using ($1):($2):($3) yerrors via a1,x01,y0
#fit sine(x, a1,f1,p1, y0) "-" using ($1):($2):($3) yerrors via a1,h1,x01,y0
#fit lorentz(x, a1,h1,x01, y0) "-" using ($1):($2):($3) yerrors via a1,h1,x01,y0
fit gauss(x, a1,s1,x01, y0) "$datapoints" using ($1):($2):($3) yerrors via a1,s1,x01,y0
# --------------------------------------------------------------------------------


# --------------------------------------------------------------------------------
# plotting
line1_col = "#000000"
points1_col = "#000000"

set xlabel "%%LABELX%%"
set ylabel "%%LABELY%%"
set title "%%TITLE%%"
set grid
#set key top right Left width 0 samplen 3 spacing 1.2

set xrange [minx : maxx]
set yrange [miny : maxy]

#set xtics rangex_tics
#set ytics rangey_tics
#set mxtics 2
#set mytics 2

# show fit results
sig2fwhm = 2.*sqrt(2.*log(2.))
fitres = sprintf("x0 = %.4g\nFWHM = %.4g", x01, sig2fwhm*s1)
set label at graph 0.05, graph 0.95 fitres

# use these functions with the plot command below
#	line(x,m1,y0) with lines linewidth 2 linecolor rgb line1_col notitle, \
#	parabola(x,a1,x01,y0) with lines linewidth 2 linecolor rgb line1_col notitle, \
#	sine(x,a1,f1,p1,y0) with lines linewidth 2 linecolor rgb line1_col notitle, \
#	lorentz(x,a1,h1,x01,y0) with lines linewidth 2 linecolor rgb line1_col notitle, \

plot \
	gauss(x,a1,s1,x01,y0) with lines linewidth 2 linecolor rgb line1_col notitle, \
	"$datapoints" using ($1):($2):($3) pointtype 7 pointsize 1 linecolor rgb points1_col with yerrorbars title "Data"
# --------------------------------------------------------------------------------
)RAWSTR";


	auto minmaxX = std::minmax_element(m_vecX.begin(), m_vecX.end());
	auto minmaxY = std::minmax_element(m_vecY.begin(), m_vecY.end());
	t_real dMaxErrY = *std::max_element(m_vecYErr.begin(), m_vecYErr.end());

	std::ostringstream ostrPoints;
	ostrPoints.precision(g_iPrec);

	for(std::size_t i=0; i<std::min(m_vecX.size(), m_vecY.size()); ++i)
	{
		ostrPoints
			<< std::left << std::setw(g_iPrec*2) << m_vecX[i] << " "
			<< std::left << std::setw(g_iPrec*2) << m_vecY[i] << " "
			<< std::left << std::setw(g_iPrec*2) << m_vecYErr[i] << "\n";
	}

	tl::find_all_and_replace<std::string>(strPySrc, "%%MINX%%", tl::var_to_str(*minmaxX.first, g_iPrec));
	tl::find_all_and_replace<std::string>(strPySrc, "%%MAXX%%", tl::var_to_str(*minmaxX.second, g_iPrec));
	tl::find_all_and_replace<std::string>(strPySrc, "%%MINY%%", tl::var_to_str(*minmaxY.first-dMaxErrY, g_iPrec));
	tl::find_all_and_replace<std::string>(strPySrc, "%%MAXY%%", tl::var_to_str(*minmaxY.second+dMaxErrY, g_iPrec));
	tl::find_all_and_replace<std::string>(strPySrc, "%%TITLE%%", strTitle);
	tl::find_all_and_replace<std::string>(strPySrc, "%%LABELX%%", strLabelX);
	tl::find_all_and_replace<std::string>(strPySrc, "%%LABELY%%", strLabelY);
	tl::find_all_and_replace<std::string>(strPySrc, "%%POINTS%%", ostrPoints.str());
	tl::find_all_and_replace<std::string>(strPySrc, "%%TAKIN_VER%%", TAKIN_VER);

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
R"RAWSTR(#
# Created with Takin version %%TAKIN_VER%%.
#

import numpy as np

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


	auto minmaxX = std::minmax_element(m_vecX.begin(), m_vecX.end());
	auto minmaxY = std::minmax_element(m_vecY.begin(), m_vecY.end());
	t_real dMaxErrY = *std::max_element(m_vecYErr.begin(), m_vecYErr.end());

	std::ostringstream ostrX, ostrY, ostrYErr;
	ostrX.precision(g_iPrec);
	ostrY.precision(g_iPrec);
	ostrYErr.precision(g_iPrec);

	for(std::size_t i=0; i<std::min(m_vecX.size(), m_vecY.size()); ++i)
	{
		ostrX << m_vecX[i] << ", ";
		ostrY << m_vecY[i] << ", ";
		ostrYErr << m_vecYErr[i] << ", ";
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
	tl::find_and_replace<std::string>(strPySrc, "%%TAKIN_VER%%", TAKIN_VER);

	textRoot->setText(strPySrc.c_str());
}


/**
 * convert to hermelin
 */
void ScanViewerDlg::GenerateForHermelin()
{
    std::string strStoatSrc =
R"RAWSTR(#!./hermelin -t

#
# Created with Takin version %%TAKIN_VER%%.
#

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
	tl::find_and_replace<std::string>(strStoatSrc, "%%TAKIN_VER%%", TAKIN_VER);

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
R"RAWSTR(/*
 * Created with Takin version %%TAKIN_VER%%.
 */

void scan_plot()
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


	auto minmaxX = std::minmax_element(m_vecX.begin(), m_vecX.end());
	auto minmaxY = std::minmax_element(m_vecY.begin(), m_vecY.end());
	t_real dMaxErrY = *std::max_element(m_vecYErr.begin(), m_vecYErr.end());

	std::ostringstream ostrX, ostrY, ostrYErr;
	ostrX.precision(g_iPrec);
	ostrY.precision(g_iPrec);
	ostrYErr.precision(g_iPrec);

	for(std::size_t i=0; i<std::min(m_vecX.size(), m_vecY.size()); ++i)
	{
		ostrX << m_vecX[i] << ", ";
		ostrY << m_vecY[i] << ", ";
		ostrYErr << m_vecYErr[i] << ", ";
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
	tl::find_and_replace<std::string>(strRootSrc, "%%TAKIN_VER%%", TAKIN_VER);

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

	std::string strPath = comboPath->currentText().toStdString();
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
				// ignore non-existing files and directories
				if(!tl::file_exists(p.string().c_str()))
					return false;

				std::string strExt = tl::wstr_to_str(p.extension().native());
				if(strExt == ".bz2" || strExt == ".gz" || strExt == ".z")
					strExt = "." + tl::wstr_to_str(tl::get_fileext2(p.filename().native()));

				// allow everything if no extensions are defined
				if(this->m_vecExts.size() == 0)
					return true;

				// see if extension is in list
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
	bool bUseSwarm = m_settings.value("use_swarm", false).toBool();

	m_vecFitX.clear();
	m_vecFitY.clear();

	if(std::min(m_vecX.size(), m_vecY.size()) == 0)
		return false;

	bool bOk = 0;
	try
	{
		std::vector<t_real_min>
			_vecVals = tl::container_cast<t_real_min, t_real, std::vector>()(vecVals),
			_vecErrs = tl::container_cast<t_real_min, t_real, std::vector>()(vecErrs);

		if(bUseSwarm)
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


/**
 * parabola: y = amp*(x-x0)^2 + offs
 */
void ScanViewerDlg::FitParabola()
{
	if(std::min(m_vecX.size(), m_vecY.size()) == 0)
		return;

	const bool bUseSlope = checkSloped->isChecked();

	auto func = tl::parabola_model<t_real>;
	auto funcSloped = tl::parabola_model_slope<t_real>;
	constexpr std::size_t iFuncArgs = 4;
	constexpr std::size_t iFuncArgsSloped = iFuncArgs+1;

	t_real_glob dAmp = m_pFitParamDlg->GetAmp(),	dAmpErr = m_pFitParamDlg->GetAmpErr();
	t_real_glob dX0 = m_pFitParamDlg->GetX0(),	dX0Err = m_pFitParamDlg->GetX0Err();
	t_real_glob dOffs = m_pFitParamDlg->GetOffs(),	dOffsErr = m_pFitParamDlg->GetOffsErr();
	t_real_glob dSlope = m_pFitParamDlg->GetSlope(),dSlopeErr = m_pFitParamDlg->GetSlopeErr();

	bool bAmpFixed = m_pFitParamDlg->GetAmpFixed();
	bool bX0Fixed = m_pFitParamDlg->GetX0Fixed();
	bool bOffsFixed = m_pFitParamDlg->GetOffsFixed();
	bool bSlopeFixed = m_pFitParamDlg->GetSlopeFixed();

	// automatic parameter determination
	if(!m_pFitParamDlg->WantParams())
	{
		auto minmaxX = std::minmax_element(m_vecX.begin(), m_vecX.end());
		auto minmaxY = std::minmax_element(m_vecY.begin(), m_vecY.end());

		dX0 = m_vecX[minmaxY.second - m_vecY.begin()];
		dAmp = std::abs(*minmaxY.second-*minmaxY.first);
		dOffs = *minmaxY.first;

		dX0Err = dX0 * 0.1;
		dAmpErr = dAmp * 0.1;
		dOffsErr = dOffs * 0.1;

		bAmpFixed = bX0Fixed = bOffsFixed = 0;
	}

	std::vector<std::string> vecParamNames = { "x0", "amp", "offs" };
	std::vector<t_real> vecVals = { dX0, dAmp, dOffs };
	std::vector<t_real> vecErrs = { dX0Err, dAmpErr, dOffsErr };
	std::vector<bool> vecFixed = { bX0Fixed, bAmpFixed, bOffsFixed };

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

	m_pFitParamDlg->SetX0(vecVals[0]);	m_pFitParamDlg->SetX0Err(vecErrs[0]);
	m_pFitParamDlg->SetAmp(vecVals[1]);	m_pFitParamDlg->SetAmpErr(vecErrs[1]);
	m_pFitParamDlg->SetOffs(vecVals[2]);	m_pFitParamDlg->SetOffsErr(vecErrs[2]);

	if(bUseSlope)
	{
		m_pFitParamDlg->SetSlope(vecVals[3]);
		m_pFitParamDlg->SetSlopeErr(vecErrs[3]);
	}
}


/**
 * sin(x + pi) = -sin(x)
 * sin(-x + phi) = -sin(x - phi)
 */
template<class t_real = double>
void sanitise_sine_params(t_real& amp, t_real& freq, t_real& phase, t_real& offs)
{
	if(freq < t_real(0))
	{
		freq = -freq;
		phase = -phase;
		amp = -amp;
	}

	if(amp < t_real(0))
	{
		amp = -amp;
		phase += tl::get_pi<t_real>();
	}

	if(phase < t_real(0))
	{
		int iNum = std::abs(int(phase / (t_real(2)*tl::get_pi<t_real>()))) + 1;
		phase += t_real(2*iNum) * tl::get_pi<t_real>();
	}

	phase = std::fmod(phase, t_real(2)*tl::get_pi<t_real>());
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

	sanitise_sine_params<t_real>(vecVals[0], vecVals[1], vecVals[2], vecVals[3]);


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
