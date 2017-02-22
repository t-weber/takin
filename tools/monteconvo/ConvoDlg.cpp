/**
 * monte carlo convolution tool
 * @author tweber
 * @date 2015, 2016
 * @license GPLv2
 */

#include "ConvoDlg.h"
#include "tlibs/string/string.h"
#include "tlibs/math/math.h"
#include "tlibs/math/rand.h"
#include "tlibs/helper/thread.h"
#include "tlibs/time/stopwatch.h"

#ifndef NO_PY
	#include "sqw_py.h"
#endif

#include "libs/globals.h"
#include "libs/globals_qt.h"

#include <iostream>
#include <fstream>
#include <tuple>

#include <QFileDialog>
#include <QMessageBox>
#include <QMenu>

#define MAX_CURVES			16
#define DISP_CURVE_START 	3

using t_real = t_real_reso;
using t_stopwatch = tl::Stopwatch<t_real>;

static constexpr const t_real g_dEpsRlu = 1e-3;


ConvoDlg::ConvoDlg(QWidget* pParent, QSettings* pSett)
	: QDialog(pParent, Qt::WindowTitleHint|Qt::WindowCloseButtonHint|Qt::WindowMinMaxButtonsHint),
		m_pSett(pSett)
{
	setupUi(this);

	// -------------------------------------------------------------------------
	// widgets
	m_vecSpinBoxes = { spinStartH, spinStartK, spinStartL, spinStartE,
		spinStopH, spinStopK, spinStopL, spinStopE,
		spinStopH2, spinStopK2, spinStopL2, spinStopE2,
		spinKfix,
	};

	m_vecSpinNames = {
		"monteconvo/h_from", "monteconvo/k_from", "monteconvo/l_from", "monteconvo/E_from",
		"monteconvo/h_to", "monteconvo/k_to", "monteconvo/l_to", "monteconvo/E_to",
		"monteconvo/h_to_2", "monteconvo/k_to_2", "monteconvo/l_to_2", "monteconvo/E_to_2",
		"monteconvo/kfix",
	};

	m_vecIntSpinBoxes = { spinNeutrons, spinSampleSteps, spinStepCnt };
	m_vecIntSpinNames = { "monteconvo/neutron_count", "monteconvo/sample_step_count", "monteconvo/step_count" };

	m_vecEditBoxes = { editCrys, editRes, editSqw, editScan, editScale, editOffs };
	m_vecEditNames = { "monteconvo/crys", "monteconvo/instr", "monteconvo/sqw_conf",
		"monteconvo/scanfile", "monteconvo/S_scale", "monteconvo/S_offs" };

	m_vecComboBoxes = { comboAlgo, comboFixedK, comboFocMono, comboFocAna };
	m_vecComboNames = { "monteconvo/algo", "monteconvo/fixedk", "monteconvo/mono_foc",
		"monteconvo/ana_foc" };

	m_vecCheckBoxes = { checkScan, check2dMap };
	m_vecCheckNames = { "monteconvo/has_scanfile", "monteconvo/scan_2d" };
	// -------------------------------------------------------------------------

	if(m_pSett)
	{
		QFont font;
		if(m_pSett->contains("main/font_gen") && font.fromString(m_pSett->value("main/font_gen", "").toString()))
			setFont(font);
	}

	btnStart->setIcon(load_icon("res/icons/media-playback-start.svg"));
	btnStop->setIcon(load_icon("res/icons/media-playback-stop.svg"));

	/*
	 * curve 0,1	->	convolution
	 * curve 2		->	scan points
	 * curve 3-15	->	dispersion branches
	 */
	m_plotwrap.reset(new QwtPlotWrapper(plot, MAX_CURVES, true));
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
	for(int iCurve=DISP_CURVE_START; iCurve<MAX_CURVES; ++iCurve)
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

	QAction *pLoad = new QAction("Load Configuration...", this);
	pLoad->setIcon(load_icon("res/icons/document-open.svg"));
	pMenuFile->addAction(pLoad);

	QAction *pSaveAs = new QAction("Save Configuration...", this);
	pSaveAs->setIcon(load_icon("res/icons/document-save-as.svg"));
	pMenuFile->addAction(pSaveAs);

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
	QObject::connect(pLoad, SIGNAL(triggered()), this, SLOT(Load()));
	QObject::connect(pSaveAs, SIGNAL(triggered()), this, SLOT(Save()));
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
	QObject::connect(this, SIGNAL(SqwLoaded(const std::vector<SqwBase::t_var>&)),
		m_pSqwParamDlg, SLOT(SqwLoaded(const std::vector<SqwBase::t_var>&)));
	QObject::connect(m_pSqwParamDlg, SIGNAL(SqwParamsChanged(const std::vector<SqwBase::t_var>&)),
		this, SLOT(SqwParamsChanged(const std::vector<SqwBase::t_var>&)));

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
	//emit SqwLoaded(std::vector<SqwBase::t_var>{});
}

void ConvoDlg::createSqwModel(const QString& qstrFile)
{
	if(!m_bAllowSqwReinit) return;

	if(m_pSqw)
	{
		m_pSqw.reset();
		emit SqwLoaded(std::vector<SqwBase::t_var>{});
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
		emit SqwLoaded(m_pSqw->GetVars());
	}
	else
	{
		//QMessageBox::critical(this, "Error", "Could not create S(q,w).");
		tl::log_err("Could not create S(q,w).");
		return;
	}
}


