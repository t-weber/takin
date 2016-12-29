/**
 * monte carlo convolution tool
 * @author tweber
 * @date jun-2015
 * @license GPLv2
 */

#include <clocale>
#include <iostream>
#include <sstream>
#include <fstream>
#include <memory>
#include <unordered_map>

#include "tlibs/string/string.h"
#include "tlibs/log/log.h"
#include "tlibs/file/loaddat.h"
#include "TASReso.h"
#include "sqw.h"
#include "../res/defs.h"

using t_real = t_real_reso;


static inline void usage(const char* pcProg)
{
	std::ostringstream ostr;
	ostr << "Usage: "
		<< "\n\t(1), single point: " << pcProg << " <mc neutron file> <S(Q,w) file>"
		<< "\n\t(2), phonon model: " << pcProg << " <resolution file> <crystal file> <steps file> <out file>"
		<< "\n\t(3), S(q,w) file:  " << pcProg << " <resolution file> <crystal file> <S(Q,w) file> <steps file> <out file>";

	tl::log_err("Wrong arguments.\n", ostr.str());
}


static inline int monteconvo_simple(const char* pcNeutrons, const char* pcSqw)
{
	std::ifstream ifstrNeutr(pcNeutrons);
	if(!ifstrNeutr.is_open())
	{
		tl::log_err("Cannot open neutrons file \"", pcNeutrons, "\".");
		return -1;
	}

	std::shared_ptr<SqwBase> ptrSqw(new SqwKdTree(pcSqw));
	SqwBase *psqw = ptrSqw.get();
	if(!psqw->IsOk())
	{
		tl::log_err("Cannot open Sqw file \"", pcSqw, "\".");
		return -2;
	}

	unsigned int iCurNeutr = 0;
	std::unordered_map<std::string, std::string> mapNeutrParams;
	t_real dS = 0.;
	t_real dhklE[4] = {0., 0., 0., 0.};

	while(!ifstrNeutr.eof())
	{
		std::string strLine;
		std::getline(ifstrNeutr, strLine);
		tl::trim(strLine);

		if(strLine.size() == 0)
			continue;

		if(strLine[0] == '#')
		{
			strLine[0] = ' ';
			mapNeutrParams.insert(tl::split_first(strLine, std::string(":"), 1));
			continue;
		}

		/*if(mapNeutrParams["coord_sys"] != "rlu")
		{
			tl::log_err("Need rlu coordinate system.");
			return -3;
		}*/

		std::vector<t_real> vecNeutr;
		tl::get_tokens<t_real>(strLine, std::string(" \t"), vecNeutr);
		if(vecNeutr.size() != 4)
		{
			tl::log_err("Need h,k,l,E data.");
			return -3;
		}

		//tl::log_info("Neutron ", iCurNeutr, ": ", vecNeutr[0], ", ", vecNeutr[1], ", ", vecNeutr[2], ", ", vecNeutr[3]);

		for(int i=0; i<4; ++i) dhklE[i] += vecNeutr[i];
		//sqw.SetNeutronParams(&mapNeutrParams);
		dS += (*psqw)(vecNeutr[0], vecNeutr[1], vecNeutr[2], vecNeutr[3]);

		++iCurNeutr;
	}

	dS /= t_real(iCurNeutr+1);

	for(int i=0; i<4; ++i)
		dhklE[i] /= t_real(iCurNeutr+1);

	tl::log_info("Processed ",  iCurNeutr, " MC neutrons.");
	tl::log_info("S(", dhklE[0], ", ", dhklE[1],  ", ", dhklE[2], ", ", dhklE[3], ") = ", dS);
	return 0;
}


