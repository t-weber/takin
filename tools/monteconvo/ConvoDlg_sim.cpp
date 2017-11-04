/**
 * monte carlo convolution tool -> convolution simulations
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 2015, 2016
 * @license GPLv2
 */

#include "ConvoDlg.h"
#include "tlibs/time/stopwatch.h"
#include "tlibs/helper/thread.h"
#include "tlibs/math/stat.h"


using t_real = t_real_reso;
using t_stopwatch = tl::Stopwatch<t_real>;

static constexpr const t_real g_dEpsRlu = 1e-3;


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
	btnStartFit->setEnabled(false);
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
			QMetaObject::invokeMethod(btnStartFit, "setEnabled", Q_ARG(bool, true));
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

		unsigned int iNumThreads = bForceDeferred ? 0 : get_max_threads();

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
						localreso.GenerateMC_deferred(iNumNeutrons, vecNeutrons);

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
				set_qwt_data<t_real>()(*m_plotwrap, m_vecQ, m_vecScaledS, 0, false);
				set_qwt_data<t_real>()(*m_plotwrap, m_vecQ, m_vecScaledS, 1, false);
				if(bUseScan)
					set_qwt_data<t_real>()(*m_plotwrap, m_scan.vecX, m_scan.vecCts, 2, false, &m_scan.vecCtsErr);
				else
					set_qwt_data<t_real>()(*m_plotwrap, vecNull, vecNull, 2, false);

				if(bIsLastStep)
				{
					t_real_qwt dLeft = std::numeric_limits<t_real_qwt>::max();
					t_real_qwt dRight = -dLeft;
					t_real_qwt dTop = dRight;
					t_real_qwt dBottom = dLeft;

					auto minmaxQ = std::minmax_element(m_vecQ.begin(), m_vecQ.end());
					auto minmaxS = std::minmax_element(m_vecScaledS.begin(), m_vecScaledS.end());

					if(minmaxQ.first != m_vecQ.end())
					{
						dLeft = *minmaxQ.first;
						dRight = *minmaxQ.second;
					}
					if(minmaxS.first != m_vecScaledS.end())
					{
						dBottom = *minmaxS.first;
						dTop = *minmaxS.second;
					}

					if(bUseScan && m_scan.vecX.size() && m_scan.vecCts.size())
					{
						auto minmaxX = std::minmax_element(m_scan.vecX.begin(), m_scan.vecX.end());
						auto minmaxY = std::minmax_element(m_scan.vecCts.begin(), m_scan.vecCts.end());

						dLeft = std::min<t_real_qwt>(dLeft, *minmaxX.first);
						dRight = std::max<t_real_qwt>(dRight, *minmaxX.second);
						dBottom = std::min<t_real_qwt>(dBottom, *minmaxY.first);
						dTop = std::max<t_real_qwt>(dTop, *minmaxY.second);
					}

					set_zoomer_base(m_plotwrap->GetZoomer(),
						dLeft, dRight, dTop, dBottom,
						!bForceDeferred, m_plotwrap.get());
				}
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


		// approximate chi^2
		if(bUseScan && m_pSqw)
		{
			const std::size_t iNumScanPts = m_scan.vecPoints.size();
			std::vector<t_real> vecSFuncY;
			vecSFuncY.reserve(iNumScanPts);

			for(std::size_t iScanPt=0; iScanPt<iNumScanPts; ++iScanPt)
			{
				const ScanPoint& pt = m_scan.vecPoints[iScanPt];
				t_real E = pt.E / tl::one_meV;
				ublas::vector<t_real> vecScanHKLE = tl::make_vec({ pt.h, pt.k, pt.l, E });


				// find point on S(q,w) curve closest to scan point
				std::size_t iMinIdx = 0;
				t_real dMinDist = std::numeric_limits<t_real>::max();
				for(std::size_t iStep=0; iStep<iNumSteps; ++iStep)
				{
					ublas::vector<t_real> vecCurveHKLE =
						tl::make_vec({ vecH[iStep], vecK[iStep], vecL[iStep], vecE[iStep] });

					t_real dDist = ublas::norm_2(vecCurveHKLE - vecScanHKLE);
					if(dDist < dMinDist)
					{
						dMinDist = dDist;
						iMinIdx = iStep;
					}
				}

				// add the scaled S value from the closest point
				vecSFuncY.push_back(m_vecScaledS[iMinIdx]);
			}

			t_real tChi2 = tl::chi2_direct<t_real>(iNumScanPts,
				vecSFuncY.data(), m_scan.vecCts.data(), m_scan.vecCtsErr.data());
			tl::log_info("chi^2 = ", tChi2);
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
	btnStartFit->setEnabled(false);
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
			QMetaObject::invokeMethod(btnStartFit, "setEnabled", Q_ARG(bool, true));
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
		t_real dStart1{}, dStop1{};
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
		t_real dStart2{}, dStop2{};
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

		unsigned int iNumThreads = bForceDeferred ? 0 : get_max_threads();

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
						localreso.GenerateMC_deferred(iNumNeutrons, vecNeutrons);

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
	btnStartFit->setEnabled(false);
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
			QMetaObject::invokeMethod(btnStartFit, "setEnabled", Q_ARG(bool, true));
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

		unsigned int iNumThreads = bForceDeferred ? 0 : get_max_threads();

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
				for(std::size_t iBranch=0; iBranch<m_vecvecE.size() && iBranch+CONVO_DISP_CURVE_START<CONVO_MAX_CURVES; ++iBranch)
				{
					set_qwt_data<t_real>()(*m_plotwrap, m_vecvecQ[iBranch], m_vecvecE[iBranch], CONVO_DISP_CURVE_START+iBranch, false);
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
