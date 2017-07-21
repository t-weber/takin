/**
 * monte carlo convolution tool
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 2015, 2016
 * @license GPLv2
 */

#include "ConvoDlg.h"
#include "tlibs/string/string.h"
#include "tlibs/math/math.h"
#include "tlibs/math/rand.h"

#ifndef NO_PY
	#include "sqw_py.h"
#endif

#include "libs/globals.h"
#include "libs/globals_qt.h"
#include "libs/recent.h"

#include <iostream>
#include <fstream>
#include <tuple>

#include <QFileDialog>
#include <QMessageBox>
#include <QMenu>


using t_real = t_real_reso;
const std::string ConvoDlg::s_strTitle = "Resolution Convolution";

ConvoDlg::ConvoDlg(QWidget* pParent, QSettings* pSett)
	: QDialog(pParent, Qt::WindowTitleHint|Qt::WindowCloseButtonHint|Qt::WindowMinMaxButtonsHint),
		m_pSett(pSett)
{
	setupUi(this);
	setWindowTitle(s_strTitle.c_str());

	// -------------------------------------------------------------------------
	// widgets
	m_vecSpinBoxes = { spinStartH, spinStartK, spinStartL, spinStartE,
		spinStopH, spinStopK, spinStopL, spinStopE,
		spinStopH2, spinStopK2, spinStopL2, spinStopE2,
		spinKfix,
		spinTolerance
	};

	m_vecSpinNames = {
		"monteconvo/h_from", "monteconvo/k_from", "monteconvo/l_from", "monteconvo/E_from",
		"monteconvo/h_to", "monteconvo/k_to", "monteconvo/l_to", "monteconvo/E_to",
		"monteconvo/h_to_2", "monteconvo/k_to_2", "monteconvo/l_to_2", "monteconvo/E_to_2",
		"monteconvo/kfix",
		"convofit/tolerance"
	};

	m_vecIntSpinBoxes = { spinNeutrons, spinSampleSteps, spinStepCnt,
		spinStrategy, spinMaxCalls };
	m_vecIntSpinNames = { "monteconvo/neutron_count", "monteconvo/sample_step_count", "monteconvo/step_count",
	"convofit/strategy", "convofit/max_calls"
	};

	m_vecEditBoxes = { editCrys, editRes, editSqw, editScan, editScale, editOffs,
		editCounter, editMonitor, editTemp, editField
	};
	m_vecEditNames = { "monteconvo/crys", "monteconvo/instr", "monteconvo/sqw_conf",
		"monteconvo/scanfile", "monteconvo/S_scale", "monteconvo/S_offs",
		"convofit/counter", "convofit/monitor",
		"convofit/temp_override", "convofit/field_override"
	};

	m_vecTextBoxes = { editSqwParams };
	m_vecTextNames = { "convofit/sqw_params" };

	m_vecComboBoxes = { comboAlgo, comboFixedK, comboFocMono, comboFocAna,
		comboFitter
	};
	m_vecComboNames = { "monteconvo/algo", "monteconvo/fixedk", "monteconvo/mono_foc",
		"monteconvo/ana_foc",
		"convofit/minimiser"
	};

	m_vecCheckBoxes = { checkScan, check2dMap,
		checkRnd, checkNorm, checkFlip
	};
	m_vecCheckNames = { "monteconvo/has_scanfile", "monteconvo/scan_2d",
		"convofit/recycle_neutrons", "convofit/normalise", "convofit/flip_coords"
	};
	// -------------------------------------------------------------------------

	if(m_pSett)
	{
		QFont font;
		if(m_pSett->contains("main/font_gen") && font.fromString(m_pSett->value("main/font_gen", "").toString()))
			setFont(font);
	}

	btnStart->setIcon(load_icon("res/icons/media-playback-start.svg"));
	btnStartFit->setIcon(load_icon("res/icons/media-playback-start.svg"));
	btnStop->setIcon(load_icon("res/icons/media-playback-stop.svg"));

	/*
	 * curve 0,1	->	convolution
	 * curve 2		->	scan points
	 * curve 3-15	->	dispersion branches
	 */
	m_plotwrap.reset(new QwtPlotWrapper(plot, CONVO_MAX_CURVES, true));
	m_plotwrap->GetPlot()->setAxisTitle(QwtPlot::xBottom, "");
	m_plotwrap->GetPlot()->setAxisTitle(QwtPlot::yLeft, "S(Q,E) (a.u.)");

	m_plotwrap2d.reset(new QwtPlotWrapper(plot2d, 1, 0, 0, 1));
	m_plotwrap2d->GetPlot()->setAxisTitle(QwtPlot::yRight, "S(Q,E) (a.u.)");


	// --------------------------------------------------------------------
	// convolution lines
	QPen penCurve;
	penCurve.setColor(QColor(0,0,0x99));
	penCurve.setWidth(2);
	m_plotwrap->GetCurve(0)->setPen(penCurve);
	m_plotwrap->GetCurve(0)->setStyle(QwtPlotCurve::CurveStyle::Lines);
	m_plotwrap->GetCurve(0)->setTitle("S(Q,E)");

	// convolution points
	QPen penPoints;
	penPoints.setColor(QColor(0xff,0,0));
	penPoints.setWidth(4);
	m_plotwrap->GetCurve(1)->setPen(penPoints);
	m_plotwrap->GetCurve(1)->setStyle(QwtPlotCurve::CurveStyle::Dots);
	m_plotwrap->GetCurve(1)->setTitle("S(Q,E)");

	// scan data points
	QPen penScanPoints;
	penScanPoints.setColor(QColor(0x00,0x90,0x00));
	penScanPoints.setWidth(6);
	m_plotwrap->GetCurve(2)->setPen(penScanPoints);
	m_plotwrap->GetCurve(2)->setStyle(QwtPlotCurve::CurveStyle::Dots);
	m_plotwrap->GetCurve(2)->setTitle("S(Q,E)");

	// dispersion branches
	for(int iCurve=CONVO_DISP_CURVE_START; iCurve<CONVO_MAX_CURVES; ++iCurve)
	{
		m_plotwrap->GetCurve(iCurve)->setPen(penCurve);
		m_plotwrap->GetCurve(iCurve)->setStyle(QwtPlotCurve::CurveStyle::Lines);
		m_plotwrap->GetCurve(iCurve)->setTitle("E(Q)");
	}
	// --------------------------------------------------------------------


	// --------------------------------------------------------------------
	QMenu *pMenuActions = new QMenu("Actions", this);

	QAction *pHK = new QAction("h <-> k", pMenuActions);
	QAction *pHL = new QAction("h <-> l", pMenuActions);
	QAction *pKL = new QAction("k <-> l", pMenuActions);
	pMenuActions->addAction(pHK);
	pMenuActions->addAction(pHL);
	pMenuActions->addAction(pKL);

	btnActions->setMenu(pMenuActions);
	// --------------------------------------------------------------------


	// --------------------------------------------------------------------
	// fill sqw combo box
	load_sqw_plugins();
	auto vecSqwNames = get_sqw_names();
	for(const auto& tupSqw : vecSqwNames)
	{
		QString strIdent = std::get<0>(tupSqw).c_str();
		QString strName = std::get<1>(tupSqw).c_str();

		comboSqw->addItem(strName, strIdent);
	}
	// --------------------------------------------------------------------


	// --------------------------------------------------------------------
	// menu bar
	m_pMenuBar = new QMenuBar(this);
	if(m_pSett)
		m_pMenuBar->setNativeMenuBar(m_pSett->value("main/native_dialogs", 1).toBool());


	// file menu
	QMenu *pMenuFile = new QMenu("File", this);

    QAction *pNew = new QAction("New", this);
    pNew->setIcon(load_icon("res/icons/document-new.svg"));
    pMenuFile->addAction(pNew);

	pMenuFile->addSeparator();

	QAction *pLoad = new QAction("Load...", this);
	pLoad->setIcon(load_icon("res/icons/document-open.svg"));
	pMenuFile->addAction(pLoad);

	m_pMenuRecent = new QMenu("Recently Loaded", this);
	RecentFiles recent(m_pSett, "monteconvo/recent");
	m_pMapperRecent = new QSignalMapper(m_pMenuRecent);
	QObject::connect(m_pMapperRecent, SIGNAL(mapped(const QString&)),
		this, SLOT(Load(const QString&)));
	recent.FillMenu(m_pMenuRecent, m_pMapperRecent);
	pMenuFile->addMenu(m_pMenuRecent);

	QAction *pSave = new QAction("Save", this);
	pSave->setIcon(load_icon("res/icons/document-save.svg"));
	pMenuFile->addAction(pSave);

	QAction *pSaveAs = new QAction("Save As...", this);
	pSaveAs->setIcon(load_icon("res/icons/document-save-as.svg"));
	pMenuFile->addAction(pSaveAs);

	pMenuFile->addSeparator();

	QAction *pConvofit = new QAction("Export to Convofit...", this);
	pConvofit->setIcon(load_icon("res/icons/drive-harddisk.svg"));
	pMenuFile->addAction(pConvofit);

	pMenuFile->addSeparator();

	QAction *pExit = new QAction("Exit", this);
	pExit->setIcon(load_icon("res/icons/system-log-out.svg"));
	pMenuFile->addAction(pExit);


	// actions menu
	QMenu *pMenuConvoActions = new QMenu("Actions", this);

	QAction *pActionStart = new QAction("Start Convolution", this);
	pActionStart->setIcon(load_icon("res/icons/media-playback-start.svg"));
	pMenuConvoActions->addAction(pActionStart);

	QAction *pActionDisp = new QAction("Plot Dispersion", this);
	pMenuConvoActions->addAction(pActionDisp);


	// results menu
	QMenu *pMenuPlots = new QMenu("Results", this);

	m_pLiveResults = new QAction("Live Results", this);
	m_pLiveResults->setCheckable(1);
	m_pLiveResults->setChecked(0);
	pMenuPlots->addAction(m_pLiveResults);

	m_pLivePlots = new QAction("Live Plots", this);
	m_pLivePlots->setCheckable(1);
	m_pLivePlots->setChecked(1);
	pMenuPlots->addAction(m_pLivePlots);

	pMenuPlots->addSeparator();

	QAction *pExportPlot = new QAction("Export Plot Data...", this);
	pMenuPlots->addAction(pExportPlot);

	QAction *pExportPlotGpl = new QAction("Export Plot to Gnuplot...", this);
	pMenuPlots->addAction(pExportPlotGpl);

	pMenuPlots->addSeparator();

	QAction *pExportPlot2d = new QAction("Export 2D Plot Data...", this);
	pMenuPlots->addAction(pExportPlot2d);

	QAction *pExportPlot2dGpl = new QAction("Export 2D Plot to Gnuplot...", this);
	pMenuPlots->addAction(pExportPlot2dGpl);

	pMenuPlots->addSeparator();

	QAction *pSaveResults = new QAction("Save Results...", this);
	pSaveResults->setIcon(load_icon("res/icons/document-save-as.svg"));
	pMenuPlots->addAction(pSaveResults);

	// help menu
	QMenu *pMenuHelp = new QMenu("Help", this);

	QAction *pAbout = new QAction("About...", this);
	pAbout->setIcon(load_icon("res/icons/dialog-information.svg"));
	pMenuHelp->addAction(pAbout);


	m_pMenuBar->addMenu(pMenuFile);
	m_pMenuBar->addMenu(pMenuConvoActions);
	m_pMenuBar->addMenu(pMenuPlots);
	m_pMenuBar->addMenu(pMenuHelp);


	QObject::connect(pExit, SIGNAL(triggered()), this, SLOT(accept()));
	QObject::connect(pNew, SIGNAL(triggered()), this, SLOT(New()));
	QObject::connect(pLoad, SIGNAL(triggered()), this, SLOT(Load()));
	QObject::connect(pSave, SIGNAL(triggered()), this, SLOT(Save()));
	QObject::connect(pSaveAs, SIGNAL(triggered()), this, SLOT(SaveAs()));
	QObject::connect(pConvofit, SIGNAL(triggered()), this, SLOT(SaveConvofit()));
	QObject::connect(pActionStart, SIGNAL(triggered()), this, SLOT(Start()));
	QObject::connect(pActionDisp, SIGNAL(triggered()), this, SLOT(StartDisp()));
	QObject::connect(pExportPlot, SIGNAL(triggered()), m_plotwrap.get(), SLOT(SavePlot()));
	QObject::connect(pExportPlot2d, SIGNAL(triggered()), m_plotwrap2d.get(), SLOT(SavePlot()));
	QObject::connect(pExportPlotGpl, SIGNAL(triggered()), m_plotwrap.get(), SLOT(ExportGpl()));
	QObject::connect(pExportPlot2dGpl, SIGNAL(triggered()), m_plotwrap2d.get(), SLOT(ExportGpl()));
	QObject::connect(pSaveResults, SIGNAL(triggered()), this, SLOT(SaveResult()));
	QObject::connect(pAbout, SIGNAL(triggered()), this, SLOT(ShowAboutDlg()));

	this->layout()->setMenuBar(m_pMenuBar);
	// --------------------------------------------------------------------


	m_pSqwParamDlg = new SqwParamDlg(this, m_pSett);
	QObject::connect(this,
		SIGNAL(SqwLoaded(const std::vector<SqwBase::t_var>&,
		const std::vector<SqwBase::t_var_fit>*)),
		m_pSqwParamDlg, SLOT(SqwLoaded(const std::vector<SqwBase::t_var>&,
		const std::vector<SqwBase::t_var_fit>*)));
	QObject::connect(m_pSqwParamDlg,
		SIGNAL(SqwParamsChanged(const std::vector<SqwBase::t_var>&, const std::vector<SqwBase::t_var_fit>*)),
		this, SLOT(SqwParamsChanged(const std::vector<SqwBase::t_var>&,
			const std::vector<SqwBase::t_var_fit>*)));

	m_pFavDlg = new FavDlg(this, m_pSett);
	QObject::connect(m_pFavDlg, SIGNAL(ChangePos(const struct FavHklPos&)),
		this, SLOT(ChangePos(const struct FavHklPos&)));

	//QObject::connect(buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(ButtonBoxClicked(QAbstractButton*)));

	QObject::connect(btnBrowseCrys, SIGNAL(clicked()), this, SLOT(browseCrysFiles()));
	QObject::connect(btnBrowseRes, SIGNAL(clicked()), this, SLOT(browseResoFiles()));
	QObject::connect(btnBrowseSqw, SIGNAL(clicked()), this, SLOT(browseSqwFiles()));
	QObject::connect(btnBrowseScan, SIGNAL(clicked()), this, SLOT(browseScanFiles()));

	QObject::connect(btnFav, SIGNAL(clicked()), this, SLOT(ShowFavourites()));
	QObject::connect(btnSqwParams, SIGNAL(clicked()), this, SLOT(showSqwParamDlg()));

	QObject::connect(comboSqw, SIGNAL(currentIndexChanged(int)),
		this, SLOT(SqwModelChanged(int)));
	QObject::connect(editSqw, SIGNAL(textChanged(const QString&)),
		this, SLOT(createSqwModel(const QString&)));
	QObject::connect(editScan, SIGNAL(textChanged(const QString&)),
		this, SLOT(scanFileChanged(const QString&)));

	QObject::connect(editScale, SIGNAL(textChanged(const QString&)), this, SLOT(scaleChanged()));
	QObject::connect(editOffs, SIGNAL(textChanged(const QString&)), this, SLOT(scaleChanged()));

	QObject::connect(btnStart, SIGNAL(clicked()), this, SLOT(Start()));
	QObject::connect(btnStartFit, SIGNAL(clicked()), this, SLOT(StartFit()));
	QObject::connect(btnStop, SIGNAL(clicked()), this, SLOT(Stop()));

	QObject::connect(checkScan, SIGNAL(toggled(bool)), this, SLOT(scanCheckToggled(bool)));

	QObject::connect(pHK, SIGNAL(triggered()), this, SLOT(ChangeHK()));
	QObject::connect(pHL, SIGNAL(triggered()), this, SLOT(ChangeHL()));
	QObject::connect(pKL, SIGNAL(triggered()), this, SLOT(ChangeKL()));

	for(QDoubleSpinBox* pSpin : {spinStartH, spinStartK, spinStartL, spinStartE,
		spinStopH, spinStopK, spinStopL, spinStopE})
		QObject::connect(pSpin, SIGNAL(valueChanged(double)), this, SLOT(UpdateCurFavPos()));

	LoadSettings();
}


