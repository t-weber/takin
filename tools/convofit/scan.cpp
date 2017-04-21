/**
 * Convolution fitting
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date dec-2015
 * @license GPLv2
 */

#include "scan.h"
#include "tlibs/log/log.h"
#include "tlibs/math/stat.h"

#include <fstream>


bool save_file(const char* pcFile, const Scan& sc)
{
	std::ofstream ofstr(pcFile);
	if(!ofstr)
		return false;

	ofstr.precision(16);

	ofstr << "# scan_origin = " 
		<< sc.vecScanOrigin[0] << " "
		<< sc.vecScanOrigin[1] << " "
		<< sc.vecScanOrigin[2] << " "
		<< sc.vecScanOrigin[3] << "\n";
	ofstr << "# scan_dir = " 
		<< sc.vecScanDir[0] << " "
		<< sc.vecScanDir[1] << " "
		<< sc.vecScanDir[2] << " "
		<< sc.vecScanDir[3] << "\n";
	ofstr << "# T = " << sc.dTemp << " +- " << sc.dTempErr << "\n";
	ofstr << "# B = " << sc.dField << " +- " << sc.dFieldErr << "\n";

	ofstr << "#\n";

	ofstr << std::left << std::setw(21) << "# x"
		<< std::left << std::setw(21) << "counts"
		<< std::left << std::setw(21) << "count errors"
		<< std::left << std::setw(21) << "monitor"
		<< std::left << std::setw(21) << "monitor errors" << "\n";

	const std::size_t iNum = sc.vecX.size();
	for(std::size_t i=0; i<iNum; ++i)
	{
		ofstr << std::left << std::setw(20) << sc.vecX[i] << " "
			<< std::left << std::setw(20) << sc.vecCts[i] << " "
			<< std::left << std::setw(20) << sc.vecCtsErr[i] << " "
			<< std::left << std::setw(20) << sc.vecMon[i] << " "
			<< std::left << std::setw(20) << sc.vecMonErr[i] << "\n";
	}

	return true;
}