static inline int monteconvo(const char* pcRes, const char* pcCrys,
	const char* pcSqw, const char* pcSteps, const char* pcOut)
{
	TASReso reso;
	tl::log_info("Loading resolution file \"", pcRes, "\".");
	if(!reso.LoadRes(pcRes))
		return -1;

	tl::log_info("Loading crystal file \"", pcRes, "\".");
	if(!reso.LoadLattice(pcCrys))
		return -2;


	tl::DatFile<t_real, char> steps;
	tl::log_info("Loading scan steps file \"", pcRes, "\".");
	if(!steps.Load(pcSteps))
	{
		tl::log_err("Cannot load steps file.");
		return -3;
	}
	if(steps.GetColumnCount() != 4)
	{
		tl::log_err("Need 4 columns in step file: h k l E.");
		return -3;
	}
	const unsigned int iNumSteps = steps.GetRowCount();
	tl::log_info("Number of scan steps: ", iNumSteps);
	const t_real *pH = steps.GetColumn(0).data();
	const t_real *pK = steps.GetColumn(1).data();
	const t_real *pL = steps.GetColumn(2).data();
	const t_real *pE = steps.GetColumn(3).data();

	unsigned int iNumNeutrons = 0;
	try
	{
		iNumNeutrons = tl::str_to_var<unsigned int>(steps.GetHeader().at("num_neutrons"));
		reso.SetAlgo(ResoAlgo(tl::str_to_var<int>(steps.GetHeader().at("algo"))));
		bool bFixedKi = tl::str_to_var<bool>(steps.GetHeader().at("fixed_ki"));
		t_real dKFix = tl::str_to_var<t_real>(steps.GetHeader().at("kfix"));

		reso.SetKiFix(bFixedKi);
		reso.SetKFix(dKFix);
	}
	catch(const std::out_of_range& ex)
	{
		tl::log_err("Need keys \"num_neutrons\", \"fixed_ki\", \"kfix\" and \"algo\" in steps file.");
		return -3;
	}


	tl::log_info("Number of neutrons: ", iNumNeutrons);


	std::shared_ptr<SqwBase> ptrSqw;
	//std::shared_ptr<SqwBase> ptrSqw(new SqwElast());

	if(pcSqw)
	{
		tl::log_info("Loading S(Q,w) file \"", pcSqw, "\".");
		ptrSqw.reset(new SqwKdTree(pcSqw));
	}
	else
	{
		tl::log_info("Using phonon model.");
		ptrSqw.reset(new SqwPhonon(tl::make_vec({4.,4.,0}),
				tl::make_vec({0.,0.,1.}), tl::make_vec({1.,-1.,0.}),
				40., M_PI/2., 0.5, 0.5, 1.,
				12., M_PI/2., 0.5, 0.5, 1.,
				18., M_PI/2., 0.5, 0.5, 1.,
				100.));
	}


	SqwBase *psqw = ptrSqw.get();

	if(!psqw->IsOk())
	{
		tl::log_err("Cannot init Sqw.");
		return -4;
	}


	std::ofstream ofstrOut(pcOut);
	ofstrOut << "#\n";
	ofstrOut << "# Format: h k l E S\n";
	ofstrOut << "#\n";

	std::vector<ublas::vector<t_real>> vecNeutrons;
	for(unsigned int iStep=0; iStep<iNumSteps; ++iStep)
	{
		t_real dProgress = t_real(iStep)/t_real(iNumSteps)*100.;

		tl::log_info("------------------------------------------------------------");
		tl::log_info("Step ", iStep+1, " of ", iNumSteps, ".");
		tl::log_info("Q = (", pH[iStep], " ", pK[iStep], " ", pL[iStep], "), E = ", pE[iStep], " meV.");
		if(!reso.SetHKLE(pH[iStep], pK[iStep], pL[iStep], pE[iStep]))
		{
			tl::log_err("Invalid position.");
			break;
		}

		std::cout <<"\x1b]0;"
			<< std::setprecision(3) << dProgress <<  "%"
			<< " - generating MC neutrons"
			<< "\x07" << std::flush;
		Ellipsoid4d<t_real> elli = reso.GenerateMC(iNumNeutrons, vecNeutrons);

		t_real dS = 0.;
		t_real dhklE_mean[4] = {0., 0., 0., 0.};

		std::cout <<"\x1b]0;"
			<< std::setprecision(3) << dProgress <<  "%"
			<< " - calculating S(q,w)"
			<< "\x07" << std::flush;
		for(const ublas::vector<t_real>& vecHKLE : vecNeutrons)
		{
			dS += (*psqw)(vecHKLE[0], vecHKLE[1], vecHKLE[2], vecHKLE[3]);

			for(int i=0; i<4; ++i)
				dhklE_mean[i] += vecHKLE[i];
		}

		dS /= t_real(iNumNeutrons);
		for(int i=0; i<4; ++i)
			dhklE_mean[i] /= t_real(iNumNeutrons);

		ofstrOut.precision(16);
		ofstrOut << std::left << std::setw(20) << pH[iStep] << " "
			<< std::left << std::setw(20) << pK[iStep] << " "
			<< std::left << std::setw(20) << pL[iStep] << " "
			<< std::left << std::setw(20) << pE[iStep] << " "
			<< std::left << std::setw(20) << dS << "\n";

		tl::log_info("Mean position: Q = (", dhklE_mean[0], " ", dhklE_mean[1], " ", dhklE_mean[2], "), E = ", dhklE_mean[3], " meV.");
		tl::log_info("S(", pH[iStep], ", ", pK[iStep],  ", ", pL[iStep], ", ", pE[iStep], ") = ", dS);
	}
	std::cout <<"\x1b]0;" << "100%" << "\x07" << std::flush;

	tl::log_info("Wrote output file \"", pcOut, "\".");
	ofstrOut.close();
	return 0;
}



// TODO: create non-Qt xml loader and remove linking to Qt...
#include <QApplication>
#include <QLocale>

int main(int argc, char** argv)
{
	QApplication app(argc, argv, 0);
	std::setlocale(LC_ALL, "C");
	QLocale::setDefault(QLocale::English);

	if(argc == 3)
	{
		return monteconvo_simple(argv[1], argv[2]);
	}
	else if(argc==5)
	{
		const char *pcRes = argv[1];
		const char *pcCrys = argv[2];
		const char *pcSteps = argv[3];
		const char *pcOut = argv[4];
		return monteconvo(pcRes, pcCrys, 0, pcSteps, pcOut);
	}
	else if(argc==6)
	{
		const char *pcRes = argv[1];
		const char *pcCrys = argv[2];
		const char *pcSqw = argv[3];
		const char *pcSteps = argv[4];
		const char *pcOut = argv[5];
		return monteconvo(pcRes, pcCrys, pcSqw, pcSteps, pcOut);
	}
	else
	{
		usage(argv[0]);
		return -1;
	}
}
