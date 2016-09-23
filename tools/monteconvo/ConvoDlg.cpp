/**
 * monte carlo convolution tool
 * @author tweber
 * @date 2015, 2016
 * @license GPLv2
 */

#include "ConvoDlg.h"
#include "tlibs/string/string.h"
#include "tlibs/math/math.h"
#include "tlibs/helper/thread.h"
#include "tlibs/time/stopwatch.h"

#ifndef NO_PY
	#include "sqw_py.h"
#endif
#include "TASReso.h"

#include "libs/globals.h"
#include "libs/globals_qt.h"

#include <iostream>
#include <fstream>

#include <QFileDialog>
#include <QMessageBox>
#include <QMenu>


using t_real = t_real_reso;
using t_stopwatch = tl::Stopwatch<t_real>;


ConvoDlg::ConvoDlg(QWidget* pParent, QSettings* pSett)
	: QDialog(pParent, Qt::WindowTitleHint|Qt::WindowCloseButtonHint|Qt::WindowMinMaxButtonsHint),
		m_pSett(pSett)
{
	setupUi(this);
	if(m_pSett)
	{
		QFont font;
		if(m_pSett->contains("main/font_gen") && font.fromString(m_pSett->value("main/font_gen", "").toString()))
			setFont(font);
	}

	btnStart->setIcon(load_icon("res/icons/media-playback-start.svg"));
	btnStop->setIcon(load_icon("res/icons/media-playback-stop.svg"));
	btnSaveResult->setIcon(load_icon("res/icons/document-save-as.svg"));


	m_plotwrap.reset(new QwtPlotWrapper(plot, 3, true));
	m_plotwrap->GetPlot()->setAxisTitle(QwtPlot::xBottom, "");
	m_plotwrap->GetPlot()->setAxisTitle(QwtPlot::yLeft, "S (a.u.)");


	// --------------------------------------------------------------------
	QPen penCurve;
	penCurve.setColor(QColor(0,0,0x99));
	penCurve.setWidth(2);
	m_plotwrap->GetCurve(0)->setPen(penCurve);
	m_plotwrap->GetCurve(0)->setStyle(QwtPlotCurve::CurveStyle::Lines);
	m_plotwrap->GetCurve(0)->setTitle("S(Q,w)");

	QPen penPoints;
	penPoints.setColor(QColor(0xff,0,0));
	penPoints.setWidth(4);
	m_plotwrap->GetCurve(1)->setPen(penPoints);
	m_plotwrap->GetCurve(1)->setStyle(QwtPlotCurve::CurveStyle::Dots);
	m_plotwrap->GetCurve(1)->setTitle("S(Q,w)");

	QPen penScanPoints;
	penScanPoints.setColor(QColor(0x00,0x90,0x00));
	penScanPoints.setWidth(6);
	m_plotwrap->GetCurve(2)->setPen(penScanPoints);
	m_plotwrap->GetCurve(2)->setStyle(QwtPlotCurve::CurveStyle::Dots);
	m_plotwrap->GetCurve(2)->setTitle("S(Q,w)");
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



	m_pSqwParamDlg = new SqwParamDlg(this, m_pSett);
	QObject::connect(this, SIGNAL(SqwLoaded(const std::vector<SqwBase::t_var>&)),
		m_pSqwParamDlg, SLOT(SqwLoaded(const std::vector<SqwBase::t_var>&)));
	QObject::connect(m_pSqwParamDlg, SIGNAL(SqwParamsChanged(const std::vector<SqwBase::t_var>&)),
		this, SLOT(SqwParamsChanged(const std::vector<SqwBase::t_var>&)));

	m_pFavDlg = new FavDlg(this, m_pSett);
	QObject::connect(m_pFavDlg, SIGNAL(ChangePos(const struct FavHklPos&)),
		this, SLOT(ChangePos(const struct FavHklPos&)));

	QObject::connect(buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(ButtonBoxClicked(QAbstractButton*)));

	QObject::connect(btnBrowseCrys, SIGNAL(clicked()), this, SLOT(browseCrysFiles()));
	QObject::connect(btnBrowseRes, SIGNAL(clicked()), this, SLOT(browseResoFiles()));
	QObject::connect(btnBrowseSqw, SIGNAL(clicked()), this, SLOT(browseSqwFiles()));
	QObject::connect(btnBrowseScan, SIGNAL(clicked()), this, SLOT(browseScanFiles()));

	QObject::connect(btnFav, SIGNAL(clicked()), this, SLOT(ShowFavourites()));

	QObject::connect(btnSqwParams, SIGNAL(clicked()), this, SLOT(showSqwParamDlg()));
	QObject::connect(btnSaveResult, SIGNAL(clicked()), this, SLOT(SaveResult()));

	QObject::connect(comboSqw, SIGNAL(currentIndexChanged(int)), this, SLOT(SqwModelChanged(int)));
	QObject::connect(editSqw, SIGNAL(textChanged(const QString&)), this, SLOT(createSqwModel(const QString&)));
	QObject::connect(editScan, SIGNAL(textChanged(const QString&)), this, SLOT(scanFileChanged(const QString&)));

	QObject::connect(editScale, SIGNAL(textChanged(const QString&)), this, SLOT(scaleChanged()));
	QObject::connect(editOffs, SIGNAL(textChanged(const QString&)), this, SLOT(scaleChanged()));

	QObject::connect(btnStart, SIGNAL(clicked()), this, SLOT(Start()));
	QObject::connect(btnStop, SIGNAL(clicked()), this, SLOT(Stop()));


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

	if(m_pSqwParamDlg)
	{
		delete m_pSqwParamDlg;
		m_pSqwParamDlg = nullptr;
	}

	if(m_pFavDlg)
	{
		delete m_pFavDlg;
		m_pFavDlg = nullptr;
	}
}


