/**
 * Extracts (hkl), E positions from scan files for monteconvo/reso
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 25-jul-2015, 20-jul-2019
 * @license GPLv2
 *
 * g++ -o posextract posextract.cpp ../../tlibs/log/log.cpp -I../.. -DTLIBS_INC_HDR_IMPLS -lboost_system -lboost_iostreams -lboost_program_options
 */

#include "tlibs/file/loadinstr.h"
#include "tlibs/log/log.h"
#include "tlibs/phys/neutrons.h"
#include "libs/globals.h"
#include <memory>
#include <fstream>


using t_real = t_real_glob;
static const tl::t_length_si<t_real> angs = tl::get_one_angstrom<t_real>();
static const tl::t_energy_si<t_real> meV = tl::get_one_meV<t_real>();
static const std::size_t iPrec = 8;
static const t_real dPadding = 2.;

#include <boost/program_options.hpp>
namespace opts = boost::program_options;



static void extract_monteconvo_pos(const char* pcIn, const char* pcOut)
{
	std::shared_ptr<tl::FileInstrBase<t_real>> ptrInstr(
		tl::FileInstrBase<t_real>::LoadInstr(pcIn));
	tl::FileInstrBase<t_real> *pInstr = ptrInstr.get();

	if(!pInstr)
	{
		tl::log_err("Cannot load scan file \"", pcIn, "\".");
		return;
	}


	std::ofstream ofstr(pcOut);
	if(!ofstr.is_open())
	{
		tl::log_err("Cannot open output file \"", pcOut, "\".");
		return;
	}

	ofstr.precision(iPrec);
	ofstr << "#\n";
	ofstr << "# num_neutrons: 1000\n";
	ofstr << "# algo: 1\n";
	ofstr << "#\n";
	ofstr << "# fixed_ki: " << pInstr->IsKiFixed() << "\n";
	ofstr << "# kfix: " << pInstr->GetKFix() << "\n";
	ofstr << "#\n";
	ofstr << "# h k l E data follows\n";
	ofstr << "#\n";

	tl::log_info("Extracting ", pInstr->GetScanCount(), " scan positions.");
	for(std::size_t iScan=0; iScan<pInstr->GetScanCount(); ++iScan)
	{
		std::array<t_real,5> arrPos = pInstr->GetScanHKLKiKf(iScan);
		t_real dh = arrPos[0];
		t_real dk = arrPos[1];
		t_real dl = arrPos[2];
		t_real dki = arrPos[3];
		t_real dkf = arrPos[4];
		t_real dE = (tl::k2E(dki/angs) - tl::k2E(dkf/angs))/meV;

		ofstr
			<< std::left << std::setw(iPrec*dPadding) << dh << " "
			<< std::left << std::setw(iPrec*dPadding) << dk << " "
			<< std::left << std::setw(iPrec*dPadding) << dl << " "
			<< std::left << std::setw(iPrec*dPadding) << dE << std::endl;
	}

	tl::log_info("Done.");
	ofstr.close();
}