void ConvoDlg::SqwParamsChanged(const std::vector<SqwBase::t_var>& vecVars)
{
	if(!m_pSqw) return;
	m_pSqw->SetVars(vecVars);

#ifndef NDEBUG
	// check: read parameters back in
	emit SqwLoaded(m_pSqw->GetVars());
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

	for(std::size_t iCurve=0; iCurve<MAX_CURVES; ++iCurve)
		set_qwt_data<t_real_reso>()(*m_plotwrap, vecZero, vecZero, iCurve, false);
}


/**
 * create 1d convolution
 */
void ConvoDlg::Start1D()
{
	m_atStop.store(false);
	ClearPlot1D();

	bool bUseScan = m_bUseScan && checkScan->isChecked();
	t_real dScale = tl::str_to_var<t_real>(editScale->text().toStdString());
	t_real dOffs = tl::str_to_var<t_real>(editOffs->text().toStdString());

	bool bLiveResults = m_pLiveResults->isChecked();
	bool bLivePlots = m_pLivePlots->isChecked();

	btnStart->setEnabled(false);
	tabSettings->setEnabled(false);
	m_pMenuBar->setEnabled(false);
	btnStop->setEnabled(true);
	tabWidget->setCurrentWidget(tabPlot);

	bool bForceDeferred = false;
	Qt::ConnectionType connty = bForceDeferred
		? Qt::ConnectionType::DirectConnection
		: Qt::ConnectionType::BlockingQueuedConnection;

	std::function<void()> fkt = [this, connty, bForceDeferred, bUseScan,
	dScale, dOffs, bLiveResults, bLivePlots]
	{
		std::function<void()> fktEnableButtons = [this]
		{
			QMetaObject::invokeMethod(btnStop, "setEnabled", Q_ARG(bool, false));
			QMetaObject::invokeMethod(tabSettings, "setEnabled", Q_ARG(bool, true));
			QMetaObject::invokeMethod(m_pMenuBar, "setEnabled", Q_ARG(bool, true));
			QMetaObject::invokeMethod(btnStart, "setEnabled", Q_ARG(bool, true));
		};

		t_stopwatch watch;
		watch.start();

		const unsigned int iNumNeutrons = spinNeutrons->value();
		const unsigned int iNumSampleSteps = spinSampleSteps->value();

		const unsigned int iNumSteps = spinStepCnt->value();
		std::vector<t_real> vecH = tl::linspace<t_real,t_real>(
			spinStartH->value(), spinStopH->value(), iNumSteps);
		std::vector<t_real> vecK = tl::linspace<t_real,t_real>(
			spinStartK->value(), spinStopK->value(), iNumSteps);
		std::vector<t_real> vecL = tl::linspace<t_real,t_real>(
			spinStartL->value(), spinStopL->value(), iNumSteps);
		std::vector<t_real> vecE = tl::linspace<t_real,t_real>(
			spinStartE->value(), spinStopE->value(), iNumSteps);

		std::string strScanVar = "";
		std::vector<t_real> *pVecScanX = nullptr;
		if(!tl::float_equal(spinStartH->value(), spinStopH->value(), g_dEpsRlu))
		{
			pVecScanX = &vecH;
			strScanVar = "h (rlu)";
		}
		else if(!tl::float_equal(spinStartK->value(), spinStopK->value(), g_dEpsRlu))
		{
			pVecScanX = &vecK;
			strScanVar = "k (rlu)";
		}
		else if(!tl::float_equal(spinStartL->value(), spinStopL->value(), g_dEpsRlu))
		{
			pVecScanX = &vecL;
			strScanVar = "l (rlu)";
		}
		else if(!tl::float_equal(spinStartE->value(), spinStopE->value(), g_dEpsRlu))
		{
			pVecScanX = &vecE;
			strScanVar = "E (meV)";
		}
		else
		{
			//QMessageBox::critical(this, "Error", "No scan variable found.");
			fktEnableButtons();
			return;
		}

		QMetaObject::invokeMethod(m_plotwrap.get(), "setAxisTitle",
			Q_ARG(int, QwtPlot::yLeft),
			Q_ARG(const QString&, QString("S(Q,E) (a.u.)")));
		QMetaObject::invokeMethod(m_plotwrap.get(), "setAxisTitle",
			Q_ARG(int, QwtPlot::xBottom),
			Q_ARG(const QString&, QString(strScanVar.c_str())));


		TASReso reso;
		if(!reso.LoadRes(editRes->text().toStdString().c_str()))
		{
			//QMessageBox::critical(this, "Error", "Could not load resolution file.");
			fktEnableButtons();
			return;
		}

		if(bUseScan)	// get crystal definition from scan file
		{
			ublas::vector<t_real> vec1 =
				tl::make_vec({m_scan.plane.vec1[0], m_scan.plane.vec1[1], m_scan.plane.vec1[2]});
			ublas::vector<t_real> vec2 =
				tl::make_vec({m_scan.plane.vec2[0], m_scan.plane.vec2[1], m_scan.plane.vec2[2]});

			reso.SetLattice(m_scan.sample.a, m_scan.sample.b, m_scan.sample.c,
				m_scan.sample.alpha, m_scan.sample.beta, m_scan.sample.gamma,
				vec1, vec2);
		}
		else			// use crystal config file
		{
			if(!reso.LoadLattice(editCrys->text().toStdString().c_str()))
			{
				//QMessageBox::critical(this, "Error", "Could not load crystal file.");
				fktEnableButtons();
				return;
			}
		}

		reso.SetAlgo(ResoAlgo(comboAlgo->currentIndex()+1));
		reso.SetKiFix(comboFixedK->currentIndex()==0);
		reso.SetKFix(spinKfix->value());
		reso.SetOptimalFocus(GetFocus());


		if(m_pSqw == nullptr || !m_pSqw->IsOk())
		{
			//QMessageBox::critical(this, "Error", "No valid S(q,w) model loaded.");
			fktEnableButtons();
			return;
		}



		std::ostringstream ostrOut;
		ostrOut << "#\n";
		ostrOut << "# Format: h k l E S\n";
		ostrOut << "# MC Neutrons: " << iNumNeutrons << "\n";
		ostrOut << "# MC Sample Steps: " << iNumSampleSteps << "\n";
		ostrOut << "#\n";

		QMetaObject::invokeMethod(editStartTime, "setText",
			Q_ARG(const QString&, QString(watch.GetStartTimeStr().c_str())));

		QMetaObject::invokeMethod(progress, "setMaximum", Q_ARG(int, iNumSteps));
		QMetaObject::invokeMethod(progress, "setValue", Q_ARG(int, 0));

		QMetaObject::invokeMethod(textResult, "clear", connty);


		m_vecQ.clear();
		m_vecS.clear();
		m_vecScaledS.clear();

		m_vecQ.reserve(iNumSteps);
		m_vecS.reserve(iNumSteps);
		m_vecScaledS.reserve(iNumSteps);

		unsigned int iNumThreads = bForceDeferred ? 0 : std::thread::hardware_concurrency();

		void (*pThStartFunc)() = []{ tl::init_rand(); };
		tl::ThreadPool<std::pair<bool, t_real>()> tp(iNumThreads, pThStartFunc);
		auto& lstFuts = tp.GetFutures();

		for(unsigned int iStep=0; iStep<iNumSteps; ++iStep)
		{
			t_real dCurH = vecH[iStep];
			t_real dCurK = vecK[iStep];
			t_real dCurL = vecL[iStep];
			t_real dCurE = vecE[iStep];

			tp.AddTask(
			[&reso, dCurH, dCurK, dCurL, dCurE, iNumNeutrons, iNumSampleSteps, this]()
				-> std::pair<bool, t_real>
			{
				if(m_atStop.load()) return std::pair<bool, t_real>(false, 0.);

				t_real dS = 0.;
				t_real dhklE_mean[4] = {0., 0., 0., 0.};

				if(iNumNeutrons == 0)
				{	// if no neutrons are given, just plot the unconvoluted S(q,w)
					dS += (*m_pSqw)(dCurH, dCurK, dCurL, dCurE);
				}
				else
				{	// convolution
					TASReso localreso = reso;
					localreso.SetRandomSamplePos(iNumSampleSteps);
					std::vector<ublas::vector<t_real>> vecNeutrons;

					try
					{
						if(!localreso.SetHKLE(dCurH, dCurK, dCurL, dCurE))
						{
							std::ostringstream ostrErr;
							ostrErr << "Invalid crystal position: (" <<
								dCurH << " " << dCurK << " " << dCurL << ") rlu, "
								<< dCurE << " meV.";
							throw tl::Err(ostrErr.str().c_str());
						}
					}
					catch(const std::exception& ex)
					{
						//QMessageBox::critical(this, "Error", ex.what());
						tl::log_err(ex.what());
						return std::pair<bool, t_real>(false, 0.);
					}

					Ellipsoid4d<t_real> elli =
						localreso.GenerateMC(iNumNeutrons, vecNeutrons);

					for(const ublas::vector<t_real>& vecHKLE : vecNeutrons)
					{
						if(m_atStop.load()) return std::pair<bool, t_real>(false, 0.);

						dS += (*m_pSqw)(vecHKLE[0], vecHKLE[1], vecHKLE[2], vecHKLE[3]);

						for(int i=0; i<4; ++i)
							dhklE_mean[i] += vecHKLE[i];
					}

					dS /= t_real(iNumNeutrons*iNumSampleSteps);
					for(int i=0; i<4; ++i)
						dhklE_mean[i] /= t_real(iNumNeutrons*iNumSampleSteps);

					if(localreso.GetResoParams().flags & CALC_RESVOL)
						dS *= localreso.GetResoResults().dResVol;
					if(localreso.GetResoParams().flags & CALC_R0)
						dS *= localreso.GetResoResults().dR0;
				}
				return std::pair<bool, t_real>(true, dS);
			});
		}

		tp.StartTasks();

		auto iterTask = tp.GetTasks().begin();
		unsigned int iStep = 0;
		for(auto &fut : lstFuts)
		{
			if(m_atStop.load()) break;

			// deferred (in main thread), eval this task manually
			if(iNumThreads == 0)
			{
				(*iterTask)();
				++iterTask;
			}

			std::pair<bool, t_real> pairS = fut.get();
			if(!pairS.first) break;
			t_real dS = pairS.second;
			if(tl::is_nan_or_inf(dS))
			{
				dS = t_real(0);
				tl::log_warn("S(q,w) is invalid.");
			}

			ostrOut.precision(g_iPrec);
			ostrOut << std::left << std::setw(g_iPrec*2) << vecH[iStep] << " "
				<< std::left << std::setw(g_iPrec*2) << vecK[iStep] << " "
				<< std::left << std::setw(g_iPrec*2) << vecL[iStep] << " "
				<< std::left << std::setw(g_iPrec*2) << vecE[iStep] << " "
				<< std::left << std::setw(g_iPrec*2) << dS << "\n";

			m_vecQ.push_back((*pVecScanX)[iStep]);
			m_vecS.push_back(dS);
	 		m_vecScaledS.push_back(dS*dScale + dOffs);

			static const std::vector<t_real> vecNull;
			bool bIsLastStep = (iStep == lstFuts.size()-1);

			if(bLivePlots || bIsLastStep)
			{
				set_qwt_data<t_real_reso>()(*m_plotwrap, m_vecQ, m_vecScaledS, 0, false);
				set_qwt_data<t_real_reso>()(*m_plotwrap, m_vecQ, m_vecScaledS, 1, false);
				if(bUseScan)
					set_qwt_data<t_real_reso>()(*m_plotwrap, m_scan.vecX, m_scan.vecCts, 2, false, &m_scan.vecCtsErr);
				else
					set_qwt_data<t_real_reso>()(*m_plotwrap, vecNull, vecNull, 2, false);

				if(bIsLastStep)
					set_zoomer_base(m_plotwrap->GetZoomer(),
					tl::container_cast<t_real_qwt, t_real, std::vector>()(m_vecQ),
					tl::container_cast<t_real_qwt, t_real, std::vector>()(m_vecScaledS),
					!bForceDeferred, m_plotwrap.get());
				QMetaObject::invokeMethod(m_plotwrap.get(), "doUpdate", connty);
			}

			if(bLiveResults || bIsLastStep)
			{
				if(bIsLastStep)
					ostrOut << "# ------------------------- EOF -------------------------\n";

				QMetaObject::invokeMethod(textResult, "setPlainText", connty,
					Q_ARG(const QString&, QString(ostrOut.str().c_str())));
			}

			QMetaObject::invokeMethod(progress, "setValue", Q_ARG(int, iStep+1));
			QMetaObject::invokeMethod(editStopTime, "setText",
				Q_ARG(const QString&, QString(watch.GetEstStopTimeStr(t_real(iStep+1)/t_real(iNumSteps)).c_str())));
			++iStep;
		}

		// output elapsed time
		watch.stop();
		QMetaObject::invokeMethod(editStopTime, "setText",
			Q_ARG(const QString&, QString(watch.GetStopTimeStr().c_str())));

		fktEnableButtons();
	};


	if(bForceDeferred)
	{
		fkt();
	}
	else
	{
		if(m_pth) { if(m_pth->joinable()) m_pth->join(); delete m_pth; }
		m_pth = new std::thread(std::move(fkt));
	}
}