void ConvoDlg::SaveResult()
{
	QFileDialog::Option fileopt = QFileDialog::Option(0);
	if(m_pSett && !m_pSett->value("main/native_dialogs", 1).toBool())
		fileopt = QFileDialog::DontUseNativeDialog;

	QString strDirLast = ".";
	if(m_pSett)
		strDirLast = m_pSett->value("convo/last_dir_result", ".").toString();

	QString strFile = QFileDialog::getSaveFileName(this,
		"Save Scan", strDirLast, "Data Files (*.dat *.DAT)", nullptr, fileopt);

	if(strFile == "")
		return;

	std::string strFile1 = strFile.toStdString();
	std::string strDir = tl::get_dir(strFile1);

	std::ofstream ofstr(strFile1);
	if(!ofstr)
	{
		QMessageBox::critical(this, "Error", "Could not open file.");
		return;
	}

	std::string strResult = textResult->toPlainText().toStdString();
	ofstr.write(strResult.c_str(), strResult.size());

	if(m_pSett)
		m_pSett->setValue("convo/last_dir_result", QString(strDir.c_str()));
}


void ConvoDlg::SqwModelChanged(int)
{
	editSqw->clear();
	emit SqwLoaded(std::vector<SqwBase::t_var>{});
}

void ConvoDlg::createSqwModel(const QString& qstrFile)
{
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
		QMessageBox::critical(this, "Error", "No S(q,w) config file given.");
		return;
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
		emit SqwLoaded(m_pSqw->GetVars());
	else
	{
		QMessageBox::critical(this, "Error", "Could not create S(q,w).");
		return;
	}
}


void ConvoDlg::SqwParamsChanged(const std::vector<SqwBase::t_var>& vecVars)
{
	if(!m_pSqw)
		return;
	m_pSqw->SetVars(vecVars);

#ifndef NDEBUG
	// check: read parameters back in
	emit SqwLoaded(m_pSqw->GetVars());
#endif
}