ConvoDlg::~ConvoDlg()
{
	if(m_pth)
	{
		if(m_pth->joinable())
			m_pth->join();
		delete m_pth;
		m_pth = nullptr;
	}

	if(m_pSqwParamDlg) { delete m_pSqwParamDlg; m_pSqwParamDlg = nullptr; }
	if(m_pFavDlg) { delete m_pFavDlg; m_pFavDlg = nullptr; }
	if(m_pSqw) m_pSqw.reset();
	if(m_pMenuBar) { delete m_pMenuBar; m_pMenuBar = nullptr; }

	unload_sqw_plugins();
}


void ConvoDlg::SqwModelChanged(int)
{
	if(!m_bAllowSqwReinit) return;

	editSqw->clear();
	createSqwModel("");
	//emit SqwLoaded(std::vector<SqwBase::t_var>{}, nullptr);
}

void ConvoDlg::createSqwModel(const QString& qstrFile)
{
	if(!m_bAllowSqwReinit) return;

	if(m_pSqw)
	{
		m_pSqw.reset();
		emit SqwLoaded(std::vector<SqwBase::t_var>{}, nullptr);
	}

	std::string strSqwIdent = comboSqw->itemData(comboSqw->currentIndex()).toString().toStdString();
	std::string strSqwFile = qstrFile.toStdString();
	tl::trim(strSqwFile);

	if(strSqwFile == "")
	{
		tl::log_warn("No S(q,w) config file given.");
		//return;
	}

#ifdef NO_PY
	if(strSqwIdent == "py")
	{
		QMessageBox::critical(this, "Error", "Compiled without python support.");
		return;
	}
#endif

	m_pSqw = construct_sqw(strSqwIdent, strSqwFile);
	if(!m_pSqw)
	{
		QMessageBox::critical(this, "Error", "Unknown S(q,w) model selected.");
		return;
	}

	if(m_pSqw && m_pSqw->IsOk())
	{
		emit SqwLoaded(m_pSqw->GetVars(), &m_pSqw->GetFitVars());
	}
	else
	{
		//QMessageBox::critical(this, "Error", "Could not create S(q,w).");
		tl::log_err("Could not create S(q,w).");
		return;
	}
}


