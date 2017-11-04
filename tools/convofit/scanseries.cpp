/**
 * processes multiple convo fit results
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date dec-2015
 * @license GPLv2
 */
// gcc -I../../ -o scanseries scanseries.cpp ../../tlibs/log/log.cpp -std=c++11 -lboost_system -lboost_filesystem -lstdc++

#include <fstream>
#include <sstream>
#include <iostream>
#include "tlibs/string/string.h"
#include "tlibs/file/file.h"
#include "tlibs/file/prop.h"
#include "tlibs/log/log.h"


using t_real = double;
using t_map = std::map<std::string, std::pair<t_real, t_real>>;


// reads property maps from data files
bool get_fileprops(const std::string& strFile, t_map& mapProps)
{
	std::ifstream ifstr(strFile);
	if(!ifstr)
	{
		tl::log_err("Cannot open file \"", strFile, "\".");
		return 0;
	}

	std::string strLine;
	while(std::getline(ifstr, strLine))
	{
		tl::trim(strLine);
		// only collect lines starting with "#"
		if(!strLine.size() || strLine[0] != '#')
			continue;
		// ignore comments starting with "##"
		if(strLine.size()>=2 && strLine[0]=='#' && strLine[1]=='#')
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
			for(std::size_t iCoord=0; iCoord<iNumCoords; ++iCoord)
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


/**
 * old behaviour expecting numbered files
 */
int invoke_old(int argc, char** argv)
{
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

		tl::log_info("Processing dataset ", iNr, ": \"", strOutDir, "/[sc|mod]", iNr, ".dat\".");

		t_map map;
		get_fileprops(ostrScFile.str(), map);
		get_fileprops(ostrModFile.str(), map);

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


/**
 * new behaviour expecting a config file
 */
int invoke_new(int argc, char** argv)
{
	if(argc < 3) return -1;

	std::string strFile = argv[2];
	tl::Prop<std::string> file;
	if(!file.Load(strFile, tl::PropType::INFO))
	{
		tl::log_err("Cannot open config file \"", strFile, "\".");
		return -1;
	}


	// output directory
	std::string strOutDir = file.Query<std::string>("convoseries/out_dir");
	tl::trim(strOutDir);
	if(strOutDir == "")
	{
		tl::log_err("No output directory given.");
		return -1;
	}


	// variables
	std::string strVars = file.Query<std::string>("convoseries/vars");
	std::vector<std::string> vecCols;
	tl::get_tokens<std::string, std::string, decltype(vecCols)>(strVars, std::string(";"), vecCols);
	for(std::string& strVar : vecCols)
		tl::trim(strVar);
	if(!vecCols.size())
	{
		tl::log_err("No variables given.");
		return -1;
	}


	// files
	std::size_t iFile = 1;
	std::vector<std::string> vecMod, vecScn;
	while(1)
	{
		std::ostringstream ostrFiles;
		ostrFiles << "convoseries/files_" << iFile;
		if(!file.Exists(ostrFiles.str()))
			break;

		std::string strFiles = file.Query<std::string>(ostrFiles.str());
		std::vector<std::string> vecFiles;
		tl::get_tokens<std::string, std::string, decltype(vecFiles)>(strFiles, std::string(";"), vecFiles);
		if(vecFiles.size() != 2)
		{
			tl::log_err("Invalid number of files given in \"", ostrFiles.str(),
				"\", has to be exactly 2: model and scan.");
			break;
		}
		for(std::string& strFile : vecFiles)
			tl::trim(strFile);

		if(vecFiles[0]=="" || vecFiles[1]=="")
		{
			tl::log_err("Invalid files given in \"", ostrFiles.str(), "\".");
			break;
		}

		vecMod.push_back(vecFiles[0]);
		vecScn.push_back(vecFiles[1]);

		++iFile;
	}

	if(vecMod.size()==0 || vecScn.size()==0)
	{
		tl::log_err("No enough model/scan files given.");
		return -1;
	}

	tl::log_info(vecMod.size(), " input files and ", vecCols.size(), " variables given.",
		" Output directory: ", strOutDir, ".");



	// process files
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


	for(unsigned iNr=0; iNr<vecMod.size(); ++iNr)
	{
		const std::string& strModFile = vecMod[iNr];
		const std::string& strScFile = vecScn[iNr];

		if(!tl::file_exists(strScFile.c_str()) ||
			!tl::file_exists(strModFile.c_str()))
			break;

		tl::log_info("Processing dataset ", iNr+1, ": \"", strModFile, "\".");

		t_map map;
		get_fileprops(strScFile, map);
		get_fileprops(strModFile, map);

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



int main(int argc, char** argv)
{
#ifdef NO_TERM_CMDS
	tl::Log::SetUseTermCmds(0);
#endif

	if(argc <= 1)
	{
		std::cerr << "Usage 1:\n";
		std::cerr << "\t" << argv[0] << " -f <input.cfg>" << "\n";

		std::cerr << "Usage 2:\n";
		std::cerr << "\t" << argv[0] << " <out dir>  <var1> <var2> ...\n";
		std::cerr << "\te.g." << argv[0] << " out  T TA2_E_HWHM TA2_amp" << std::endl;
		return -1;
	}

	if(argc >= 3 && std::string(argv[1]) == "-f")
		return invoke_new(argc, argv);
	else
		return invoke_old(argc, argv);
}