bool load_file(const std::vector<std::string>& vecFiles, Scan& scan, bool bNormToMon,
	const Filter& filter, bool bFlipCoords)
{
	if(!vecFiles.size()) return 0;
	tl::log_info("Loading \"", vecFiles[0], "\".");

	std::unique_ptr<tl::FileInstrBase<t_real_sc>> pInstr(tl::FileInstrBase<t_real_sc>::LoadInstr(vecFiles[0].c_str()));
	if(!pInstr)
	{
		tl::log_err("Cannot load \"", vecFiles[0], "\".");
		return false;
	}

	for(std::size_t iFile=1; iFile<vecFiles.size(); ++iFile)
	{
		tl::log_info("Loading \"", vecFiles[iFile], "\" for merging.");
		std::unique_ptr<tl::FileInstrBase<t_real_sc>> pInstrM(tl::FileInstrBase<t_real_sc>::LoadInstr(vecFiles[iFile].c_str()));
		if(!pInstrM)
		{
			tl::log_err("Cannot load \"", vecFiles[iFile], "\".");
			continue;
		}

		pInstr->MergeWith(pInstrM.get());
	}


	std::string strCountVar = pInstr->GetCountVar();	// defaults
	std::string strMonVar = pInstr->GetMonVar();
	if(scan.strCntCol != "") strCountVar = scan.strCntCol;	// overrides
	if(scan.strMonCol != "") strMonVar = scan.strMonCol;
	tl::log_info("Counts column: ", strCountVar, "\nMonitor column: ", strMonVar, ".");

	scan.vecCts = pInstr->GetCol(strCountVar);
	scan.vecMon = pInstr->GetCol(strMonVar);

	std::function<t_real_sc(t_real_sc)> funcErr = [](t_real_sc d) -> t_real_sc
	{
		//if(tl::float_equal<t_real_sc>(d, 0.))	// error 0 causes problems with minuit
		//	return d/100.;
		return std::sqrt(d);
	};
	scan.vecCtsErr = tl::apply_fkt(scan.vecCts, funcErr);
	scan.vecMonErr = tl::apply_fkt(scan.vecMon, funcErr);

	if(bNormToMon)
	{
		for(std::size_t iPos=0; iPos<scan.vecCts.size(); ++iPos)
		{
			t_real_sc y = scan.vecCts[iPos];
			t_real_sc dy = scan.vecCtsErr[iPos];
			t_real_sc m = scan.vecMon[iPos];
			t_real_sc dm  = scan.vecMonErr[iPos];

			scan.vecCts[iPos] /= m;
			scan.vecCtsErr[iPos] = std::sqrt(dy/m * dy/m  +  y*dm/(m*m) * y*dm/(m*m));
		}
	}

	const std::array<t_real_sc, 3> latt = pInstr->GetSampleLattice();
	const std::array<t_real_sc, 3> ang = pInstr->GetSampleAngles();

	scan.sample.a = latt[0]; scan.sample.b = latt[1]; scan.sample.c = latt[2];
	scan.sample.alpha = ang[0]; scan.sample.beta = ang[1]; scan.sample.gamma = ang[2];

	tl::log_info("Sample lattice: ", scan.sample.a, " ", scan.sample.b, " ", scan.sample.c, ".");
	tl::log_info("Sample angles: ", tl::r2d(scan.sample.alpha), " ", tl::r2d(scan.sample.beta), " ", tl::r2d(scan.sample.gamma), ".");


	const std::array<t_real_sc, 3> vec1 = pInstr->GetScatterPlane0();
	const std::array<t_real_sc, 3> vec2 = pInstr->GetScatterPlane1();
	t_real_sc dFlip = bFlipCoords ? t_real_sc(-1) : t_real_sc(1);
	scan.plane.vec1[0] = vec1[0]; scan.plane.vec1[1] = vec1[1]; scan.plane.vec1[2] = vec1[2];
	scan.plane.vec2[0] = dFlip*vec2[0]; scan.plane.vec2[1] = dFlip*vec2[1]; scan.plane.vec2[2] = dFlip*vec2[2];

	tl::log_info("Scattering plane: [", vec1[0], vec1[1], vec1[2], "], "
		"[", vec2[0], vec2[1], vec2[2], "].");
	if(bFlipCoords)
		tl::log_info("Flipped RHS <-> LHS coordinate system.");


	scan.bKiFixed = pInstr->IsKiFixed();
	scan.dKFix = pInstr->GetKFix();
	if(scan.bKiFixed)
		tl::log_info("ki = ", scan.dKFix, ".");
	else
		tl::log_info("kf = ", scan.dKFix, ".");


	const tl::FileInstrBase<t_real_sc>::t_vecVals& vecTemp = pInstr->GetCol(scan.strTempCol);
	if(vecTemp.size() == 0)
	{
		tl::log_warn("Sample temperature column \"", scan.strTempCol, "\" not found.");
		//return false;
	}
	else
	{
		scan.dTemp = tl::mean_value(vecTemp);
		scan.dTempErr = tl::std_dev(vecTemp);
		tl::log_info("Sample temperature: ", scan.dTemp, " +- ", scan.dTempErr);
	}

	const tl::FileInstrBase<t_real_sc>::t_vecVals& vecField = pInstr->GetCol(scan.strFieldCol);
	if(vecField.size() == 0)
	{
		tl::log_warn("Sample field column \"", scan.strFieldCol, "\" not found.");
		//return false;
	}
	else
	{
		scan.dField = tl::mean_value(vecField);
		scan.dFieldErr = tl::std_dev(vecField);
		tl::log_info("Sample field: ", scan.dField, " +- ", scan.dFieldErr, ".");
	}


	const std::size_t iNumPts = pInstr->GetScanCount();
	for(std::size_t iPt=0; iPt<iNumPts; ++iPt)
	{
		const std::array<t_real_sc, 5> sc = pInstr->GetScanHKLKiKf(iPt);

		ScanPoint pt;
		pt.h = sc[0]; pt.k = sc[1]; pt.l = sc[2];
		pt.ki = sc[3]/tl::get_one_angstrom<t_real_sc>();
		pt.kf = sc[4]/tl::get_one_angstrom<t_real_sc>();
		pt.Ei = tl::k2E(pt.ki); pt.Ef = tl::k2E(pt.kf);
		pt.E = pt.Ei-pt.Ef;

		tl::log_info("Point ", iPt+1, ": ", "h=", pt.h, ", k=", pt.k, ", l=", pt.l,
			", ki=", t_real_sc(pt.ki*tl::get_one_angstrom<t_real_sc>()), 
			", kf=", t_real_sc(pt.kf*tl::get_one_angstrom<t_real_sc>()),
			", E=", t_real_sc(pt.E/tl::get_one_meV<t_real_sc>())/*, ", Q=", pt.Q*tl::angstrom*/,
			", Cts=", scan.vecCts[iPt]/*, "+-", scan.vecCtsErr[iPt]*/,
			", Mon=", scan.vecMon[iPt]/*, "+-", scan.vecMonErr[iPt]*/, ".");

		scan.vecPoints.emplace_back(std::move(pt));
	}



	const ScanPoint& ptBegin = *scan.vecPoints.cbegin();
	const ScanPoint& ptEnd = *scan.vecPoints.crbegin();

	scan.vecScanOrigin[0] = ptBegin.h;
	scan.vecScanOrigin[1] = ptBegin.k;
	scan.vecScanOrigin[2] = ptBegin.l;
	scan.vecScanOrigin[3] = ptBegin.E / tl::get_one_meV<t_real_sc>();

	scan.vecScanDir[0] = ptEnd.h - ptBegin.h;
	scan.vecScanDir[1] = ptEnd.k - ptBegin.k;
	scan.vecScanDir[2] = ptEnd.l - ptBegin.l;
	scan.vecScanDir[3] = (ptEnd.E - ptBegin.E) / tl::get_one_meV<t_real_sc>();

	const t_real_sc dEps = 0.01;

	for(unsigned int i=0; i<4; ++i)
	{
		if(!tl::float_equal<t_real_sc>(scan.vecScanDir[i], 0., dEps))
		{
			scan.vecScanDir[i] /= scan.vecScanDir[i];
			scan.vecScanOrigin[i] = 0.;
		}
		else
		{
			scan.vecScanDir[i] = 0.;
		}
	}

	tl::log_info("Scan origin: (", scan.vecScanOrigin[0], " ", scan.vecScanOrigin[1], " ", scan.vecScanOrigin[2], " ", scan.vecScanOrigin[3], ").");
	tl::log_info("Scan dir: [", scan.vecScanDir[0], " ", scan.vecScanDir[1], " ", scan.vecScanDir[2], " ", scan.vecScanDir[3], "].");



	unsigned int iScIdx = 0;
	for(iScIdx=0; iScIdx<4; ++iScIdx)
		if(!tl::float_equal<t_real_sc>(scan.vecScanDir[iScIdx], 0., dEps))
			break;

	if(iScIdx >= 4)
	{
		tl::log_err("No scan variable found!");
		return false;
	}

	for(std::size_t iPt=0; iPt<iNumPts; ++iPt)
	{
		const ScanPoint& pt = scan.vecPoints[iPt];

		t_real_sc dPos[] = { pt.h, pt.k, pt.l, pt.E/tl::get_one_meV<t_real_sc>() };
		scan.vecX.push_back(dPos[iScIdx]);
		//tl::log_info("Added pos: ", *scan.vecX.rbegin());
	}



	// filter
	decltype(scan.vecX) vecXNew;
	decltype(scan.vecCts) vecCtsNew, vecMonNew, vecCtsErrNew, vecMonErrNew;
	for(std::size_t i=0; i<scan.vecX.size(); ++i)
	{
		if(filter.bLower && scan.vecX[i] <= filter.dLower) continue;
		if(filter.bUpper && scan.vecX[i] >= filter.dUpper) continue;

		vecXNew.push_back(scan.vecX[i]);
		vecCtsNew.push_back(scan.vecCts[i]);
		vecMonNew.push_back(scan.vecMon[i]);
		vecCtsErrNew.push_back(scan.vecCtsErr[i]);
		vecMonErrNew.push_back(scan.vecMonErr[i]);
	}
	scan.vecX = std::move(vecXNew);
	scan.vecCts = std::move(vecCtsNew);
	scan.vecMon = std::move(vecMonNew);
	scan.vecCtsErr = std::move(vecCtsErrNew);
	scan.vecMonErr = std::move(vecMonErrNew);


	return true;
}

bool load_file(const char* pcFile, Scan& scan, bool bNormToMon, const Filter& filter, bool bFlip)
{
	std::vector<std::string> vec{pcFile};
	return load_file(vec, scan, bNormToMon, filter, bFlip);
}
