/**
 * Plot scattering plane and positions from given scan files
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 2016, 30-jan-2017
 * @license GPLv2
 *
 * gcc -I../.. -I. -DNO_IOSTR -o scanpos ../../tlibs/file/loadinstr.cpp ../../tlibs/log/log.cpp ../../libs/globals.cpp scanpos.cpp -std=c++11 -lstdc++ -lm -lboost_system -lboost_filesystem -lboost_program_options
 */

#include "scanpos.h"
#include <boost/program_options.hpp>


namespace opts = boost::program_options;

using t_real = t_real_glob;
using t_vec = tl::ublas::vector<t_real>;
using t_mat = tl::ublas::matrix<t_real>;


bool make_plot(const std::string& strFile,
	const t_vec& vec0, const t_vec& vec1,
	const std::vector<t_vec>& vecAllHKL, const std::vector<t_vec>& vecAllPos,
	bool bFlip = 1)
{
	if(!vecAllPos.size())
		return false;

	std::ostream *pOstr = &std::cout;
	std::ofstream ofstr;

	if(strFile != "")
	{
		ofstr.open(strFile);
		if(!ofstr)
		{
			tl::log_err("Cannot write to file \"", strFile, "\".");
			return false;
		}

		pOstr = &ofstr;
	}

	// guess Bragg peak
	t_vec vecBraggHKL = tl::make_vec<t_vec>(
		{std::round(vecAllHKL[0][0]), std::round(vecAllHKL[0][1]), std::round(vecAllHKL[0][2])});

	return make_plot<t_vec, t_real>(*pOstr, vec0, vec1, vecBraggHKL, vecAllHKL, vecAllPos, bFlip);
}


int main(int argc, char** argv)
{
	// --------------------------------------------------------------------
	// get job files and program options
	std::vector<std::string> vecScans;
	std::string strOutFile;
	bool bFlip = 0;
	std::string strVec0, strVec1;
	t_vec vec0, vec1;

	// normal args
	opts::options_description args("Extracts scan positions into a figure");
	args.add(boost::shared_ptr<opts::option_description>(
		new opts::option_description("scan-file",
		opts::value<decltype(vecScans)>(&vecScans), "scan file")));
	args.add(boost::shared_ptr<opts::option_description>(
		new opts::option_description("flip-y",
		opts::bool_switch(&bFlip), "flip y axis")));
	args.add(boost::shared_ptr<opts::option_description>(
		new opts::option_description("out-file",
		opts::value<decltype(strOutFile)>(&strOutFile), "output file")));
	args.add(boost::shared_ptr<opts::option_description>(
		new opts::option_description("vec0",
		opts::value<decltype(strVec0)>(&strVec0), "first scattering plane vector")));
	args.add(boost::shared_ptr<opts::option_description>(
		new opts::option_description("vec1",
		opts::value<decltype(strVec1)>(&strVec1), "second scattering plane vector")));

	// positional args
	opts::positional_options_description args_pos;
	args_pos.add("scan-file", -1);

	opts::basic_command_line_parser<char> clparser(argc, argv);
	clparser.options(args);
	clparser.positional(args_pos);
	opts::basic_parsed_options<char> parsedopts = clparser.run();

	opts::variables_map opts_map;
	opts::store(parsedopts, opts_map);
	opts::notify(opts_map);

	if(argc <= 1)
	{
		std::ostringstream ostrHelp;
		ostrHelp << "Usage: " << argv[0] << " [options] <scan-file 1> <scan-file 2> ... <scan-file n>\n";
		ostrHelp << args;
		tl::log_info(ostrHelp.str());
		return -1;
	}

	if(vecScans.size() == 0)
	{
		tl::log_err("No scan files given.");
		return -1;
	}


	if(strVec0.size())
		vec0 = tl::str_to_var<decltype(vec0)>("[3](" + strVec0 + ")");
	if(strVec1.size())
		vec1 = tl::str_to_var<decltype(vec1)>("[3](" + strVec1 + ")");
	// --------------------------------------------------------------------


	// first scan file serves as reference
	std::unique_ptr<tl::FileInstrBase<t_real>> ptrScan(
		tl::FileInstrBase<t_real>::LoadInstr(vecScans[0].c_str()));
	if(!ptrScan)
	{
		tl::log_err("Invalid scan file: \"", vecScans[0], "\".");
		return -1;
	}


	// get scattering plane if not already given as program arg
	if(vec0.size() != 3)
	{
		auto arrVec0 = ptrScan->GetScatterPlane0();
		vec0 = tl::make_vec<t_vec>({arrVec0[0],arrVec0[1],arrVec0[2]});
	}
	if(vec1.size() != 3)
	{
		auto arrVec1 = ptrScan->GetScatterPlane1();
		vec1 = tl::make_vec<t_vec>({arrVec1[0],arrVec1[1],arrVec1[2]});
	}

	if(bFlip) vec1 = -vec1;
	tl::log_info("Scattering plane: ", vec0, ", ", vec1, ".");


	std::vector<t_vec> vecAllHKL, vecAllPos;
	vecAllHKL.reserve(argc-2);
	vecAllPos.reserve(argc-2);

	// first scan position
	auto vecPos = get_coord<t_vec, t_real>(vec0, vec1, *ptrScan);
	tl::log_info(vecPos.first, " rlu  ->  ", vecPos.second, ".");
	if(vecPos.second.size() != 2)
	{
		tl::log_err("Invalid scan position for file \"", vecScans[0], "\".");
		return -1;
	}
	vecAllHKL.push_back(vecPos.first);
	vecAllPos.push_back(vecPos.second);


	// load other files
	for(int iFile=1; iFile<vecScans.size(); ++iFile)
	{
		std::unique_ptr<tl::FileInstrBase<t_real>> ptrScanOther(
			tl::FileInstrBase<t_real>::LoadInstr(vecScans[iFile].c_str()));
		if(!ptrScanOther)
		{
			tl::log_err("Invalid scan file: \"", vecScans[iFile], "\".");
			continue;
		}

		// remaining scan positions
		auto vecPosOther = get_coord<t_vec, t_real>(vec0, vec1, *ptrScanOther);
		tl::log_info(vecPos.first, " rlu  ->  ", vecPos.second, ".");
		if(vecPosOther.second.size() != 2)
		{
			tl::log_err("Invalid scan position for file \"", vecScans[iFile], "\".");
			continue;
		}

		vecAllHKL.push_back(vecPosOther.first);
		vecAllPos.push_back(vecPosOther.second);
	}


	if(!make_plot(strOutFile, vec0, vec1, vecAllHKL, vecAllPos, bFlip))
	{
		tl::log_err("No scan positions found.");
		return -1;
	}

	return 0;
}
