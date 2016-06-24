/**
 * Convolution fitting
 * @author tweber
 * @date dec-2015
 * @license GPLv2
 */

#include "tlibs/file/prop.h"
#include "tlibs/file/loaddat.h"
#include "tlibs/log/log.h"
#include "tlibs/log/debug.h"
#include "tlibs/helper/thread.h"
#include "tlibs/gfx/gnuplot.h"
//#include "tlibs/math/neutrons.h"
#include "libs/version.h"

#include <iostream>
#include <fstream>
#include <locale>

#include <boost/asio/io_service.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/scope_exit.hpp>

#include "convofit.h"
#include "scan.h"
#include "model.h"
#include "../monteconvo/sqwfactory.h"
#include "../res/defs.h"

//using t_real = tl::t_real_min;
using t_real = t_real_reso;

namespace asio = boost::asio;
namespace sys = boost::system;


bool run_job(const std::string& strJob)
{
	// Parameters
	tl::Prop<std::string> prop;
	if(!prop.Load(strJob.c_str(), tl::PropType::INFO))
	{
		tl::log_err("Cannot load job file \"", strJob, "\".");
		return 0;
	}

	std::string strScFile = prop.Query<std::string>("input/scan_file");
	if(strScFile == "")	// "scan_file_0" is synonymous to "scan_file"
		strScFile = prop.Query<std::string>("input/scan_file_0");

	std::string strTempCol = prop.Query<std::string>("input/temp_col");
	std::string strFieldCol = prop.Query<std::string>("input/field_col");
	bool bTempOverride = prop.Exists("input/temp_override");
	bool bFieldOverride = prop.Exists("input/field_override");
	t_real dTempOverride = prop.QueryAndParse<t_real>("input/temp_override");
	t_real dFieldOverride = prop.QueryAndParse<t_real>("input/field_override");
	std::string strCntCol = prop.Query<std::string>("input/counts_col");
	std::string strMonCol = prop.Query<std::string>("input/monitor_col");

	std::string strResFile = prop.Query<std::string>("input/instrument_file");
	if(strResFile == "")	// "instrument_file_0" is synonymous to "instrument_file"
		strResFile = prop.Query<std::string>("input/instrument_file_0");

	std::string strSqwMod = prop.Query<std::string>("input/sqw_model");
	std::string strSqwFile = prop.Query<std::string>("input/sqw_file");
	std::string strTempVar = prop.Query<std::string>("input/sqw_temp_var", "T");
	std::string strFieldVar = prop.Query<std::string>("input/sqw_field_var", "");
	std::string strSetParams = prop.Query<std::string>("input/sqw_set_params", "");
	bool bNormToMon = prop.Query<bool>("input/norm_to_monitor", 1);

	Filter filter;
	filter.bLower = prop.Exists("input/filter_lower");
	filter.bUpper = prop.Exists("input/filter_upper");
	if(filter.bLower) filter.dLower = prop.QueryAndParse<t_real>("input/filter_lower", 0);
	if(filter.bUpper) filter.dUpper = prop.QueryAndParse<t_real>("input/filter_upper", 0);


	// --------------------------------------------------------------------
	// files in inner vector will be merged
	// files in outer vector will be used for multi-function fitting
	std::vector<std::vector<std::string>> vecvecScFiles;

	// primary scan files
	{
		std::vector<std::string> vecScFiles;
		tl::get_tokens<std::string, std::string>(strScFile, ";", vecScFiles);
		std::for_each(vecScFiles.begin(), vecScFiles.end(), [](std::string& str){ tl::trim(str); });
		vecvecScFiles.emplace_back(std::move(vecScFiles));
	}

	// get secondary scan files for multi-function fitting
	for(std::size_t iSecFile=1; 1; ++iSecFile)
	{
		std::string strSecFile = "input/scan_file_" + tl::var_to_str(iSecFile);
		std::string strSecScFile = prop.Query<std::string>(strSecFile, "");
		if(strSecScFile == "")
			break;

		std::vector<std::string> vecSecScFiles;
		tl::get_tokens<std::string, std::string>(strSecScFile, ";", vecSecScFiles);
		std::for_each(vecSecScFiles.begin(), vecSecScFiles.end(), [](std::string& str){ tl::trim(str); });
		vecvecScFiles.emplace_back(std::move(vecSecScFiles));
	}
	// --------------------------------------------------------------------


	// --------------------------------------------------------------------
	//primary resolution file
	std::vector<std::string> vecResFiles({strResFile});

	// get secondary resolution files for multi-function fitting
	for(std::size_t iSecFile=1; 1; ++iSecFile)
	{
		std::string strSecFile = "input/instrument_file_" + tl::var_to_str(iSecFile);
		std::string strSecResFile = prop.Query<std::string>(strSecFile, "");
		if(strSecResFile == "")
			break;
		tl::trim(strSecResFile);
		vecResFiles.emplace_back(std::move(strSecResFile));
	}

	if(vecResFiles.size()!=1 && vecResFiles.size()!=vecvecScFiles.size())
	{
		tl::log_err("Number of resolution files has to be either one or match the number of scan files.");
		tl::log_err("Number of scan files: ", vecvecScFiles.size(), ", number of resolution files: ", vecResFiles.size(), ".");
		return 0;
	}
	// --------------------------------------------------------------------


	unsigned iNumNeutrons = prop.Query<unsigned>("montecarlo/neutrons", 1000);
	unsigned iNumSample = prop.Query<unsigned>("montecarlo/sample_positions", 1);

	std::string strResAlgo = prop.Query<std::string>("resolution/algorithm", "pop");
	bool bUseR0 = prop.Query<bool>("resolution/use_r0", 0);
	bool bResFocMonoV = prop.Query<bool>("resolution/focus_mono_v", 0);
	bool bResFocMonoH = prop.Query<bool>("resolution/focus_mono_h", 0);
	bool bResFocAnaV = prop.Query<bool>("resolution/focus_ana_v", 0);
	bool bResFocAnaH = prop.Query<bool>("resolution/focus_ana_h", 0);

	std::string strMinimiser = prop.Query<std::string>("fitter/minimiser");
	int iStrat = prop.Query<int>("fitter/strategy", 0);
	t_real dSigma = prop.Query<t_real>("fitter/sigma", 1.);

	bool bDoFit = prop.Query<bool>("fitter/do_fit", 1);
	unsigned int iMaxFuncCalls = prop.Query<unsigned>("fitter/max_funccalls", 0);
	t_real dTolerance = prop.Query<t_real>("fitter/tolerance", 0.5);

	std::string strScOutFile = prop.Query<std::string>("output/scan_file");
	std::string strModOutFile = prop.Query<std::string>("output/model_file");
	std::string strLogOutFile = prop.Query<std::string>("output/log_file");
	bool bPlot = prop.Query<bool>("output/plot", 0);
	bool bPlotIntermediate = prop.Query<bool>("output/plot_intermediate", 0);
	unsigned int iPlotPoints = prop.Query<unsigned>("output/plot_points", 128);

	std::unique_ptr<tl::GnuPlot<t_real>> plt;
	if(bPlot || bPlotIntermediate)
	{
		plt.reset(new tl::GnuPlot<t_real>());
		plt->Init();
	}

	// thread-local debug log
	std::unique_ptr<std::ostream> ofstrLog;
	if(strLogOutFile != "")
	{
		ofstrLog.reset(new std::ofstream(strLogOutFile));

		for(tl::Log* plog : { &tl::log_info, &tl::log_warn, &tl::log_err, &tl::log_crit, &tl::log_debug })
			plog->AddOstr(ofstrLog.get(), 0, 1);
	}

	if(strScOutFile=="" || strModOutFile=="")
	{
		tl::log_err("Not output files selected.");
		return 0;
	}


	std::string strFitParams = prop.Query<std::string>("fit_parameters/params");
	std::string strFitValues = prop.Query<std::string>("fit_parameters/values");
	std::string strFitErrors = prop.Query<std::string>("fit_parameters/errors");
	std::string strFitFixed = prop.Query<std::string>("fit_parameters/fixed");

	std::vector<std::string> vecFitParams;
	tl::get_tokens<std::string, std::string>(strFitParams, " \t\n,;", vecFitParams);
	std::vector<t_real> vecFitValues;
	tl::parse_tokens<t_real, std::string>(strFitValues, " \t\n,;", vecFitValues);
	std::vector<t_real> vecFitErrors;
	tl::parse_tokens<t_real, std::string>(strFitErrors, " \t\n,;", vecFitErrors);
	std::vector<bool> vecFitFixed;
	tl::get_tokens<bool, std::string>(strFitFixed, " \t\n,;", vecFitFixed);

	if(vecFitParams.size() != vecFitValues.size() || 
		vecFitParams.size() != vecFitErrors.size() || 
		vecFitParams.size() != vecFitFixed.size())
	{
		tl::log_err("Fit parameter size mismatch.");
		return 0;
	}




	// --------------------------------------------------------------------
	// Scan files
	std::vector<Scan> vecSc;
	for(std::size_t iSc=0; iSc<vecvecScFiles.size(); ++iSc)
	{
		Scan sc;
		if(strTempCol != "")
			sc.strTempCol = strTempCol;
		if(strFieldCol != "")
			sc.strFieldCol = strFieldCol;
		sc.strCntCol = strCntCol;
		sc.strMonCol = strMonCol;

		if(vecvecScFiles.size() > 1)
			tl::log_info("Loading scan group ", iSc, ".");
		if(!load_file(vecvecScFiles[iSc], sc, bNormToMon, filter))
		{
			tl::log_err("Cannot load scan files of group ", iSc, ".");
			continue;
		}

		vecSc.emplace_back(std::move(sc));
	}
	if(!vecSc.size())
	{
		tl::log_err("No scans loaded.");
		return 0;
	}

	tl::log_info("Number of scan groups: ", vecSc.size(), ".");

	// scan plot object
	tl::PlotObj<t_real> pltMeas;
	if(bPlot || bPlotIntermediate)
	{
		pltMeas.vecX = vecSc[0].vecX;
		pltMeas.vecY = vecSc[0].vecCts;
		pltMeas.vecErrY = vecSc[0].vecCtsErr;
		pltMeas.linestyle = tl::STYLE_POINTS;
	}
	// --------------------------------------------------------------------




	// --------------------------------------------------------------------
	// Resolution files
	std::vector<TASReso> vecResos;
	for(const std::string& strCurResFile : vecResFiles)
	{
		TASReso reso;
		tl::log_info("Loading instrument resolution file \"", strCurResFile, "\".");
		if(!reso.LoadRes(strCurResFile.c_str()))
			return 0;

		if(strResAlgo == "pop")
			reso.SetAlgo(ResoAlgo::POP);
		else if(strResAlgo == "cn")
			reso.SetAlgo(ResoAlgo::CN);
		else if(strResAlgo == "eck")
			reso.SetAlgo(ResoAlgo::ECK);
		else if(strResAlgo == "viol")
			reso.SetAlgo(ResoAlgo::VIOL);
		else
		{
			tl::log_err("Invalid resolution algorithm selected: \"", strResAlgo, "\".");
			return 0;
		}

		if(bResFocMonoV || bResFocMonoH || bResFocAnaV || bResFocAnaH)
		{
			unsigned iFoc = 0;
			if(bResFocMonoV) iFoc |= unsigned(ResoFocus::FOC_MONO_V);
			if(bResFocMonoH) iFoc |= unsigned(ResoFocus::FOC_MONO_H);
			if(bResFocAnaV) iFoc |= unsigned(ResoFocus::FOC_ANA_V);
			if(bResFocAnaH) iFoc |= unsigned(ResoFocus::FOC_ANA_H);

			reso.SetOptimalFocus(ResoFocus(iFoc));
		}

		if(bUseR0 && !reso.GetResoParams().bCalcR0)
			tl::log_warn("Resolution R0 requested, but not calculated, using raw ellipsoid volume.");

		reso.SetRandomSamplePos(iNumSample);
		vecResos.emplace_back(std::move(reso));
	}

	// base parameter set for single-fits
	set_tasreso_params_from_scan(vecResos[0], vecSc[0]);
	// --------------------------------------------------------------------




	// --------------------------------------------------------------------
	// Model file
	tl::log_info("Loading S(q,w) file \"", strSqwFile, "\".");
	std::shared_ptr<SqwBase> pSqw = construct_sqw(strSqwMod, strSqwFile);

	if(!pSqw)
	{
		tl::log_err("Invalid S(q,w) model selected: \"", strSqwMod, "\".");
		return 0;
	}

	if(!pSqw->IsOk())
	{
		tl::log_err("S(q,w) model cannot be initialised.");
		return 0;
	}
	SqwFuncModel mod(pSqw, vecResos);


	std::vector<t_real> vecModTmpX, vecModTmpY;
	// slots
	mod.AddFuncResultSlot(
	[&plt, &pltMeas, &vecModTmpX, &vecModTmpY, bPlotIntermediate](t_real h, t_real k, t_real l, t_real E, t_real S)
	{
		tl::log_info("Q = (", h, ", ", k, ", ", l, ") rlu, E = ", E, " meV -> S = ", S);

		if(bPlotIntermediate)
		{
			vecModTmpX.push_back(E);	// TODO: use scan direction
			vecModTmpY.push_back(S);

			tl::PlotObj<t_real> pltMod;
			pltMod.vecX = vecModTmpX;
			pltMod.vecY = vecModTmpY;
			pltMod.linestyle = tl::STYLE_LINES_SOLID;
			pltMod.odSize = 1.5;

			plt->StartPlot();
			plt->SetYLabel("Intensity");
			plt->AddLine(pltMod);
			plt->AddLine(pltMeas);
			plt->FinishPlot();
		}
	});
	mod.AddParamsChangedSlot(
	[&vecModTmpX, &vecModTmpY, bPlotIntermediate](const std::string& strDescr)
	{
		tl::log_info("Changed model parameters: ", strDescr);

		if(bPlotIntermediate)
		{
			vecModTmpX.clear();
			vecModTmpY.clear();
		}
	});


	// only needed for multi-fits
	if(vecSc.size() > 1)
		mod.SetScans(&vecSc);

	mod.SetNumNeutrons(iNumNeutrons);
	mod.SetUseR0(bUseR0);

	if(bTempOverride)
	{
		for(Scan& sc : vecSc)
		{
			sc.dTemp = dTempOverride;
			sc.dTempErr = 0.;
		}
	}
	if(bFieldOverride)
	{
		for(Scan& sc : vecSc)
		{
			sc.dField = dFieldOverride;
			sc.dFieldErr = 0.;
		}
	}
	mod.SetOtherParamNames(strTempVar, strFieldVar);

	// base parameter set for single-fits
	set_model_params_from_scan(mod, vecSc[0]);

	tl::log_info("Model temperature variable: \"", strTempVar, "\", value: ", vecSc[0].dTemp);
	tl::log_info("Model field variable: \"", strFieldVar, "\", value: ", vecSc[0].dField);


	// set given individual model parameters
	if(strSetParams != "")
	{
		std::vector<std::string> vecSetParams;
		tl::get_tokens<std::string, std::string>(strSetParams, ";", vecSetParams);
		for(const std::string& strModParam : vecSetParams)
		{
			std::vector<std::string> vecModParam;
			tl::get_tokens<std::string, std::string>(strModParam, "=", vecModParam);
			if(vecModParam.size() < 2)
				continue;
			tl::trim(vecModParam[0]);
			tl::trim(vecModParam[1]);

			if(mod.GetSqwBase()->SetVarIfAvail(vecModParam[0], vecModParam[1]))
				tl::log_info("Setting model parameter \"", vecModParam[0], "\" to \"", vecModParam[1], "\".");
			else
				tl::log_err("No parameter named \"", vecModParam[0], "\" available in S(q,w) model.");
		}
	}
	// --------------------------------------------------------------------




	// --------------------------------------------------------------------
	// Fitting
	for(std::size_t iParam=0; iParam<vecFitParams.size(); ++iParam)
	{
		const std::string& strParam = vecFitParams[iParam];
		t_real dVal = vecFitValues[iParam];
		t_real dErr = vecFitErrors[iParam];

		// not a S(q,w) model parameter
		if(strParam=="scale" || strParam=="offs")
			continue;

		mod.AddModelFitParams(strParam, dVal, dErr);
	}

	//tl::Chi2Function<t_real_sc> chi2fkt(&mod, vecSc[0].vecX.size(), vecSc[0].vecX.data(), vecSc[0].vecCts.data(), vecSc[0].vecCtsErr.data());
	tl::Chi2Function_mult<t_real_sc, std::vector> chi2fkt;
	// the vecSc[0] data sets are the default data set (will not be used if scan groups are defined)
	chi2fkt.AddFunc(&mod, vecSc[0].vecX.size(), vecSc[0].vecX.data(), vecSc[0].vecCts.data(), vecSc[0].vecCtsErr.data());
	chi2fkt.SetDebug(1);
	chi2fkt.SetSigma(dSigma);


	minuit::MnUserParameters params = mod.GetMinuitParams();
	for(std::size_t iParam=0; iParam<vecFitParams.size(); ++iParam)
	{
		const std::string& strParam = vecFitParams[iParam];
		t_real dVal = vecFitValues[iParam];
		t_real dErr = vecFitErrors[iParam];
		bool bFix = vecFitFixed[iParam];

		params.SetValue(strParam, dVal);
		params.SetError(strParam, dErr);
		if(bFix) params.Fix(strParam);
	}
	// set initials
	mod.SetMinuitParams(params);


	minuit::MnStrategy strat(iStrat);
	/*strat.SetGradientStepTolerance(1.);
	strat.SetGradientTolerance(0.2);
	strat.SetHessianStepTolerance(1.);
	strat.SetHessianG2Tolerance(0.2);*/

	std::unique_ptr<minuit::MnApplication> pmini;
	if(strMinimiser == "simplex")
		pmini.reset(new minuit::MnSimplex(chi2fkt, params, strat));
	else if(strMinimiser == "migrad")
		pmini.reset(new minuit::MnMigrad(chi2fkt, params, strat));
	else
	{
		tl::log_err("Invalid minimiser selected: \"", strMinimiser, "\".");
		return 0;
	}

	bool bValidFit = 0;
	if(bDoFit)
	{
		tl::log_info("Performing fit.");
		minuit::FunctionMinimum mini = (*pmini)(iMaxFuncCalls, dTolerance);
		const minuit::MnUserParameterState& state = mini.UserState();
		bValidFit = mini.IsValid() && mini.HasValidParameters() && state.IsValid();
		mod.SetMinuitParams(state);

		std::ostringstream ostrMini;
		ostrMini << mini << "\n";
		tl::log_info(ostrMini.str(), "Fit valid: ", bValidFit);
	}
	else
	{
		tl::log_info("Skipping fit, keeping initial values.");
	}


	tl::log_info("Saving results.");

	for(std::size_t iSc=0; iSc<vecSc.size(); ++iSc)
	{
		const Scan& sc = vecSc[iSc];

		std::string strCurModOutFile = strModOutFile;
		std::string strCurScOutFile = strScOutFile;

		// append scan group number if this is a multi-fit
		if(vecSc.size() > 1)
		{
			strCurModOutFile += tl::var_to_str(iSc);
			strCurScOutFile += tl::var_to_str(iSc);
		}
		std::pair<decltype(sc.vecX)::const_iterator, decltype(sc.vecX)::const_iterator> xminmax
			= std::minmax_element(sc.vecX.begin(), sc.vecX.end());
		mod.Save(strCurModOutFile.c_str(), *xminmax.first, *xminmax.second, iPlotPoints);
		save_file(strCurScOutFile.c_str(), sc);
	}
	// --------------------------------------------------------------------




	// --------------------------------------------------------------------
	// Plotting
	if(bPlot)
	{
		/*std::ostringstream ostr;
		ostr << "gnuplot -p -e \"plot \\\"" 
			<< strModOutFile.c_str() << "\\\" using 1:2 w lines lw 1.5 lt 1, \\\""
			<< strScOutFile.c_str() << "\\\" using 1:2:3 w yerrorbars ps 1 pt 7\"\n";
		std::system(ostr.str().c_str());*/

		tl::DatFile<t_real, char> datMod;
		datMod.Load(strModOutFile);

		tl::PlotObj<t_real> pltMod;
		pltMod.vecX = datMod.GetColumn(0);
		pltMod.vecY = datMod.GetColumn(1);
		pltMod.linestyle = tl::STYLE_LINES_SOLID;
		pltMod.odSize = 1.5;

		plt->StartPlot();
		plt->SetYLabel("Intensity");
		plt->AddLine(pltMod);
		plt->AddLine(pltMeas);
		plt->FinishPlot();
	}
	// --------------------------------------------------------------------




	// remove thread-local loggers
	if(!!ofstrLog)
	{
		for(tl::Log* plog : { &tl::log_info, &tl::log_warn, &tl::log_err, &tl::log_crit, &tl::log_debug })
			plog->RemoveOstr(ofstrLog.get());
	}

	return bValidFit;
}




