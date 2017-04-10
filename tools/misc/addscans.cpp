/**
 * add scan files
 * @author Tobias Weber <tobias.weber@tum.de>
 * @license GPLv2
 * @date 2014
 */
// gcc -I../.. -I. -DNO_IOSTR -o addscans ../../tlibs/file/loadinstr.cpp ../../tlibs/log/log.cpp addscans.cpp -std=c++11 -lstdc++ -lm -lboost_system -lboost_filesystem
// e.g. ./addscans /home/tweber/Auswertungen/MnSi-Mira-15/data3/11009_00016851.dat /home/tweber/Auswertungen/MnSi-Mira-15/data3/11009_00016867.dat merged.dat

#include <iostream>
#include <fstream>
#include <iomanip>
#include "tlibs/file/loadinstr.h"
#include "tlibs/log/log.h"
#include "libs/globals.h"

using t_real = t_real_glob;

int main(int argc, char **argv)
{
	if(argc < 3)
	{
		tl::log_err("Usage:\n\t", argv[0], "<file1> <file2> ... <fileN> <file_out>");
		return -1;
	}

	tl::FileFrm<> dat0;
	tl::log_info("Loading file ", argv[1]);
	if(!dat0.Load(argv[1]))
	{
		tl::log_err("Cannot load data file ", argv[1], ".");
		return -1;
	}

	std::string strCnt = dat0.GetCountVar();
	std::string strMon = "mon2"; //dat0.GetMonVar();
	tl::log_info("Count var: ", strCnt, ", monitor var: ", strMon);

	tl::FileInstrBase<>::t_vecVals& vecCnt0 = dat0.GetCol(strCnt);
	tl::FileInstrBase<>::t_vecVals& vecMon0 = dat0.GetCol(strMon);


	for(int iArg=2; iArg<argc-1; ++iArg)
	{
		const char* pcFile = argv[iArg];

		tl::FileFrm<> dat;
		tl::log_info("Loading file ", pcFile);
	        if(!dat.Load(pcFile))
		{
			tl::log_err("Cannot load data file ", pcFile, ".");
			return -1;
		}

		const tl::FileInstrBase<>::t_vecVals& vecCnt = dat.GetCol(strCnt);
		const tl::FileInstrBase<>::t_vecVals& vecMon = dat.GetCol(strMon);

		if(vecCnt.size() != vecCnt0.size() || vecMon.size() != vecMon0.size())
		{
			tl::log_err("Size mismatch in file ", pcFile, ".");
			return -1;
		}

		for(unsigned int i=0; i<vecCnt0.size(); ++i)
		{
			vecCnt0[i] += vecCnt[i];
			vecMon0[i] += vecMon[i];
		}
	}



	tl::log_info("Saving file ", argv[argc-1]);
	std::ofstream ofstr(argv[argc-1]);
	if(!ofstr.is_open())
	{
		tl::log_err("Cannot save data file ", argv[argc-1], ".");
		return -1;
	}

	ofstr.precision(16);
	const tl::FileInstrBase<>::t_vecColNames& vecColNames = dat0.GetColNames();
	for(unsigned int i=0; i<vecCnt0.size(); ++i)
	{
		for(const std::string& strColName : vecColNames)
		{
			tl::FileInstrBase<>::t_vecVals& vecCol = dat0.GetCol(strColName);
			ofstr << std::setw(20) << vecCol[i];
		}
		ofstr << "\n";
	}

	return 0;
}