static void extract_pos(
	const std::vector<std::string>& vecScans,
	const std::vector<std::string>& vecCols,
	const std::string& strOut)
{
	std::ostream *pOstr = &std::cout;
	std::ofstream ofstr;

	if(strOut != "")
	{
		ofstr.open(strOut);
		if(ofstr.is_open())
			pOstr = &ofstr;
		else
			tl::log_err("Cannot open output file \"", strOut, "\", using standard output.");
	}

	pOstr->precision(iPrec);


	for(const std::string& strScan : vecScans)
	{
		tl::log_info("Loading \"", strScan, "\".");

		std::shared_ptr<tl::FileInstrBase<t_real>> ptrInstr(
			tl::FileInstrBase<t_real>::LoadInstr(strScan.c_str()));

		if(!ptrInstr)
		{
			tl::log_err("Cannot load scan file \"", strScan, "\", skipping.");
			continue;
		}

		tl::log_info("Extracting ", ptrInstr->GetScanCount(), " scan positions.");


		std::string strMonVar = ptrInstr->GetMonVar();
		std::string strCtrVar = ptrInstr->GetCountVar();
		std::vector<std::vector<t_real>> vecUserCols;

		const auto& vecMon = ptrInstr->GetCol(strMonVar);
		const auto& vecCtr = ptrInstr->GetCol(strCtrVar);


		(*pOstr)
			<< std::left << std::setw(iPrec*dPadding) << "# h" << " "
			<< std::left << std::setw(iPrec*dPadding) << "k" << " "
			<< std::left << std::setw(iPrec*dPadding) << "l" << " "
			<< std::left << std::setw(iPrec*dPadding) << "E" << " "
			<< std::left << std::setw(iPrec*dPadding) << "I" << " "
			<< std::left << std::setw(iPrec*dPadding) << "dI" << " ";

		for(const std::string& strCol : vecCols)
		{
			const auto& vecUserCol = ptrInstr->GetCol(strCol);
			if(vecUserCol.size() != ptrInstr->GetScanCount())
			{
				tl::log_err("Invalid data column \"", strCol, "\" in scan file \"", strScan, "\", skipping.");
				continue;
			}

			vecUserCols.push_back(vecUserCol);

			(*pOstr) << std::left << std::setw(iPrec*dPadding) << strCol << " ";
		}

		(*pOstr) << std::endl;


		for(std::size_t iScanPos=0; iScanPos<ptrInstr->GetScanCount(); ++iScanPos)
		{
			t_real m = vecMon[iScanPos];
			t_real c = vecCtr[iScanPos];
			t_real dm = tl::float_equal(m, 0.) ? 1. : std::sqrt(m);
			t_real dc = tl::float_equal(c, 0.) ? 1. : std::sqrt(c);

			std::array<t_real,5> arrPos = ptrInstr->GetScanHKLKiKf(iScanPos);
			t_real dh = arrPos[0];
			t_real dk = arrPos[1];
			t_real dl = arrPos[2];
			t_real dki = arrPos[3];
			t_real dkf = arrPos[4];
			t_real dE = (tl::k2E(dki/angs) - tl::k2E(dkf/angs))/meV;
			t_real dCtsPerMon = c / m;
			t_real dCtsPerMonErr = std::sqrt(std::pow(dc/m, 2.) + std::pow(dm*c/(m*m), 2.));

			(*pOstr)
				<< std::left << std::setw(iPrec*dPadding) << dh << " "
				<< std::left << std::setw(iPrec*dPadding) << dk << " "
				<< std::left << std::setw(iPrec*dPadding) << dl << " "
				<< std::left << std::setw(iPrec*dPadding) << dE << " "
				<< std::left << std::setw(iPrec*dPadding) << dCtsPerMon << " "
				<< std::left << std::setw(iPrec*dPadding) << dCtsPerMonErr << " ";

			for(const auto& vecCol : vecUserCols)
			{
				(*pOstr) << std::left << std::setw(iPrec*dPadding) << vecCol[iScanPos] << " ";
			}

			(*pOstr) << std::endl;
		}
	}
}


int main(int argc, char** argv)
{
	std::ios_base::sync_with_stdio(0);

	std::vector<std::string> vecScans;
	std::vector<std::string> vecCols;
	std::string strOutFile;
	bool bMonteconvo = 0;

	opts::options_description args("program options");
	args.add(boost::shared_ptr<opts::option_description>(
		new opts::option_description("scan-file",
		opts::value<decltype(vecScans)>(&vecScans), "scan file")));
	args.add(boost::shared_ptr<opts::option_description>(
		new opts::option_description("column",
		opts::value<decltype(vecCols)>(&vecCols), "include column")));
	args.add(boost::shared_ptr<opts::option_description>(
		new opts::option_description("out-file",
		opts::value<decltype(strOutFile)>(&strOutFile),
		"output file")));
	args.add(boost::shared_ptr<opts::option_description>(
		new opts::option_description("monteconvo",
		opts::bool_switch(&bMonteconvo),
		"extract positions for monteconvo")));

	opts::positional_options_description args_pos;
	args_pos.add("scan-file", -1);

	opts::basic_command_line_parser<char> clparser(argc, argv);
	clparser.options(args);
	clparser.positional(args_pos);
	opts::basic_parsed_options<char> parsedopts = clparser.run();

	opts::variables_map opts_map;
	opts::store(parsedopts, opts_map);
	opts::notify(opts_map);


	if(argc < 2)
	{
		std::cerr << args << std::endl;
		return -1;
	}

	if(vecScans.size() < 1)
	{
		tl::log_err("No input scan files given.");
		return -1;
	}


	if(bMonteconvo)
		extract_monteconvo_pos(vecScans[0].c_str(), strOutFile.c_str());
	else
		extract_pos(vecScans, vecCols, strOutFile);

	return 0;
}
