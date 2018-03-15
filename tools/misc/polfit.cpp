/**
 * Fitting of polarisation data
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date mar-18
 * @license GPLv2
 *
 * gcc -I../.. -o polfit polfit.cpp ../../tlibs/file/loadinstr.cpp ../../tlibs/string/eval.cpp ../../tlibs/log/log.cpp -std=c++11 -lstdc++ -lm -lboost_iostreams -lboost_system -lMinuit2 -lgomp
 */

#include "tlibs/file/loadinstr.h"
#include "tlibs/fit/minuit.h"

#include <tuple>
#include <vector>

using t_real = tl::t_real_min;
using t_vec = std::vector<t_real>;

#define NUM_POINTS 512


template<std::size_t iFuncArgs, class t_func>
std::tuple<bool, t_vec, t_vec>
fit(const t_vec& vecX, const t_vec& vecY, const t_vec& vecYErr,
	t_func&& func,
	const std::vector<std::string>& vecParamNames,
	t_vec& vecVals, t_vec& vecErrs,
	const std::vector<bool>& vecFixed)
{
	t_vec vecFitX, vecFitY;

	if(std::min(vecX.size(), vecY.size()) == 0)
		return std::make_tuple(false, vecFitX, vecFitY);

	bool bOk = 0;
	try
	{
		bOk = tl::fit<iFuncArgs>(func, vecX, vecY, vecYErr,
			vecParamNames, vecVals, vecErrs, &vecFixed);
	}
	catch(const std::exception& ex)
	{
		tl::log_err(ex.what());
	}

	if(!bOk)
		return std::make_tuple(false, vecFitX, vecFitY);

	auto minmaxX = std::minmax_element(vecX.begin(), vecX.end());

	vecFitX.reserve(NUM_POINTS);
	vecFitY.reserve(NUM_POINTS);

	t_vec vecValsWithX = { 0. };
	for(t_real dVal : vecVals) vecValsWithX.push_back(dVal);

	for(std::size_t i=0; i<NUM_POINTS; ++i)
	{
		t_real dX = t_real(i)*(*minmaxX.second - *minmaxX.first)/t_real(NUM_POINTS-1) + *minmaxX.first;
		vecValsWithX[0] = dX;
		t_real dY = tl::call<iFuncArgs, decltype(func), t_real, std::vector>(func, vecValsWithX);

		vecFitX.push_back(dX);
		vecFitY.push_back(dY);
	}

	return std::make_tuple(true, vecFitX, vecFitY);
}


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


std::tuple<bool, std::vector<std::string>, t_vec, t_vec, t_vec, t_vec>
fit_sine(const t_vec& vecX, const t_vec& vecY, const t_vec& vecYErr,
	const std::vector<std::string>& vecHints)
{
	if(std::min(vecX.size(), vecY.size()) == 0)
		return std::make_tuple(false, std::vector<std::string>(), t_vec(), t_vec(), t_vec(), t_vec());

	auto func = [](t_real x, t_real amp, t_real freq, t_real phase, t_real offs) -> t_real
	{ return amp*std::sin(freq*x + phase) + offs; };
	constexpr std::size_t iFuncArgs = 5;

	t_real dAmp, dFreq, dPhase, dOffs;
	t_real dAmpErr, dFreqErr, dPhaseErr, dOffsErr;
	bool bAmpFixed, bFreqFixed, bPhaseFixed, bOffsFixed;

	if(vecHints.size() == 0)
	{
		auto minmaxX = std::minmax_element(vecX.begin(), vecX.end());
		auto minmaxY = std::minmax_element(vecY.begin(), vecY.end());

		dFreq = t_real(2.*M_PI) / (*minmaxX.second - *minmaxX.first);
		dOffs = tl::mean_value(vecY);
		dAmp = (std::abs(*minmaxY.second - dOffs) + std::abs(dOffs - *minmaxY.first)) * 0.5;
		dPhase = 0.;
		std::cout << dAmp << " " << dOffs << std::endl;

		dFreqErr = dFreq * 0.2;
		dOffsErr = tl::std_dev(vecY);;
		dAmpErr = dAmp * 0.1;
		dPhaseErr = M_PI;

		bAmpFixed = bFreqFixed = bPhaseFixed = bOffsFixed = 0;
	}
	else
	{
		dAmp = tl::str_to_var_parse<t_real>(vecHints[0]);
		dFreq = tl::str_to_var_parse<t_real>(vecHints[1]);
		dPhase = tl::str_to_var_parse<t_real>(vecHints[2]);
		dOffs = tl::str_to_var_parse<t_real>(vecHints[3]);

		dAmpErr = tl::str_to_var_parse<t_real>(vecHints[4]);
		dFreqErr = tl::str_to_var_parse<t_real>(vecHints[5]);
		dPhaseErr = tl::str_to_var_parse<t_real>(vecHints[6]);
		dOffsErr = tl::str_to_var_parse<t_real>(vecHints[7]);

		bAmpFixed = (tl::str_to_var_parse<int>(vecHints[8])!=0);
		bFreqFixed = (tl::str_to_var_parse<int>(vecHints[9])!=0);
		bPhaseFixed = (tl::str_to_var_parse<int>(vecHints[10])!=0);
		bOffsFixed = (tl::str_to_var_parse<int>(vecHints[11])!=0);
	}

	std::vector<std::string> vecParamNames = { "amp", "freq", "phase",  "offs" };
	t_vec vecVals = { dAmp, dFreq, dPhase, dOffs };
	t_vec vecErrs = { dAmpErr, dFreqErr, dPhaseErr, dOffsErr };
	std::vector<bool> vecFixed = { bAmpFixed, bFreqFixed, bPhaseFixed, bOffsFixed };

	t_vec vecFitX, vecFitY;
	bool bOk = 0;
	std::tie(bOk, vecFitX, vecFitY)
		= fit<iFuncArgs>(vecX, vecY, vecYErr, func, vecParamNames, vecVals, vecErrs, vecFixed);
	if(!bOk)
		return std::make_tuple(false, std::vector<std::string>(), t_vec(), t_vec(), t_vec(), t_vec());

	for(t_real &d : vecErrs)
		d = std::abs(d);

	sanitise_sine_params(vecVals[0], vecVals[1], vecVals[2], vecVals[3]);

	return std::make_tuple(bOk, vecParamNames, vecVals, vecErrs, vecFitX, vecFitY);
}



