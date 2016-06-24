// gcc -I../../ -o scanseries scanseries.cpp ../../tlibs/log/log.cpp -std=c++11 -lboost_system -lboost_filesystem -lstdc++
/**
 * processes multiple convo fit results
 * @author tweber
 * @date dec-2015
 * @license GPLv2
 */

#include <fstream>
#include <sstream>
#include <iostream>
#include "tlibs/string/string.h"
#include "tlibs/file/file.h"
#include "tlibs/log/log.h"


using t_real = double;
using t_map = std::map<std::string, std::pair<t_real, t_real>>;


// reads property maps from data files
bool get_fileprops(const char* pcFile, t_map& mapProps)
{
	std::ifstream ifstr(pcFile);
	if(!ifstr)
	{
		tl::log_err("Cannot open file \"", pcFile, "\".");
		return 0;
	}

	std::string strLine;
	while(std::getline(ifstr, strLine))
	{
		tl::trim(strLine);
		if(!strLine.size() || strLine[0] != '#')
			continue;

		strLine = strLine.substr(1);
		std::pair<std::string, std::string> strKeyVal = 
			tl::split_first(strLine, std::string("="), 1);

		std::string& strKey = strKeyVal.first;
		std::string& _strVal = strKeyVal.second;

		if(!strKey.size())
			continue;


		std::vector<t_real> vecHKLE;
		tl::get_tokens<t_real>(_strVal, std::string(" \t"), vecHKLE);

		// value & error pair
		if(_strVal.find("+-") != std::string::npos)
		{
			std::pair<std::string, std::string> strValErr =
				tl::split_first(_strVal, std::string("+-"), 1, 1);

			std::string& strVal = strValErr.first;
			std::string& strErr = strValErr.second;

			//std::cout << strKey << ": " << strVal << ", " << strErr << std::endl;
			if(strVal.length())
			{
				t_real dVal = tl::str_to_var<t_real>(strVal);
				t_real dErr = tl::str_to_var<t_real>(strErr);

				mapProps.insert(t_map::value_type(strKey, {dVal, dErr}));
			}
		}
		// hklE values -> split them, e.g. "scan_dir = 0 0 0 1" -> "scan_dir_h = 0", ...
		else if(vecHKLE.size()==3 || vecHKLE.size()==4)
		{
			std::vector<std::string> vecSuffixes = {"_h", "_k", "_l", "_E"};

			std::size_t iNumCoords = std::min(vecSuffixes.size(), vecHKLE.size());
			for(unsigned int iCoord=0; iCoord<iNumCoords; ++iCoord)
			{
				std::string strNewKey = strKey + vecSuffixes[iCoord];
				mapProps.insert(t_map::value_type(strNewKey, {vecHKLE[iCoord], 0.}));
			}
		}
		else
		{
			tl::log_warn("Unknown key: \"", strKey, "\".");
			continue;
		}
	}

	return 1;
}


int main(int argc, char** argv)
{
	if(argc<=1)
	{
		std::cerr << "Usage:\n";
		std::cerr << "\t" << argv[0] << " <out dir>  <var1> <var2> ..." << std::endl;
		std::cerr << "\te.g." << argv[0] << " out  T TA2_E_HWHM TA2_amp" << std::endl;
		return -1;
	}

	std::string strOutDir = argv[1];

	std::vector<std::string> vecCols;
	for(int iArg=2; iArg<argc; ++iArg)
		vecCols.push_back(argv[iArg]);



	std::string strOut = strOutDir + "/result.dat";
	std::ofstream ofstr(strOut);
	if(!ofstr)
	{
		tl::log_err("Cannot open output file \"", strOut, "\".");
		return -1;
	}
	ofstr.precision(16);


	for(std::size_t iCol=0; iCol<vecCols.size(); ++iCol)
	{
		if(iCol==0)
			ofstr << std::left << std::setw(42) << ("# " + vecCols[iCol]);
		else
			ofstr << std::left << std::setw(42) << vecCols[iCol];
	}
	ofstr << "\n";


	for(unsigned iNr=1; 1; ++iNr)
	{
		std::ostringstream ostrScFile;
		ostrScFile << strOutDir << "/sc" << iNr << ".dat";
		std::ostringstream ostrModFile;
		ostrModFile << strOutDir <<  "/mod" << iNr << ".dat";


		if(!tl::file_exists(ostrScFile.str().c_str()) ||
			!tl::file_exists(ostrModFile.str().c_str()))
			break;

		tl::log_info("Processing dataset ", iNr, ".");

		t_map map;
		get_fileprops(ostrScFile.str().c_str(), map);
		get_fileprops(ostrModFile.str().c_str(), map);

		for(const std::string& strCol : vecCols)
		{
			t_map::const_iterator iter = map.find(strCol);
			ofstr << std::left << std::setw(20) << iter->second.first << " ";
			ofstr << std::left << std::setw(20) << iter->second.second << " ";
		}

		ofstr << "\n";
	}

	tl::log_info("Wrote \"", strOut, "\".");
	return 0;
}