void ConvoDlg::Start()
{
	bool bUseScan = m_bUseScan && checkScan->isChecked();
	t_real dScale = tl::str_to_var<t_real>(editScale->text().toStdString());
	t_real dOffs = tl::str_to_var<t_real>(editOffs->text().toStdString());

	m_atStop.store(false);

	btnStart->setEnabled(false);
	tabSettings->setEnabled(false);
	btnStop->setEnabled(true);
	tabWidget->setCurrentWidget(tabPlot);

	bool bForceDeferred = false;
	/*std::string strSqwIdent = comboSqw->itemData(comboSqw->currentIndex()).toString().toStdString();
	if(strSqwIdent == "py")
		bForceDeferred = true;*/
	Qt::ConnectionType connty = bForceDeferred ? Qt::ConnectionType::DirectConnection
			: Qt::ConnectionType::BlockingQueuedConnection;

	std::function<void()> fkt = [this, connty, bForceDeferred, bUseScan, dScale, dOffs]
	{
		std::function<void()> fktEnableButtons = [this]
		{
			QMetaObject::invokeMethod(btnStop, "setEnabled", Q_ARG(bool, false));
			QMetaObject::invokeMethod(tabSettings, "setEnabled", Q_ARG(bool, true));
			QMetaObject::invokeMethod(btnStart, "setEnabled", Q_ARG(bool, true));
		};

		t_stopwatch watch;
		watch.start();

		const bool bUseR0 = true;
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
		if(!tl::float_equal(spinStartH->value(), spinStopH->value(), 0.0001))
		{
			pVecScanX = &vecH;
			strScanVar = "h (rlu)";
		}
		else if(!tl::float_equal(spinStartK->value(), spinStopK->value(), 0.0001))
		{
			pVecScanX = &vecK;
			strScanVar = "k (rlu)";
		}
		else if(!tl::float_equal(spinStartL->value(), spinStopL->value(), 0.0001))
		{
			pVecScanX = &vecL;
			strScanVar = "l (rlu)";
		}
		else if(!tl::float_equal(spinStartE->value(), spinStopE->value(), 0.0001))
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

		//QMetaObject::invokeMethod(plot, "setAxisTitle",
		//	Q_ARG(int, QwtPlot::xBottom),
		//	Q_ARG(const QString&, QString(strScanVar.c_str())));
		//plot->setAxisTitle(QwtPlot::xBottom, strScanVar.c_str());


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

		const int iFocMono = comboFocMono->currentIndex();
		const int iFocAna = comboFocAna->currentIndex();

		unsigned ifocMode = unsigned(ResoFocus::FOC_NONE);
		if(iFocMono == 1) ifocMode |= unsigned(ResoFocus::FOC_MONO_H);	// horizontal
		else if(iFocMono == 2) ifocMode |= unsigned(ResoFocus::FOC_MONO_V);	// vertical
		else if(iFocMono == 3) ifocMode |= unsigned(ResoFocus::FOC_MONO_V)|unsigned(ResoFocus::FOC_MONO_H);		// both
		if(iFocAna == 1) ifocMode |= unsigned(ResoFocus::FOC_ANA_H);	// horizontal
		else if(iFocAna == 2) ifocMode |= unsigned(ResoFocus::FOC_ANA_V);	// vertical
		else if(iFocAna == 3) ifocMode |= unsigned(ResoFocus::FOC_ANA_V)|unsigned(ResoFocus::FOC_ANA_H);		// both

		ResoFocus focMode = ResoFocus(ifocMode);
		reso.SetOptimalFocus(focMode);


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

		tl::ThreadPool<std::pair<bool, t_real>()> tp(iNumThreads);
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

				t_real dS = 0.;
				t_real dhklE_mean[4] = {0., 0., 0., 0.};

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

				if(bUseR0)
					dS *= localreso.GetResoResults().dResVol;
				if(bUseR0 && (localreso.GetResoParams().flags & CALC_R0))
					dS *= localreso.GetResoResults().dR0;

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
			if(!pairS.first)
				break;
			t_real dS = pairS.second;

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

			set_qwt_data<t_real_reso>()(*m_plotwrap, m_vecQ, m_vecScaledS, 0, false);
			set_qwt_data<t_real_reso>()(*m_plotwrap, m_vecQ, m_vecScaledS, 1, false);
			if(bUseScan)
				set_qwt_data<t_real_reso>()(*m_plotwrap, m_scan.vecX, m_scan.vecCts, 2, false);
			else
				set_qwt_data<t_real_reso>()(*m_plotwrap, vecNull, vecNull, 2, false);

			//set_zoomer_base(m_plotwrap->GetZoomer(), m_vecQ, m_vecScaledS, true);
			QMetaObject::invokeMethod(m_plotwrap->GetPlot(), "replot", connty);

			QMetaObject::invokeMethod(textResult, "setPlainText", connty,
				Q_ARG(const QString&, QString(ostrOut.str().c_str())));

			QMetaObject::invokeMethod(progress, "setValue", Q_ARG(int, iStep+1));
			QMetaObject::invokeMethod(editStopTime, "setText",
				Q_ARG(const QString&, QString(watch.GetEstStopTimeStr(t_real(iStep+1)/t_real(iNumSteps)).c_str())));

			++iStep;
		}

		ostrOut << "# ---------------- EOF ----------------\n";

		QMetaObject::invokeMethod(textResult, "setPlainText", connty,
			Q_ARG(const QString&, QString(ostrOut.str().c_str())));

		watch.stop();
		QMetaObject::invokeMethod(editStopTime, "setText",
			Q_ARG(const QString&, QString(watch.GetStopTimeStr().c_str())));

		fktEnableButtons();
	};


	if(bForceDeferred)
		fkt();
	else
	{
		if(m_pth) { if(m_pth->joinable()) m_pth->join(); delete m_pth; }
		m_pth = new std::thread(std::move(fkt));
	}
}