std::tuple<t_vec, t_vec, t_vec,  t_vec, t_vec, t_vec, std::string>
load_split_file(const char* pcFile)
{
	auto pInstr = tl::FileInstrBase<t_real>::LoadInstr(pcFile);
	auto vecScanVars = pInstr->GetScannedVars();
	auto strScanVar = vecScanVars[0];
	auto strCntVar = pInstr->GetCountVar();
	auto vecFullX = pInstr->GetCol(strScanVar.c_str());
	auto vecFullY = pInstr->GetCol(strCntVar.c_str());

	t_vec vecX1, vecY1, vecYErr1;
	t_vec vecX2, vecY2, vecYErr2;

	for(std::size_t i=0; i<vecFullX.size(); i+=2)
	{
		vecX1.push_back(vecFullX[i]);
		vecY1.push_back(vecFullY[i]);
		t_real dY1Err = std::sqrt(vecFullY[i]);
		if(tl::float_equal<t_real>(dY1Err, 0.))
			dY1Err = 1.;
		vecYErr1.push_back(dY1Err);

		vecX2.push_back(vecFullX[i+1]);
		vecY2.push_back(vecFullY[i+1]);
		t_real dY2Err = std::sqrt(vecFullY[i+1]);
		if(tl::float_equal<t_real>(dY2Err, 0.))
			dY2Err = 1.;
		vecYErr2.push_back(dY2Err);
	}


	std::string strUserVar;
	const tl::FileInstrBase<t_real>::t_mapParams& params = pInstr->GetAllParams();
	for(const tl::FileInstrBase<t_real>::t_mapParams::value_type& pair : params)
	{
		if(pair.first == "user_vars")
		{
			strUserVar = pair.second;
			break;
		}
	}

	delete pInstr;

	return std::make_tuple(vecX1, vecY1, vecYErr1,  vecX2, vecY2, vecYErr2, strUserVar);
}


std::tuple<t_vec, t_vec, t_vec>
mk_pol(const t_vec& vecX1, const t_vec& vecY1, const t_vec& vecYErr1,
	const t_vec& vecX2, const t_vec& vecY2, const t_vec& vecYErr2)
{
	t_vec vecPolX, vecPolY, vecPolYErr;

	std::size_t iLen = std::min(vecX1.size(), vecX2.size());
	for(std::size_t i=0; i<iLen; ++i)
	{
		vecPolX.push_back(vecX1[i]);
		vecPolY.push_back((vecY1[i]-vecY2[i]) / (vecY1[i]+vecY2[i]));

		// d((x-y)/(x+y)) = dx * 2*y/(x+y)^2 - dy * 2*x/(x+y)^2
		t_real x = vecY1[i];
		t_real y = vecY2[i];
		t_real dx = vecYErr1[i];
		t_real dy = vecYErr2[i];
		vecPolYErr.push_back(std::sqrt(dx*2.*y/((x+y)*(x+y))*dx*2.*y/((x+y)*(x+y))
			+ dy*2.*x/((x+y)*(x+y))*dy*2.*x/((x+y)*(x+y))));
	}

	return std::make_tuple(vecPolX, vecPolY, vecPolYErr);
}


