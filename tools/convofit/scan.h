/**
 * Convolution fitting
 * @author tweber
 * @date dec-2015
 * @license GPLv2
 */

#ifndef __CONVOFIT_SCAN_H__
#define __CONVOFIT_SCAN_H__

#include <vector>
#include <string>

#include "tlibs/math/math.h"
#include "tlibs/math/neutrons.h"
#include "tlibs/file/loadinstr.h"

//#include "tlibs/fit/minuit.h"	// only to get intrinsic real type
//using t_real_sc = tl::t_real_min;

#include "../res/defs.h"
using t_real_sc = t_real_reso;


struct ScanPoint
{
	t_real_sc h, k, l;
	tl::t_wavenumber_si<t_real_sc> ki, kf;
	tl::t_energy_si<t_real_sc> Ei, Ef, E;
};

struct Sample
{
	t_real_sc a, b, c;
	t_real_sc alpha, beta, gamma;
};

struct Plane
{
	t_real_sc vec1[3];
	t_real_sc vec2[3];
};


struct Filter
{
	bool bLower = 0;
	bool bUpper = 0;

	t_real_sc dLower = 0.;
	t_real_sc dUpper = 0.;
};


struct Scan
{
	Sample sample;
	Plane plane;
	bool bKiFixed=0;
	t_real_sc dKFix = 2.662;

	std::string strTempCol = "TT";
	t_real_sc dTemp = 100., dTempErr=0.;

	std::string strFieldCol = "";
	t_real_sc dField = 0., dFieldErr=0.;

	std::string strCntCol = "";
	std::string strMonCol = "";
	std::vector<ScanPoint> vecPoints;

	std::vector<t_real_sc> vecX;
	std::vector<t_real_sc> vecCts, vecMon;
	std::vector<t_real_sc> vecCtsErr, vecMonErr;

	t_real_sc vecScanOrigin[4];
	t_real_sc vecScanDir[4];


	ScanPoint InterpPoint(std::size_t i, std::size_t N) const
	{
		const ScanPoint& ptBegin = *vecPoints.cbegin();
		const ScanPoint& ptEnd = *vecPoints.crbegin();

		ScanPoint pt;

		pt.h = tl::lerp(ptBegin.h, ptEnd.h, t_real_sc(i)/t_real_sc(N-1));
		pt.k = tl::lerp(ptBegin.k, ptEnd.k, t_real_sc(i)/t_real_sc(N-1));
		pt.l = tl::lerp(ptBegin.l, ptEnd.l, t_real_sc(i)/t_real_sc(N-1));
		pt.E = tl::lerp(ptBegin.E, ptEnd.E, t_real_sc(i)/t_real_sc(N-1));
		pt.Ei = tl::lerp(ptBegin.Ei, ptEnd.Ei, t_real_sc(i)/t_real_sc(N-1));
		pt.Ef = tl::lerp(ptBegin.Ef, ptEnd.Ef, t_real_sc(i)/t_real_sc(N-1));
		bool bImag=0;
		pt.ki = tl::E2k(pt.Ei, bImag);
		pt.kf = tl::E2k(pt.Ef, bImag);

		return pt;
	}
};


extern bool load_file(const std::vector<std::string>& vecFiles, Scan& scan,
	bool bNormToMon=1, const Filter& filter = Filter(), bool bFlipCoords=0);
extern bool load_file(const char* pcFile, Scan& scan,
	bool bNormToMon=1, const Filter& filter=Filter(), bool bFlipCoords=0);
extern bool save_file(const char* pcFile, const Scan& sc);


#endif