void ConvoDlg::Stop()
{
	m_atStop.store(true);
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

void ConvoDlg::browseCrysFiles()
{
	QFileDialog::Option fileopt = QFileDialog::Option(0);
	if(m_pSett && !m_pSett->value("main/native_dialogs", 1).toBool())
		fileopt = QFileDialog::DontUseNativeDialog;

	QString strDirLast = ".";
	if(m_pSett)
		strDirLast = m_pSett->value("convo/last_dir_crys", ".").toString();
	QString strFile = QFileDialog::getOpenFileName(this,
		"Open Crystal File...", strDirLast, "Takin files (*.taz *.TAZ)",
		nullptr, fileopt);
	if(strFile == "")
		return;

	editCrys->setText(strFile);

	std::string strDir = tl::get_dir(strFile.toStdString());
	if(m_pSett)
		m_pSett->setValue("convo/last_dir_crys", QString(strDir.c_str()));
}

void ConvoDlg::browseResoFiles()
{
	QFileDialog::Option fileopt = QFileDialog::Option(0);
	if(m_pSett && !m_pSett->value("main/native_dialogs", 1).toBool())
		fileopt = QFileDialog::DontUseNativeDialog;

	QString strDirLast = ".";
	if(m_pSett)
		strDirLast = m_pSett->value("convo/last_dir_reso", ".").toString();
	QString strFile = QFileDialog::getOpenFileName(this,
		"Open Resolution File...", strDirLast, "Takin files (*.taz *.TAZ)",
		nullptr, fileopt);
	if(strFile == "")
		return;

	editRes->setText(strFile);

	std::string strDir = tl::get_dir(strFile.toStdString());
	if(m_pSett)
		m_pSett->setValue("convo/last_dir_reso", QString(strDir.c_str()));
}

void ConvoDlg::browseSqwFiles()
{
	QFileDialog::Option fileopt = QFileDialog::Option(0);
	if(m_pSett && !m_pSett->value("main/native_dialogs", 1).toBool())
		fileopt = QFileDialog::DontUseNativeDialog;

	QString strDirLast = ".";
	if(m_pSett)
		strDirLast = m_pSett->value("convo/last_dir_sqw", ".").toString();
	QString strFile = QFileDialog::getOpenFileName(this,
		"Open S(q,w) File...", strDirLast, "All S(q,w) files (*.dat *.DAT *.py *.PY)",
		nullptr, fileopt);
	if(strFile == "")
		return;

	editSqw->setText(strFile);

	std::string strDir = tl::get_dir(strFile.toStdString());
	if(m_pSett)
		m_pSett->setValue("convo/last_dir_sqw", QString(strDir.c_str()));
}

void ConvoDlg::browseScanFiles()
{
	QFileDialog::Option fileopt = QFileDialog::Option(0);
	if(m_pSett && !m_pSett->value("main/native_dialogs", 1).toBool())
		fileopt = QFileDialog::DontUseNativeDialog;

	QString strDirLast = ".";
	if(m_pSett)
		strDirLast = m_pSett->value("convo/last_dir_scan", ".").toString();
	QString strFile = QFileDialog::getOpenFileName(this,
		"Open S(q,w) File...", strDirLast, "All scan files (*.dat *.DAT *.scn *.SCN)",
		nullptr, fileopt);
	if(strFile == "")
		return;

	editScan->setText(strFile);

	std::string strDir = tl::get_dir(strFile.toStdString());
	if(m_pSett)
		m_pSett->setValue("convo/last_dir_scan", QString(strDir.c_str()));
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


void ConvoDlg::showSqwParamDlg()
{
	m_pSqwParamDlg->show();
	m_pSqwParamDlg->activateWindow();
}

void ConvoDlg::LoadSettings()
{
	if(m_pSett)
	{
		if(m_pSett->contains("monteconvo/geo"))
			restoreGeometry(m_pSett->value("monteconvo/geo").toByteArray());

		if(m_pSett->contains("monteconvo/algo"))
			comboAlgo->setCurrentIndex(m_pSett->value("monteconvo/algo").toInt());
		if(m_pSett->contains("monteconvo/fixedk"))
			comboFixedK->setCurrentIndex(m_pSett->value("monteconvo/fixedk").toInt());
		if(m_pSett->contains("monteconvo/mono_foc"))
			comboFocMono->setCurrentIndex(m_pSett->value("monteconvo/mono_foc").toInt());
		if(m_pSett->contains("monteconvo/ana_foc"))
			comboFocAna->setCurrentIndex(m_pSett->value("monteconvo/ana_foc").toInt());
		if(m_pSett->contains("monteconvo/sqw"))
			comboSqw->setCurrentIndex(comboSqw->findData(m_pSett->value("monteconvo/sqw").toString()));

		if(m_pSett->contains("monteconvo/crys"))
			editCrys->setText(m_pSett->value("monteconvo/crys").toString());
		if(m_pSett->contains("monteconvo/instr"))
			editRes->setText(m_pSett->value("monteconvo/instr").toString());
		if(m_pSett->contains("monteconvo/sqw_conf"))
			editSqw->setText(m_pSett->value("monteconvo/sqw_conf").toString());
		if(m_pSett->contains("monteconvo/scanfile"))
			editScan->setText(m_pSett->value("monteconvo/scanfile").toString());

		if(m_pSett->contains("monteconvo/h_from"))
			spinStartH->setValue(m_pSett->value("monteconvo/h_from").toDouble());
		if(m_pSett->contains("monteconvo/k_from"))
			spinStartK->setValue(m_pSett->value("monteconvo/k_from").toDouble());
		if(m_pSett->contains("monteconvo/l_from"))
			spinStartL->setValue(m_pSett->value("monteconvo/l_from").toDouble());
		if(m_pSett->contains("monteconvo/E_from"))
			spinStartE->setValue(m_pSett->value("monteconvo/E_from").toDouble());
		if(m_pSett->contains("monteconvo/h_to"))
			spinStopH->setValue(m_pSett->value("monteconvo/h_to").toDouble());
		if(m_pSett->contains("monteconvo/k_to"))
			spinStopK->setValue(m_pSett->value("monteconvo/k_to").toDouble());
		if(m_pSett->contains("monteconvo/l_to"))
			spinStopL->setValue(m_pSett->value("monteconvo/l_to").toDouble());
		if(m_pSett->contains("monteconvo/E_to"))
			spinStopE->setValue(m_pSett->value("monteconvo/E_to").toDouble());

		if(m_pSett->contains("monteconvo/S_scale"))
			editScale->setText(m_pSett->value("monteconvo/S_scale").toString());
		if(m_pSett->contains("monteconvo/S_offs"))
			editOffs->setText(m_pSett->value("monteconvo/S_offs").toString());

		if(m_pSett->contains("monteconvo/kfix"))
			spinKfix->setValue(m_pSett->value("monteconvo/kfix").toDouble());
		if(m_pSett->contains("monteconvo/neutron_count"))
			spinNeutrons->setValue(m_pSett->value("monteconvo/neutron_count").toInt());
		if(m_pSett->contains("monteconvo/sample_step_count"))
			spinSampleSteps->setValue(m_pSett->value("monteconvo/sample_step_count").toInt());
		if(m_pSett->contains("monteconvo/step_count"))
			spinStepCnt->setValue(m_pSett->value("monteconvo/step_count").toInt());
	}
}

void ConvoDlg::showEvent(QShowEvent *pEvt)
{
	//LoadSettings();
	QDialog::showEvent(pEvt);
}

void ConvoDlg::ButtonBoxClicked(QAbstractButton *pBtn)
{
	if(pBtn == buttonBox->button(QDialogButtonBox::Close))
	{
		if(m_pSett)
		{
			m_pSett->setValue("monteconvo/geo", saveGeometry());

			m_pSett->setValue("monteconvo/algo", comboAlgo->currentIndex());
			m_pSett->setValue("monteconvo/fixedk", comboFixedK->currentIndex());
			m_pSett->setValue("monteconvo/mono_foc", comboFocMono->currentIndex());
			m_pSett->setValue("monteconvo/ana_foc", comboFocAna->currentIndex());
			m_pSett->setValue("monteconvo/sqw", comboSqw->itemData(comboSqw->currentIndex()).toString());

			m_pSett->setValue("monteconvo/crys", editCrys->text());
			m_pSett->setValue("monteconvo/instr", editRes->text());
			m_pSett->setValue("monteconvo/sqw_conf", editSqw->text());
			m_pSett->setValue("monteconvo/scanfile", editScan->text());

			m_pSett->setValue("monteconvo/h_from", spinStartH->value());
			m_pSett->setValue("monteconvo/k_from", spinStartK->value());
			m_pSett->setValue("monteconvo/l_from", spinStartL->value());
			m_pSett->setValue("monteconvo/E_from", spinStartE->value());
			m_pSett->setValue("monteconvo/h_to", spinStopH->value());
			m_pSett->setValue("monteconvo/k_to", spinStopK->value());
			m_pSett->setValue("monteconvo/l_to", spinStopL->value());
			m_pSett->setValue("monteconvo/E_to", spinStopE->value());

			m_pSett->setValue("monteconvo/S_scale", editScale->text());
			m_pSett->setValue("monteconvo/S_offs", editOffs->text());

			m_pSett->setValue("monteconvo/kfix", spinKfix->value());
			m_pSett->setValue("monteconvo/neutron_count", spinNeutrons->value());
			m_pSett->setValue("monteconvo/sample_step_count", spinSampleSteps->value());
			m_pSett->setValue("monteconvo/step_count", spinStepCnt->value());
		}

		QDialog::accept();
	}
}

#include "ConvoDlg.moc"