void ConvoDlg::SqwParamsChanged(const std::vector<SqwBase::t_var>& vecVars,
	const std::vector<SqwBase::t_var_fit>* pvecVarsFit)
{
	if(!m_pSqw) return;
	m_pSqw->SetVars(vecVars);
	if(pvecVarsFit) m_pSqw->SetFitVars(*pvecVarsFit);

#ifndef NDEBUG
	// check: read parameters back in
	emit SqwLoaded(m_pSqw->GetVars(), &m_pSqw->GetFitVars());
#endif
}


// -----------------------------------------------------------------------------

ResoFocus ConvoDlg::GetFocus() const
{
	const int iFocMono = comboFocMono->currentIndex();
	const int iFocAna = comboFocAna->currentIndex();

	unsigned ifocMode = unsigned(ResoFocus::FOC_NONE);
	if(iFocMono == 1) ifocMode |= unsigned(ResoFocus::FOC_MONO_H);	// horizontal
	else if(iFocMono == 2) ifocMode |= unsigned(ResoFocus::FOC_MONO_V);	// vertical
	else if(iFocMono == 3) ifocMode |= unsigned(ResoFocus::FOC_MONO_V)|unsigned(ResoFocus::FOC_MONO_H);		// both
	if(iFocAna == 1) ifocMode |= unsigned(ResoFocus::FOC_ANA_H);	// horizontal
	else if(iFocAna == 2) ifocMode |= unsigned(ResoFocus::FOC_ANA_V);	// vertical
	else if(iFocAna == 3) ifocMode |= unsigned(ResoFocus::FOC_ANA_V)|unsigned(ResoFocus::FOC_ANA_H);		// both

	return ResoFocus(ifocMode);
}


