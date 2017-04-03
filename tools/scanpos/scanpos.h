/**
 * Plot scattering plane and positions from given scan files
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 2016, 30-jan-2017
 * @license GPLv2
 */

#ifndef __SCANPOS_H__
#define __SCANPOS_H__

#include "tlibs/math/linalg.h"
#include "tlibs/math/linalg_ops.h"
#include "tlibs/file/loadinstr.h"
#include "libs/globals.h"
#include "tlibs/log/log.h"
#include "tlibs/version.h"

#include <vector>
#include <iostream>


/**
 * get plane position from hkl
 */
template<class t_vec, class t_real>
t_vec get_plane_coord(const t_vec& vec0, const t_vec& vec1, const t_vec& vecHKL)
{
	using namespace tl_ops;
	t_vec vecPos;

	vecPos.resize(2);
	vecPos[0] = vecHKL*vec0/std::pow(tl::ublas::norm_2(vec0), t_real(2));
	vecPos[1] = vecHKL*vec1/std::pow(tl::ublas::norm_2(vec1), t_real(2));

	return vecPos;
}

/**
 * transforms rlu into local <vec0, vec1> coordinates
 */
template<class t_vec, class t_real>
std::pair<t_vec, t_vec> get_coord(const t_vec& vec0, const t_vec& vec1, const tl::FileInstrBase<t_real>& scan)
{
	using namespace tl_ops;
	t_vec vecHKL, vecPos;

	std::size_t iNumPos = scan.GetScanCount();
	if(iNumPos)
	{
		// only use first position
		auto pos = scan.GetScanHKLKiKf(0);
		vecHKL = tl::make_vec<t_vec>({ pos[0], pos[1], pos[2] });
		vecPos = get_plane_coord<t_vec, t_real>(vec0, vec1, vecHKL);
	}

	return std::make_pair(vecHKL, vecPos);
}


/**
 * generate scattering plane plot script
 */
template<class t_vec, class t_real>
bool make_plot(std::ostream& ostr,
	const t_vec& vec0, const t_vec& vec1, const t_vec& vecBraggHKL,
	const std::vector<t_vec>& vecAllHKL, const std::vector<t_vec>& vecAllPos,
	bool bFlip = 1)
{
	using namespace tl_ops;
	std::ostream *pOstr = &ostr;

	if(!vecAllPos.size())
		return false;

	(*pOstr).precision(g_iPrec);
	(*pOstr) << "#!/usr/bin/gnuplot -p\n";
	(*pOstr) << "# Created with tlibs version " << TLIBS_VERSION << ".\n\n";

	(*pOstr) << "#set term pdf enhanced color font \"NimbusSanL-Regu,16\"\n";
	(*pOstr) << "#set output \"" << "scanpos.pdf\"\n\n";

	(*pOstr) << "col_bragg = \"#ff0000\"\n";
	(*pOstr) << "col_pos = \"#0000ff\"\n";
	(*pOstr) << "size_bragg = 2\n";
	(*pOstr) << "size_pos = 1\n\n";

	(*pOstr) << "unset key\n";
	(*pOstr) << "set size 1,1\n";
	(*pOstr) << "set xlabel \"[" << vec0[0] << ", " << vec0[1] << ", " << vec0[2] << "] (rlu)\"\n";
	(*pOstr) << "set ylabel \"[" << vec1[0] << ", " << vec1[1] << ", " << vec1[2] << "] (rlu)\"\n\n";

	(*pOstr) << "set xtics rotate 0.05\n";
	(*pOstr) << "set mxtics 2\n";
	(*pOstr) << "set ytics 0.05\n";
	(*pOstr) << "set mytics 2\n\n";


	t_vec vecBragg(2);
	vecBragg[0] = vecBraggHKL*vec0 / std::pow(tl::ublas::norm_2(vec0), t_real(2));
	vecBragg[1] = vecBraggHKL*vec1 / std::pow(tl::ublas::norm_2(vec1), t_real(2));


	// labels
	t_real dLabelPadX = std::abs(vecBragg[0]*0.0025);
	t_real dLabelPadY = std::abs(vecBragg[1]*0.0025);

	(*pOstr) << "set label 1"
		<< " at " << vecBragg[0]+dLabelPadX << "," << vecBragg[1]
		<< " \"(" << vecBraggHKL[0] << ", " << vecBraggHKL[1] << ", " << vecBraggHKL[2] << ")\""
		<< " \n";

	t_real xmin = vecBragg[0], xmax = vecBragg[0];
	t_real ymin = vecBragg[1], ymax = vecBragg[1];

	for(std::size_t iPos=0; iPos<vecAllPos.size(); ++iPos)
	{
		const t_vec& vecHKL = vecAllHKL[iPos];
		const t_vec& vecPos = vecAllPos[iPos];

		xmin = std::min(vecPos[0], xmin);
		ymin = std::min(vecPos[1], ymin);
		xmax = std::max(vecPos[0], xmax);
		ymax = std::max(vecPos[1], ymax);

		(*pOstr) << "set label " << (iPos+2)
			<< " at " << vecPos[0]+dLabelPadX << "," << vecPos[1]
			<< " \"(" << vecHKL[0] << ", " << vecHKL[1] << ", " << vecHKL[2] << ")\""
			<< " \n";
	}
	(*pOstr) << "\n";


	// ranges
	t_real xpad = (xmax-xmin)*0.1 + dLabelPadX*10.;
	t_real ypad = (ymax-ymin)*0.1 + dLabelPadY*10.;
	(*pOstr) << "xpad = " << xpad << "\n";
	(*pOstr) << "ypad = " << ypad << "\n";
	(*pOstr) << "set xrange [" << xmin << "-xpad" << " : " << xmax << "+xpad" << "]\n";
	if(bFlip)
		(*pOstr) << "set yrange [" << ymax << "+ypad" << " : " << ymin << "-ypad" << "]\n\n";
	else
		(*pOstr) << "set yrange [" << ymin << "-ypad" << " : " << ymax << "+ypad" << "]\n\n";


	(*pOstr) << "plot \\\n";
	(*pOstr) << "\t\"-\" u 1:2 w p pt 7 ps size_bragg lc rgb col_bragg, \\\n";
	(*pOstr) << "\t\"-\" u 1:2 w p pt 7 ps size_pos lc rgb col_pos\n";

	(*pOstr) << std::left << std::setw(g_iPrec*2) << vecBragg[0] << " " 
		<< std::left << std::setw(g_iPrec*2)<< vecBragg[1] 
		<< "\t# Bragg peak: G = (" 
		<< vecBraggHKL[0] << ", " << vecBraggHKL[1] << ", " << vecBraggHKL[2] << ")"
		<< "\ne\n";

	for(std::size_t iPos=0; iPos<vecAllPos.size(); ++iPos)
	{
		const t_vec& vecHKL = vecAllHKL[iPos];
		const t_vec& vecPos = vecAllPos[iPos];

		(*pOstr) << std::left << std::setw(g_iPrec*2) << vecPos[0] << " " 
			<< std::left << std::setw(g_iPrec*2) << vecPos[1]
			<< "\t# Q = (" << vecHKL[0] << ", " << vecHKL[1] << ", " << vecHKL[2] << ")\n";
	}
	(*pOstr) << "e\n";

	return true;
}

#endif