void save_data(const char* pcFile, const t_vec& vecPolX, const t_vec& vecPolY, const t_vec* vecPolYErr=0)
{
	std::ofstream ofstr(pcFile);
	//ofstr << "# polarisation\n";

	for(std::size_t i=0; i<vecPolX.size(); ++i)
	{
		ofstr << std::setw(10) << vecPolX[i] << " "
			<< std::setw(10) << vecPolY[i] << " ";
		if(vecPolYErr)
			ofstr << std::setw(10) << (*vecPolYErr)[i];
		ofstr << "\n";
	}
}


int main(int argc, char** argv)
{
	if(argc<2)
	{
		std::cerr << "Specify job file." << std::endl;
		return -1;
	}

	std::ifstream ifstrJob(argv[1]);
	std::string strLine;

	std::ofstream ofstrOut("out.dat");
	bool bParamNamesWritten = 0;

	while(ifstrJob)
	{
		std::getline(ifstrJob, strLine);
		tl::trim(strLine);
		if(strLine == "")
			continue;

		std::vector<std::string> vecStr;
		tl::get_tokens<std::string, std::string>(strLine, ",;", vecStr);
		if(vecStr.size() == 0)
			continue;

		std::string strFile = vecStr[0];
		vecStr.erase(vecStr.begin());
		std::string strFileNoDir = tl::get_file_nodir(strFile);

		tl::log_info("Loading file ", strFile, ".");

		t_vec vecX1, vecY1, vecYErr1;
		t_vec vecX2, vecY2, vecYErr2;

		std::string strUserVar;

		std::tie(vecX1, vecY1, vecYErr1,  vecX2, vecY2, vecYErr2, strUserVar)
			= load_split_file(strFile.c_str());

		std::vector<std::string> vecUserVars;
		tl::get_tokens<std::string, std::string>(strUserVar, ",;=", vecUserVars);
		strUserVar = "";
		for(std::size_t iUserVar=0; iUserVar<vecUserVars.size(); ++iUserVar)
		{
			tl::trim(vecUserVars[iUserVar]);
			if(vecUserVars[iUserVar] == "user_var")
			{
				strUserVar = vecUserVars[iUserVar+1];
				break;
			}
		}


		t_vec vecPolX, vecPolY, vecPolYErr;

		std::tie(vecPolX, vecPolY, vecPolYErr) =
			mk_pol(vecX1, vecY1, vecYErr1,  vecX2, vecY2, vecYErr2);

		save_data((strFileNoDir + ".pol").c_str(), vecPolX, vecPolY, &vecPolYErr);


		bool bOk = false;
		std::vector<std::string> vecParamNames;
		t_vec vecVals, vecErrs, vecFitX, vecFitY;

		std::tie(bOk, vecParamNames, vecVals, vecErrs, vecFitX, vecFitY)
			= fit_sine(vecPolX, vecPolY, vecPolYErr, vecStr);

		if(!bOk)
			tl::log_err("Fit failed!");

		save_data((strFileNoDir + ".fit").c_str(), vecFitX, vecFitY);


		if(!bParamNamesWritten)
		{
			ofstrOut << "# params: OK, ";
			if(strUserVar != "")
				ofstrOut << "user_var, ";

			for(std::size_t iParam=0; iParam<vecParamNames.size(); ++iParam)
				ofstrOut << vecParamNames[iParam] << ", ";
			ofstrOut << "\n";
			bParamNamesWritten = 1;
		}

		ofstrOut /*<< std::boolalpha*/ << std::left << std::setw(4) << bOk << " ";
		if(strUserVar != "")
			ofstrOut << std::left << std::setw(4) << strUserVar << " ";
		for(std::size_t iParam=0; iParam<vecVals.size(); ++iParam)
		{
			ofstrOut << std::left << std::setw(10) << vecVals[iParam] << " "
				<< std::left << std::setw(10) << vecErrs[iParam] << "    ";
		}
		ofstrOut << "\n";
	}

	return 0;
}