/**
 * clear plot curves
 */
void ConvoDlg::ClearPlot1D()
{
	static const std::vector<t_real> vecZero;
	if(!m_plotwrap) return;

	for(std::size_t iCurve=0; iCurve<CONVO_MAX_CURVES; ++iCurve)
		set_qwt_data<t_real_reso>()(*m_plotwrap, vecZero, vecZero, iCurve, false);
}


/**
 * start 1d or 2d convolutions
 */
void ConvoDlg::Start()
{
	if(check2dMap->isChecked())
		Start2D();
	else
		Start1D();
}


/**
 * stop running operations
 */
void ConvoDlg::Stop()
{
	m_atStop.store(true);
}


// -----------------------------------------------------------------------------


void ConvoDlg::ShowFavourites()
{
	focus_dlg(m_pFavDlg);
}

void ConvoDlg::UpdateCurFavPos()
{
	FavHklPos pos;
	pos.dhstart = spinStartH->value();
	pos.dkstart = spinStartK->value();
	pos.dlstart = spinStartL->value();
	pos.dEstart = spinStartE->value();
	pos.dhstop = spinStopH->value();
	pos.dkstop = spinStopK->value();
	pos.dlstop = spinStopL->value();
	pos.dEstop = spinStopE->value();

	m_pFavDlg->UpdateCurPos(pos);
}