int main(int argc, char** argv)
{
	// plain C locale
	std::setlocale(LC_ALL, "C");
	std::locale::global(std::locale::classic());

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
		//tl::log_debug("Exiting...");
		ioSrv.stop();
		thSig.join();
	}
	BOOST_SCOPE_EXIT_END


	tl::log_info("This is the Takin command-line convolution fitter, version " TAKIN_VER ".");
	tl::log_info("Written by Tobias Weber <tobias.weber@tum.de>, 2014-2016.");
	tl::log_debug("Resolution calculation uses ", sizeof(t_real_reso)*8, " bit ", tl::get_typename<t_real_reso>(), "s.");
	tl::log_debug("Fitting uses ", sizeof(tl::t_real_min)*8, " bit ", tl::get_typename<tl::t_real_min>(), "s.");

	if(argc > 2)
	{
		for(tl::Log* log : { &tl::log_info, &tl::log_warn, &tl::log_err, &tl::log_crit, &tl::log_debug })
			log->SetShowThread(1);
	}

	if(argc <= 1)
	{
		tl::log_info("Usage:");
		tl::log_info("\t", argv[0], " <job file 1> <job file 2> ...");
		return -1;
	}


	unsigned int iNumThreads = std::thread::hardware_concurrency();
	tl::ThreadPool<bool()> tp(iNumThreads);

	for(int iArg=1; iArg<argc; ++iArg)
	{
		std::string strJob = argv[iArg];
		tp.AddTask([iArg, strJob]() -> bool
		{
			tl::log_info("Executing job file ", iArg, ": \"", strJob, "\".");

			return run_job(strJob);
			//if(argc > 2) tl::log_info("================================================================================");
		});
	}

	tp.StartTasks();

	auto& lstFut = tp.GetFutures();
	unsigned int iTask = 1;
	for(auto& fut : lstFut)
	{
		bool bOk = fut.get();
		if(!bOk)
			tl::log_err("Job ", iTask, " (", argv[iTask], ") failed or fit invalid!");
		++iTask;
	}

	return 0;
}