/**
 * create 2d convolution
 */
void ConvoDlg::Start2D()
{
	m_atStop.store(false);

	bool bLiveResults = m_pLiveResults->isChecked();
	bool bLivePlots = m_pLivePlots->isChecked();

	btnStart->setEnabled(false);
	tabSettings->setEnabled(false);
	m_pMenuBar->setEnabled(false);
	btnStop->setEnabled(true);
	tabWidget->setCurrentWidget(tabPlot2d);

	bool bForceDeferred = false;
	Qt::ConnectionType connty = bForceDeferred
		? Qt::ConnectionType::DirectConnection
		: Qt::ConnectionType::BlockingQueuedConnection;

	std::function<void()> fkt = [this, connty, bForceDeferred, bLiveResults, bLivePlots]
	{
		std::function<void()> fktEnableButtons = [this]
		{
			QMetaObject::invokeMethod(btnStop, "setEnabled", Q_ARG(bool, false));
			QMetaObject::invokeMethod(tabSettings, "setEnabled", Q_ARG(bool, true));
			QMetaObject::invokeMethod(m_pMenuBar, "setEnabled", Q_ARG(bool, true));
			QMetaObject::invokeMethod(btnStart, "setEnabled", Q_ARG(bool, true));
		};

		t_stopwatch watch;
		watch.start();

		const unsigned int iNumNeutrons = spinNeutrons->value();
		const unsigned int iNumSampleSteps = spinSampleSteps->value();

		const unsigned int iNumSteps = std::sqrt(spinStepCnt->value());
		const t_real dStartHKL[] =
		{
			spinStartH->value(), spinStartK->value(),
			spinStartL->value(), spinStartE->value()
		};
		const t_real dDeltaHKL1[] =
		{
			(spinStopH->value() - spinStartH->value()) / t_real(iNumSteps),
			(spinStopK->value() - spinStartK->value()) / t_real(iNumSteps),
			(spinStopL->value() - spinStartL->value()) / t_real(iNumSteps),
			(spinStopE->value() - spinStartE->value()) / t_real(iNumSteps)
		};
		const t_real dDeltaHKL2[] =
		{
			(spinStopH2->value() - spinStartH->value()) / t_real(iNumSteps),
			(spinStopK2->value() - spinStartK->value()) / t_real(iNumSteps),
			(spinStopL2->value() - spinStartL->value()) / t_real(iNumSteps),
			(spinStopE2->value() - spinStartE->value()) / t_real(iNumSteps)
		};


		// -------------------------------------------------------------------------
		// find axis labels and ranges
		std::string strScanVar1 = "";
		t_real dStart1, dStop1;
		if(!tl::float_equal(spinStartH->value(), spinStopH->value(), g_dEpsRlu))
		{
			strScanVar1 = "h (rlu)";
			dStart1 = spinStartH->value();
			dStop1 = spinStopH->value();
		}
		else if(!tl::float_equal(spinStartK->value(), spinStopK->value(), g_dEpsRlu))
		{
			strScanVar1 = "k (rlu)";
			dStart1 = spinStartK->value();
			dStop1 = spinStopK->value();
		}
		else if(!tl::float_equal(spinStartL->value(), spinStopL->value(), g_dEpsRlu))
		{
			strScanVar1 = "l (rlu)";
			dStart1 = spinStartL->value();
			dStop1 = spinStopL->value();
		}
		else if(!tl::float_equal(spinStartE->value(), spinStopE->value(), g_dEpsRlu))
		{
			strScanVar1 = "E (meV)";
			dStart1 = spinStartE->value();
			dStop1 = spinStopE->value();
		}

		std::string strScanVar2 = "";
		t_real dStart2, dStop2;
		if(!tl::float_equal(spinStartH->value(), spinStopH2->value(), g_dEpsRlu))
		{
			strScanVar2 = "h (rlu)";
			dStart2 = spinStartH->value();
			dStop2 = spinStopH2->value();
		}
		else if(!tl::float_equal(spinStartK->value(), spinStopK2->value(), g_dEpsRlu))
		{
			strScanVar2 = "k (rlu)";
			dStart2 = spinStartK->value();
			dStop2 = spinStopK2->value();
		}
		else if(!tl::float_equal(spinStartL->value(), spinStopL2->value(), g_dEpsRlu))
		{
			strScanVar2 = "l (rlu)";
			dStart2 = spinStartL->value();
			dStop2 = spinStopL2->value();
		}
		else if(!tl::float_equal(spinStartE->value(), spinStopE2->value(), g_dEpsRlu))
		{
			strScanVar2 = "E (meV)";
			dStart2 = spinStartE->value();
			dStop2 = spinStopE2->value();
		}

		QMetaObject::invokeMethod(m_plotwrap2d.get(), "setAxisTitle",
			Q_ARG(int, QwtPlot::xBottom),
			Q_ARG(const QString&, QString(strScanVar1.c_str())));

		QMetaObject::invokeMethod(m_plotwrap2d.get(), "setAxisTitle",
			Q_ARG(int, QwtPlot::yLeft),
			Q_ARG(const QString&, QString(strScanVar2.c_str())));
		// -------------------------------------------------------------------------


		TASReso reso;
		if(!reso.LoadRes(editRes->text().toStdString().c_str()))
		{
			//QMessageBox::critical(this, "Error", "Could not load resolution file.");
			fktEnableButtons();
			return;
		}

		if(!reso.LoadLattice(editCrys->text().toStdString().c_str()))
		{
			//QMessageBox::critical(this, "Error", "Could not load crystal file.");
			fktEnableButtons();
			return;
		}

		reso.SetAlgo(ResoAlgo(comboAlgo->currentIndex()+1));
		reso.SetKiFix(comboFixedK->currentIndex()==0);
		reso.SetKFix(spinKfix->value());
		reso.SetOptimalFocus(GetFocus());

		if(m_pSqw == nullptr || !m_pSqw->IsOk())
		{
			//QMessageBox::critical(this, "Error", "No valid S(q,w) model loaded.");
			fktEnableButtons();
			return;
		}


		std::ostringstream ostrOut;
		ostrOut << "#\n";
		ostrOut << "# Format: h k l E S\n";
		ostrOut << "# MC Neutrons: " << iNumNeutrons << "\n";
		ostrOut << "# MC Sample Steps: " << iNumSampleSteps << "\n";
		ostrOut << "#\n";

		QMetaObject::invokeMethod(editStartTime2d, "setText",
			Q_ARG(const QString&, QString(watch.GetStartTimeStr().c_str())));

		QMetaObject::invokeMethod(progress, "setMaximum", Q_ARG(int, iNumSteps*iNumSteps));
		QMetaObject::invokeMethod(progress, "setValue", Q_ARG(int, 0));

		QMetaObject::invokeMethod(textResult, "clear", connty);

		// raster width & height
		m_plotwrap2d->GetRaster()->Init(iNumSteps, iNumSteps);
		m_plotwrap2d->GetRaster()->SetXRange(dStart1, dStop1);
		m_plotwrap2d->GetRaster()->SetYRange(dStart2, dStop2);
		set_zoomer_base(m_plotwrap2d->GetZoomer(),
			m_plotwrap2d->GetRaster()->GetXMin(), m_plotwrap2d->GetRaster()->GetXMax(),
			m_plotwrap2d->GetRaster()->GetYMax(), m_plotwrap2d->GetRaster()->GetYMin(),
			!bForceDeferred, m_plotwrap2d.get());

		std::vector<t_real> vecH; vecH.reserve(iNumSteps*iNumSteps);
		std::vector<t_real> vecK; vecK.reserve(iNumSteps*iNumSteps);
		std::vector<t_real> vecL; vecL.reserve(iNumSteps*iNumSteps);
		std::vector<t_real> vecE; vecE.reserve(iNumSteps*iNumSteps);

		for(unsigned int iStepY=0; iStepY<iNumSteps; ++iStepY)
		{
			for(unsigned int iStepX=0; iStepX<iNumSteps; ++iStepX)
			{
				vecH.push_back(dStartHKL[0] + dDeltaHKL2[0]*t_real(iStepY) + dDeltaHKL1[0]*t_real(iStepX));
				vecK.push_back(dStartHKL[1] + dDeltaHKL2[1]*t_real(iStepY) + dDeltaHKL1[1]*t_real(iStepX));
				vecL.push_back(dStartHKL[2] + dDeltaHKL2[2]*t_real(iStepY) + dDeltaHKL1[2]*t_real(iStepX));
				vecE.push_back(dStartHKL[3] + dDeltaHKL2[3]*t_real(iStepY) + dDeltaHKL1[3]*t_real(iStepX));
			}
		}

		unsigned int iNumThreads = bForceDeferred ? 0 : std::thread::hardware_concurrency();

		void (*pThStartFunc)() = []{ tl::init_rand(); };
		tl::ThreadPool<std::pair<bool, t_real>()> tp(iNumThreads, pThStartFunc);
		auto& lstFuts = tp.GetFutures();

		for(unsigned int iStep=0; iStep<iNumSteps*iNumSteps; ++iStep)
		{
			t_real dCurH = vecH[iStep];
			t_real dCurK = vecK[iStep];
			t_real dCurL = vecL[iStep];
			t_real dCurE = vecE[iStep];

			tp.AddTask(
			[&reso, dCurH, dCurK, dCurL, dCurE, iNumNeutrons, iNumSampleSteps, this]()
				-> std::pair<bool, t_real>
			{
				if(m_atStop.load()) return std::pair<bool, t_real>(false, 0.);

				t_real dS = 0.;
				t_real dhklE_mean[4] = {0., 0., 0., 0.};

				if(iNumNeutrons == 0)
				{	// if no neutrons are given, just plot the unconvoluted S(q,w)
					dS += (*m_pSqw)(dCurH, dCurK, dCurL, dCurE);
				}
				else
				{	// convolution
					TASReso localreso = reso;
					localreso.SetRandomSamplePos(iNumSampleSteps);
					std::vector<ublas::vector<t_real>> vecNeutrons;

					try
					{
						if(!localreso.SetHKLE(dCurH, dCurK, dCurL, dCurE))
						{
							std::ostringstream ostrErr;
							ostrErr << "Invalid crystal position: (" <<
								dCurH << " " << dCurK << " " << dCurL << ") rlu, "
								<< dCurE << " meV.";
							throw tl::Err(ostrErr.str().c_str());
						}
					}
					catch(const std::exception& ex)
					{
						//QMessageBox::critical(this, "Error", ex.what());
						tl::log_err(ex.what());
						return std::pair<bool, t_real>(false, 0.);
					}

					Ellipsoid4d<t_real> elli =
						localreso.GenerateMC(iNumNeutrons, vecNeutrons);

					for(const ublas::vector<t_real>& vecHKLE : vecNeutrons)
					{
						if(m_atStop.load()) return std::pair<bool, t_real>(false, 0.);

						dS += (*m_pSqw)(vecHKLE[0], vecHKLE[1], vecHKLE[2], vecHKLE[3]);

						for(int i=0; i<4; ++i)
							dhklE_mean[i] += vecHKLE[i];
					}

					dS /= t_real(iNumNeutrons*iNumSampleSteps);
					for(int i=0; i<4; ++i)
						dhklE_mean[i] /= t_real(iNumNeutrons*iNumSampleSteps);

					if(localreso.GetResoParams().flags & CALC_RESVOL)
						dS *= localreso.GetResoResults().dResVol;
					if(localreso.GetResoParams().flags & CALC_R0)
						dS *= localreso.GetResoResults().dR0;
				}
				return std::pair<bool, t_real>(true, dS);
			});
		}

		tp.StartTasks();

		auto iterTask = tp.GetTasks().begin();
		unsigned int iStep = 0;
		for(auto &fut : lstFuts)
		{
			if(m_atStop.load()) break;

			// deferred (in main thread), eval this task manually
			if(iNumThreads == 0)
			{
				(*iterTask)();
				++iterTask;
			}

			std::pair<bool, t_real> pairS = fut.get();
			if(!pairS.first) break;
			t_real dS = pairS.second;
			if(tl::is_nan_or_inf(dS))
			{
				dS = t_real(0);
				tl::log_warn("S(q,w) is invalid.");
			}

			ostrOut.precision(g_iPrec);
			ostrOut << std::left << std::setw(g_iPrec*2) << vecH[iStep] << " "
				<< std::left << std::setw(g_iPrec*2) << vecK[iStep] << " "
				<< std::left << std::setw(g_iPrec*2) << vecL[iStep] << " "
				<< std::left << std::setw(g_iPrec*2) << vecE[iStep] << " "
				<< std::left << std::setw(g_iPrec*2) << dS << "\n";

			m_plotwrap2d->GetRaster()->SetPixel(iStep%iNumSteps, iStep/iNumSteps, t_real_qwt(dS));

			bool bIsLastStep = (iStep == lstFuts.size()-1);

			if(bLivePlots || bIsLastStep)
			{
				m_plotwrap2d->GetRaster()->SetZRange();

				QMetaObject::invokeMethod(m_plotwrap2d.get(), "scaleColorBar", connty);
				QMetaObject::invokeMethod(m_plotwrap2d.get(), "doUpdate", connty);
			}

			if(bLiveResults || bIsLastStep)
			{
				if(bIsLastStep)
					ostrOut << "# ------------------------- EOF -------------------------\n";
				QMetaObject::invokeMethod(textResult, "setPlainText", connty,
					Q_ARG(const QString&, QString(ostrOut.str().c_str())));
			}

			QMetaObject::invokeMethod(progress, "setValue", Q_ARG(int, iStep+1));
			QMetaObject::invokeMethod(editStopTime2d, "setText",
				Q_ARG(const QString&, QString(watch.GetEstStopTimeStr(t_real(iStep+1)/t_real(iNumSteps*iNumSteps)).c_str())));

			++iStep;
		}

		// output elapsed time
		watch.stop();
		QMetaObject::invokeMethod(editStopTime2d, "setText",
			Q_ARG(const QString&, QString(watch.GetStopTimeStr().c_str())));

		fktEnableButtons();
	};


	if(bForceDeferred)
	{
		fkt();
	}
	else
	{
		if(m_pth) { if(m_pth->joinable()) m_pth->join(); delete m_pth; }
		m_pth = new std::thread(std::move(fkt));
	}
}