void ConvoDlg::ChangePos(const struct FavHklPos& pos)
{
	spinStartH->setValue(pos.dhstart);
	spinStartK->setValue(pos.dkstart);
	spinStartL->setValue(pos.dlstart);
	spinStartE->setValue(pos.dEstart);
	spinStopH->setValue(pos.dhstop);
	spinStopK->setValue(pos.dkstop);
	spinStopL->setValue(pos.dlstop);
	spinStopE->setValue(pos.dEstop);
}


// -----------------------------------------------------------------------------


static void SwapSpin(QDoubleSpinBox* pS1, QDoubleSpinBox* pS2)
{
	double dVal = pS1->value();
	pS1->setValue(pS2->value());
	pS2->setValue(dVal);
}

void ConvoDlg::ChangeHK()
{
	SwapSpin(spinStartH, spinStartK);
	SwapSpin(spinStopH, spinStopK);
}
void ConvoDlg::ChangeHL()
{
	SwapSpin(spinStartH, spinStartL);
	SwapSpin(spinStopH, spinStopL);
}
void ConvoDlg::ChangeKL()
{
	SwapSpin(spinStartK, spinStartL);
	SwapSpin(spinStopK, spinStopL);
}


// -----------------------------------------------------------------------------

void ConvoDlg::scanCheckToggled(bool bChecked)
{
	if(bChecked)
		scanFileChanged(editScan->text());
}

void ConvoDlg::scanFileChanged(const QString& qstrFile)
{
	m_bUseScan = 0;

	std::string strFile = qstrFile.toStdString();
	tl::trim(strFile);
	if(strFile == "" || !checkScan->isChecked())
		return;

	std::vector<std::string> vecFiles;
	tl::get_tokens<std::string, std::string>(strFile, ";", vecFiles);
	std::for_each(vecFiles.begin(), vecFiles.end(), [](std::string& str){ tl::trim(str); });

	Filter filter;
	m_scan = Scan();
	if(!::load_file(vecFiles, m_scan, 1, filter))
	{
		tl::log_err("Cannot load scan(s).");
		return;
	}

	if(!m_scan.vecPoints.size())
	{
		tl::log_err("No points in scan(s).");
		return;
	}

	const ScanPoint& ptBegin = *m_scan.vecPoints.cbegin();
	const ScanPoint& ptEnd = *m_scan.vecPoints.crbegin();

	comboFixedK->setCurrentIndex(m_scan.bKiFixed ? 0 : 1);
	spinKfix->setValue(m_scan.dKFix);

	spinStartH->setValue(ptBegin.h);
	spinStartK->setValue(ptBegin.k);
	spinStartL->setValue(ptBegin.l);
	spinStartE->setValue(ptBegin.E / tl::get_one_meV<t_real>());

	spinStopH->setValue(ptEnd.h);
	spinStopK->setValue(ptEnd.k);
	spinStopL->setValue(ptEnd.l);
	spinStopE->setValue(ptEnd.E / tl::get_one_meV<t_real>());

	m_bUseScan = 1;
}

void ConvoDlg::scaleChanged()
{
	t_real dScale = tl::str_to_var<t_real>(editScale->text().toStdString());
	t_real dOffs = tl::str_to_var<t_real>(editOffs->text().toStdString());

	m_vecScaledS.resize(m_vecS.size());
	for(std::size_t i=0; i<m_vecS.size(); ++i)
		m_vecScaledS[i] = dScale*m_vecS[i] + dOffs;

	set_qwt_data<t_real_reso>()(*m_plotwrap, m_vecQ, m_vecScaledS, 0, false);
	set_qwt_data<t_real_reso>()(*m_plotwrap, m_vecQ, m_vecScaledS, 1, false);

	m_plotwrap->GetPlot()->replot();
}


// -----------------------------------------------------------------------------


void ConvoDlg::showSqwParamDlg()
{
	focus_dlg(m_pSqwParamDlg);
}


#include "libs/version.h"

void ConvoDlg::ShowAboutDlg()
{
	std::ostringstream ostrAbout;
	ostrAbout << "Takin/Monteconvo version " << TAKIN_VER << ".\n";
	ostrAbout << "Written by Tobias Weber <tobias.weber@tum.de>,\n";
	ostrAbout << "2015 - 2017.\n";
	ostrAbout << "\n" << TAKIN_LICENSE("Takin/Monteconvo");

	QMessageBox::about(this, "About Monteconvo", ostrAbout.str().c_str());
}


#include "ConvoDlg.moc"