/**
 * start dispersion plot
 */
void ConvoDlg::StartDisp()
{
	m_atStop.store(false);
	ClearPlot1D();

	bool bLiveResults = m_pLiveResults->isChecked();
	bool bLivePlots = m_pLivePlots->isChecked();

	btnStart->setEnabled(false);
	tabSettings->setEnabled(false);
	m_pMenuBar->setEnabled(false);
	btnStop->setEnabled(true);
	tabWidget->setCurrentWidget(tabPlot);

	bool bForceDeferred = false;
	Qt::ConnectionType connty = bForceDeferred
		? Qt::ConnectionType::DirectConnection
		: Qt::ConnectionType::BlockingQueuedConnection;

	std::function<void()> fkt = [this, connty, bForceDeferred, bLiveResults, bLivePlots]
	{
		std::function<void()> fktEnableButtons = [this]
		{
			QMetaObject::invokeMethod(btnStop, "setEnabled", Q_ARG(bool, false));
			QMetaObject::invokeMethod(tabSettings, "setEnabled", Q_ARG(bool, true));
			QMetaObject::invokeMethod(m_pMenuBar, "setEnabled", Q_ARG(bool, true));
			QMetaObject::invokeMethod(btnStart, "setEnabled", Q_ARG(bool, true));
		};

		t_stopwatch watch;
		watch.start();

		const unsigned int iNumSteps = spinStepCnt->value();
		std::vector<t_real> vecH = tl::linspace<t_real,t_real>(
			spinStartH->value(), spinStopH->value(), iNumSteps);
		std::vector<t_real> vecK = tl::linspace<t_real,t_real>(
			spinStartK->value(), spinStopK->value(), iNumSteps);
		std::vector<t_real> vecL = tl::linspace<t_real,t_real>(
			spinStartL->value(), spinStopL->value(), iNumSteps);

		std::string strScanVar = "";
		std::vector<t_real> *pVecScanX = nullptr;
		if(!tl::float_equal(spinStartH->value(), spinStopH->value(), g_dEpsRlu))
		{
			pVecScanX = &vecH;
			strScanVar = "h (rlu)";
		}
		else if(!tl::float_equal(spinStartK->value(), spinStopK->value(), g_dEpsRlu))
		{
			pVecScanX = &vecK;
			strScanVar = "k (rlu)";
		}
		else if(!tl::float_equal(spinStartL->value(), spinStopL->value(), g_dEpsRlu))
		{
			pVecScanX = &vecL;
			strScanVar = "l (rlu)";
		}
		else
		{
			//QMessageBox::critical(this, "Error", "No scan variable found.");
			fktEnableButtons();
			return;
		}

		QMetaObject::invokeMethod(m_plotwrap.get(), "setAxisTitle",
			Q_ARG(int, QwtPlot::yLeft),
			Q_ARG(const QString&, QString("E(Q) (meV)")));
		QMetaObject::invokeMethod(m_plotwrap.get(), "setAxisTitle",
			Q_ARG(int, QwtPlot::xBottom),
			Q_ARG(const QString&, QString(strScanVar.c_str())));


		if(m_pSqw == nullptr || !m_pSqw->IsOk())
		{
			//QMessageBox::critical(this, "Error", "No valid S(q,w) model loaded.");
			fktEnableButtons();
			return;
		}



		std::ostringstream ostrOut;
		ostrOut << "#\n";
		ostrOut << "# Format: h k l E1 w1 E2 w2 ... En wn\n";
		ostrOut << "#\n";

		QMetaObject::invokeMethod(editStartTime, "setText",
			Q_ARG(const QString&, QString(watch.GetStartTimeStr().c_str())));

		QMetaObject::invokeMethod(progress, "setMaximum", Q_ARG(int, iNumSteps));
		QMetaObject::invokeMethod(progress, "setValue", Q_ARG(int, 0));

		QMetaObject::invokeMethod(textResult, "clear", connty);


		m_vecvecQ.clear();
		m_vecvecE.clear();
		m_vecvecW.clear();

		unsigned int iNumThreads = bForceDeferred ? 0 : std::thread::hardware_concurrency();

		tl::ThreadPool<std::tuple<bool, std::vector<t_real>, std::vector<t_real>>()>
			tp(iNumThreads);
		auto& lstFuts = tp.GetFutures();

		for(unsigned int iStep=0; iStep<iNumSteps; ++iStep)
		{
			t_real dCurH = vecH[iStep];
			t_real dCurK = vecK[iStep];
			t_real dCurL = vecL[iStep];

			tp.AddTask([dCurH, dCurK, dCurL, this]() ->
			std::tuple<bool, std::vector<t_real>, std::vector<t_real>>
			{
				if(m_atStop.load())
					return std::make_tuple(false, std::vector<t_real>(), std::vector<t_real>());

				std::vector<t_real> vecE, vecW;
				std::tie(vecE, vecW) = m_pSqw->disp(dCurH, dCurK, dCurL);
				return std::tuple<bool, std::vector<t_real>, std::vector<t_real>>
					(true, vecE, vecW);
			});
		}

		tp.StartTasks();

		auto iterTask = tp.GetTasks().begin();
		unsigned int iStep = 0;
		for(auto &fut : lstFuts)
		{
			if(m_atStop.load()) break;

			// deferred (in main thread), eval this task manually
			if(iNumThreads == 0)
			{
				(*iterTask)();
				++iterTask;
			}

			auto tupEW = fut.get();
			if(!std::get<0>(tupEW)) break;

			ostrOut.precision(g_iPrec);
			ostrOut << std::left << std::setw(g_iPrec*2) << vecH[iStep] << " "
				<< std::left << std::setw(g_iPrec*2) << vecK[iStep] << " "
				<< std::left << std::setw(g_iPrec*2) << vecL[iStep] << " ";
			for(std::size_t iE=0; iE<std::get<1>(tupEW).size(); ++iE)
			{
				ostrOut << std::left << std::setw(g_iPrec*2) << std::get<1>(tupEW)[iE] << " ";
				ostrOut << std::left << std::setw(g_iPrec*2) << std::get<2>(tupEW)[iE] << " ";
			}
			ostrOut << "\n";


			// store dispersion branches as separate curves
			if(std::get<1>(tupEW).size() > m_vecvecE.size())
			{
				m_vecvecQ.resize(std::get<1>(tupEW).size());
				m_vecvecE.resize(std::get<1>(tupEW).size());
				m_vecvecW.resize(std::get<2>(tupEW).size());
			}
			for(std::size_t iBranch=0; iBranch<std::get<1>(tupEW).size(); ++iBranch)
			{
				m_vecvecQ[iBranch].push_back((*pVecScanX)[iStep]);
				m_vecvecE[iBranch].push_back(std::get<1>(tupEW)[iBranch]);
				m_vecvecW[iBranch].push_back(std::get<2>(tupEW)[iBranch]);
			}


			bool bIsLastStep = (iStep == lstFuts.size()-1);

			if(bLivePlots || bIsLastStep)
			{
				for(std::size_t iBranch=0; iBranch<m_vecvecE.size() && iBranch+DISP_CURVE_START<MAX_CURVES; ++iBranch)
				{
					set_qwt_data<t_real_reso>()(*m_plotwrap, m_vecvecQ[iBranch], m_vecvecE[iBranch], DISP_CURVE_START+iBranch, false);
				}

				if(bIsLastStep)
					set_zoomer_base(m_plotwrap->GetZoomer(),
					tl::container2_cast<t_real_qwt, t_real, std::vector>()(m_vecvecQ),
					tl::container2_cast<t_real_qwt, t_real, std::vector>()(m_vecvecE),
					!bForceDeferred, m_plotwrap.get());
				QMetaObject::invokeMethod(m_plotwrap.get(), "doUpdate", connty);
			}

			if(bLiveResults || bIsLastStep)
			{
				if(bIsLastStep)
					ostrOut << "# ------------------------- EOF -------------------------\n";

				QMetaObject::invokeMethod(textResult, "setPlainText", connty,
					Q_ARG(const QString&, QString(ostrOut.str().c_str())));
			}

			QMetaObject::invokeMethod(progress, "setValue", Q_ARG(int, iStep+1));
			QMetaObject::invokeMethod(editStopTime, "setText",
				Q_ARG(const QString&, QString(watch.GetEstStopTimeStr(t_real(iStep+1)/t_real(iNumSteps)).c_str())));
			++iStep;
		}

		// output elapsed time
		watch.stop();
		QMetaObject::invokeMethod(editStopTime, "setText",
			Q_ARG(const QString&, QString(watch.GetStopTimeStr().c_str())));

		fktEnableButtons();
	};


	if(bForceDeferred)
	{
		fkt();
	}
	else
	{
		if(m_pth) { if(m_pth->joinable()) m_pth->join(); delete m_pth; }
		m_pth = new std::thread(std::move(fkt));
	}
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


void ConvoDlg::Stop()
{
	m_atStop.store(true);
}


// -----------------------------------------------------------------------------


void ConvoDlg::ShowFavourites()
{
	m_pFavDlg->show();
	m_pFavDlg->activateWindow();
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
	std::string strFile  = qstrFile.toStdString();
	tl::trim(strFile);
	if(strFile == "") return;
	if(!checkScan->isChecked()) return;

	std::vector<std::string> vecFiles{strFile};

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
	m_pSqwParamDlg->show();
	m_pSqwParamDlg->activateWindow();
}


#include "libs/version.h"

void ConvoDlg::ShowAboutDlg()
{
	std::ostringstream ostrAbout;
	ostrAbout << "Takin/Monteconvo version " << TAKIN_VER << ".\n";
	ostrAbout << "Written by Tobias Weber <tobias.weber@tum.de>,\n";
	ostrAbout << "2015 - 2016.\n";

	QMessageBox::about(this, "About Monteconvo", ostrAbout.str().c_str());
}


#include "ConvoDlg.moc"
