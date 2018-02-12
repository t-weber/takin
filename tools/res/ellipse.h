/**
 * resolution ellipse calculation
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 14-may-2013
 * @license GPLv2
 *
 * @desc This is a reimplementation in C++ of the files rc_projs.m and rc_int.m of the
 *	- rescal5 package by Zinkin, McMorrow, Tennant, Farhi, and Wildes:
 *	  http://www.ill.eu/en/instruments-support/computing-for-science/cs-software/all-software/matlab-ill/rescal-for-matlab/
 *  - and the 'mcresplot.pl' program from McStas (www.mcstas.org)
 *  - see also: [eck14] G. Eckold and O. Sobolev, NIM A 752, pp. 54-64 (2014)
 */

#ifndef __RES_ELLIPSE__
#define __RES_ELLIPSE__

#include <string>
#include <ostream>
#include <cmath>
#include <vector>
#include <utility>

#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/matrix.hpp>
namespace ublas = boost::numeric::ublas;

#include "tlibs/math/geo.h"
#include "tlibs/math/linalg.h"
#include "tlibs/math/linalg2.h"
#include "tlibs/math/quat.h"
#include "tlibs/math/math.h"
#include "defs.h"


template<class t_real = t_real_reso>
struct Ellipse2d
{
	tl::QuadEllipsoid<t_real> quad;
	ublas::matrix<t_real> rot;

	t_real phi, slope;
	t_real x_hwhm, y_hwhm;
	t_real x_hwhm_bound, y_hwhm_bound;
	t_real x_offs, y_offs;
	t_real area;

	std::string x_lab, y_lab;

	ublas::vector<t_real> operator()(t_real t, bool bOffs=true) const;
	void GetCurvePoints(std::vector<t_real>& x, std::vector<t_real>& y,
		std::size_t iPoints=512, t_real *pLRTB=0);
};

template<class t_real = t_real_reso>
struct Ellipsoid3d
{
	tl::QuadEllipsoid<t_real> quad;
	ublas::matrix<t_real> rot;

	t_real x_hwhm, y_hwhm, z_hwhm;
	t_real x_offs, y_offs, z_offs;
	t_real vol;

	std::string x_lab, y_lab, z_lab;
};

template<class t_real = t_real_reso>
struct Ellipsoid4d
{
	tl::QuadEllipsoid<t_real> quad;
	ublas::matrix<t_real> rot;

	t_real x_hwhm, y_hwhm, z_hwhm, w_hwhm;
	t_real x_offs, y_offs, z_offs, w_offs;
	t_real vol;

	std::string x_lab, y_lab, z_lab, w_lab;
};


enum class EllipseCoordSys : int
{
	AUTO = -1,

	Q_AVG = 0,		// Q|| Qperp system (1/A)
	RLU = 1,		// absolute hkl system (rlu)
	RLU_ORIENT = 2	// system using scattering plane (rlu)
};



// --------------------------------------------------------------------------------


/**
 * Integration of the quadratic part of the quadric
 * this is a 1:1 C++ reimplementation of 'rc_int' from 'mcresplot' and 'rescal5'
 * (see also [eck14], equ. 57)
 * integrate over row/column iIdx
 */
template<class T = t_real_reso>
ublas::matrix<T> ellipsoid_gauss_int(const ublas::matrix<T>& mat, std::size_t iIdx)
{
	ublas::vector<T> b = T(0.5)*(tl::get_column(mat, iIdx) + tl::get_row(mat, iIdx));
	b = tl::remove_elem(b, iIdx);

	ublas::matrix<T> m = tl::remove_elems(mat, iIdx);
	m -= ublas::outer_prod(b,b) / mat(iIdx, iIdx);

	return m;
}

/**
 * Integration of the linear part of the quadric
 * (see [eck14], equ. 57)
 */
template<class T = t_real_reso>
ublas::vector<T> ellipsoid_gauss_int(const ublas::vector<T>& vec,
	const ublas::matrix<T>& mat, std::size_t iIdx)
{
	ublas::vector<T> b = tl::get_column(mat, iIdx);
	b = tl::remove_elem(b, iIdx);

	ublas::vector<T> vecInt = tl::remove_elem(vec, iIdx);
	vecInt -= b*vec[iIdx] / mat(iIdx, iIdx);

	return vecInt;
}


// --------------------------------------------------------------------------------

template<class t_real = t_real_reso>
std::ostream& operator<<(std::ostream& ostr, const Ellipse2d<t_real>& ell)
{
	ostr << "phi = " << tl::r2d(ell.phi) << " deg \n";
	ostr << "slope = " << ell.slope << " deg \n";
	ostr << "x_hwhm = " << ell.x_hwhm << ", ";
	ostr << "y_hwhm = " << ell.y_hwhm << "\n";
	ostr << "x_hwhm_bound = " << ell.x_hwhm_bound << ", ";
	ostr << "y_hwhm_bound = " << ell.y_hwhm_bound << "\n";
	ostr << "x_offs = " << ell.x_offs << ", ";
	ostr << "y_offs = " << ell.y_offs << "\n";
	ostr << "x_lab = " << ell.x_lab << ", ";
	ostr << "y_lab = " << ell.y_lab << "\n";
	ostr << "area = " << ell.area;

	return ostr;
}

template<class t_real = t_real_reso>
std::ostream& operator<<(std::ostream& ostr, const Ellipsoid4d<t_real>& ell)
{
	ostr << "x_hwhm = " << ell.x_hwhm << ", ";
	ostr << "y_hwhm = " << ell.y_hwhm << ", ";
	ostr << "z_hwhm = " << ell.z_hwhm << ", ";
	ostr << "w_hwhm = " << ell.w_hwhm << "\n";
	ostr << "x_offs = " << ell.x_offs << ", ";
	ostr << "y_offs = " << ell.y_offs << ", ";
	ostr << "z_offs = " << ell.z_offs << ", ";
	ostr << "w_offs = " << ell.w_offs << "\n";
	ostr << "x_lab = " << ell.x_lab << ", ";
	ostr << "y_lab = " << ell.y_lab << ", ";
	ostr << "z_lab = " << ell.z_lab << ", ";
	ostr << "w_lab = " << ell.w_lab << "\n";
	ostr << "volume = " << ell.vol;

	return ostr;
}

// --------------------------------------------------------------------------------

template<class t_real>
ublas::vector<t_real> Ellipse2d<t_real>::operator()(t_real t, bool bAddOffs/*=1*/) const
{
	ublas::vector<t_real> vec = tl::make_vec<ublas::vector<t_real>>
		({ x_hwhm * std::cos(t_real(2)*tl::get_pi<t_real>()*t),
			y_hwhm * std::sin(t_real(2)*tl::get_pi<t_real>()*t) });

	vec = ublas::prod(rot, vec);

	if(bAddOffs)
	{
		vec[0] += x_offs;
		vec[1] += y_offs;
	}

	return vec;
}

template<class t_real>
void Ellipse2d<t_real>::GetCurvePoints(std::vector<t_real>& x, std::vector<t_real>& y,
	std::size_t iPoints, t_real *pLRTB)
{
	x.resize(iPoints);
	y.resize(iPoints);

	for(std::size_t i=0; i<iPoints; ++i)
	{
		t_real dT = t_real(i)/t_real(iPoints-1);
		ublas::vector<t_real> vec = operator()(dT);

		x[i] = vec[0];
		y[i] = vec[1];
	}

	if(pLRTB)	// bounding rect
	{
		auto pairX = std::minmax_element(x.begin(), x.end());
		auto pairY = std::minmax_element(y.begin(), y.end());

		*(pLRTB+0) = *pairX.first;	// left
		*(pLRTB+1) = *pairX.second;	// right
		*(pLRTB+2) = *pairY.second;	// top
		*(pLRTB+3) = *pairY.first;	// bottom
	}
}

// --------------------------------------------------------------------------------

template<class T = t_real_reso>
static void elli_gauss_int(tl::QuadEllipsoid<T>& quad, std::size_t iIdx)
{
	//tl::log_debug("before int: ", quad.GetR());

	ublas::vector<T> vecRint = ellipsoid_gauss_int(quad.GetR(), quad.GetQ(), iIdx);
	ublas::matrix<T> matQint = ellipsoid_gauss_int(quad.GetQ(), iIdx);
	quad.RemoveElems(iIdx);
	quad.SetQ(matQint);
	quad.SetR(vecRint);

	//tl::log_debug("after int: ", quad.GetR());
}


static const std::string g_strLabelsCentre[] = {"Q_{para}-<Q> (1/A)", "Q_{ortho}-<Q> (1/A)", "Q_z-<Q> (1/A)", "E (meV)"};
static const std::string g_strLabels[] = {"Q_{para} (1/A)", "Q_{ortho} (1/A)", "Q_z (1/A)", "E (meV)"};
static const std::string g_strLabelsHKLCentre[] = {"h-<h> (rlu)", "k-<k> (rlu)", "l-<l> (rlu)", "E (meV)"};
static const std::string g_strLabelsHKL[] = {"h (rlu)", "k (rlu)", "l (rlu)", "E (meV)"};
static const std::string g_strLabelsHKLOrient[] = {"Reflex 1 (rlu)", "Reflex 2 (rlu)", "Up (rlu)", "E (meV)"};

static inline const std::string& ellipse_labels(int iCoord, EllipseCoordSys sys, bool bCentre=0)
{
	switch(sys)
	{
		case EllipseCoordSys::RLU:
			if(bCentre)
				return g_strLabelsHKLCentre[iCoord];
			return g_strLabelsHKL[iCoord];
		case EllipseCoordSys::RLU_ORIENT:
			return g_strLabelsHKLOrient[iCoord];
		case EllipseCoordSys::AUTO:
		case EllipseCoordSys::Q_AVG:
		default:
			if(bCentre)
				return g_strLabelsCentre[iCoord];
			return g_strLabels[iCoord];
	}
}



/*
 * this is a 1:1 C++ reimplementation of 'proj_elip' from 'mcresplot' and 'rescal5'
 * iX, iY: dimensions to plot
 * iInt: dimension to integrate
 * iRem1, iRem2: dimensions to remove
 */
template<class t_real = t_real_reso>
Ellipse2d<t_real> calc_res_ellipse(
	const ublas::matrix<t_real>& reso,		// quadratic part of quadric
	const ublas::vector<t_real>& reso_vec,	// linear part
	t_real reso_const,						// const part
	const ublas::vector<t_real>& Q_avg,
	int iX, int iY, int iInt, int iRem1, int iRem2)
{
	Ellipse2d<t_real> ell;
	ell.quad.SetDim(4);
	ell.quad.SetQ(reso);
	ell.quad.SetR(reso_vec);
	//ell.quad.SetS(reso_const);

	ell.x_offs = ell.y_offs = 0.;

	// labels only valid for non-rotated system
	ell.x_lab = g_strLabels[iX];
	ell.y_lab = g_strLabels[iY];


	ublas::vector<t_real> Q_offs = Q_avg;

	if(iRem1>-1)
	{
		ell.quad.RemoveElems(iRem1);
		Q_offs = tl::remove_elem(Q_offs, iRem1);

		if(iInt>=iRem1) --iInt;
		if(iRem2>=iRem1) --iRem2;
		if(iX>=iRem1) --iX;
		if(iY>=iRem1) --iY;
	}

	if(iRem2>-1)
	{
		ell.quad.RemoveElems(iRem2);
		Q_offs = tl::remove_elem(Q_offs, iRem2);

		if(iInt>=iRem2) --iInt;
		if(iX>=iRem2) --iX;
		if(iY>=iRem2) --iY;
	}

	if(iInt>-1)
	{
		elli_gauss_int(ell.quad, iInt);
		Q_offs = tl::remove_elem(Q_offs, iInt);

		if(iX>=iInt) --iX;
		if(iY>=iInt) --iY;
	}

	std::vector<t_real> evals;

	tl::QuadEllipsoid<t_real> quad(2);
	ell.quad.GetPrincipalAxes(ell.rot, evals, &quad);
	//tl::log_debug("old: ", ell.quad.GetR(), ", new: ", quad.GetR());

	ell.phi = tl::rotation_angle(ell.rot)[0];

	// test: set rotation directly
	//ublas::matrix<t_real> matEvecs = tl::rotation_matrix_2d(ell.phi);
	//quad.SetR(ublas::prod(matEvecs, ell.quad.GetR()));

	ell.x_hwhm = tl::get_SIGMA2HWHM<t_real>() * quad.GetRadius(0);
	ell.y_hwhm = tl::get_SIGMA2HWHM<t_real>() * quad.GetRadius(1);

	ell.x_offs = Q_offs[iX];
	ell.y_offs = Q_offs[iY];


	// calculate bounding rect
	std::tie(ell.x_hwhm_bound, ell.y_hwhm_bound)
		= [&ell]() -> std::pair<t_real, t_real>
	{
		t_real t0deg = ell.phi / (tl::get_pi<t_real>()*t_real(2));
		t_real t90deg = (ell.phi + tl::get_pi<t_real>()/t_real(2))
			/ (tl::get_pi<t_real>()*t_real(2));

		ublas::vector<t_real> vec1 = ell(t0deg, false);
		ublas::vector<t_real> vec2 = ell(t90deg, false);

		t_real dX = std::max(std::abs(vec1[0]), std::abs(vec2[0]));
		t_real dY = std::max(std::abs(vec1[1]), std::abs(vec2[1]));

		return std::pair<t_real, t_real>(dX, dY);
	}();


	// linear part of quadric
	const ublas::vector<t_real> vecTrans = ublas::prod(ell.rot, quad.GetPrincipalOffset());

	if(vecTrans.size() == 2)
	{
		ell.x_offs += vecTrans[0];
		ell.y_offs += vecTrans[1];
	}
	else
	{
		tl::log_err("Invalid ellipse shift.");
	}

	ell.area = quad.GetVolume();
	ell.slope = std::tan(ell.phi);


#ifndef NDEBUG
	// sanity check, see Shirane p. 267
	ublas::matrix<t_real> res_mat0 = ell.quad.GetQ();
	t_real dMyPhi = tl::r2d(ell.phi);
	t_real dPhiShirane = tl::r2d(t_real(0.5)*atan(2.*res_mat0(0,1) / (res_mat0(0,0)-res_mat0(1,1))));
	if(!tl::float_equal(dMyPhi, dPhiShirane, 0.01)
		&& !tl::float_equal(dMyPhi-90., dPhiShirane, 0.01))
	{
		tl::log_warn("Calculated ellipse phi = ", dMyPhi, " deg",
			" deviates from theoretical phi = ", dPhiShirane, " deg.");
	}
#endif

	return ell;
}


/**
 * incoherent (vanadium) widths
 */
template<class t_real = t_real_reso>
std::tuple<t_real, t_real> calc_vanadium_fwhms(
	const ublas::matrix<t_real>& matReso,
	const ublas::vector<t_real>& vecReso,
	const t_real dScalarReso,
	const ublas::vector<t_real>& vecQAvg)
{
	struct Ellipse2d<t_real> ellVa =
		calc_res_ellipse<t_real>(matReso, vecReso, dScalarReso,
		vecQAvg, 0, 3, 1, 2, -1);

	t_real dVanadiumFWHM_Q = ellVa.x_hwhm_bound*2.;
	t_real dVanadiumFWHM_E = ellVa.y_hwhm_bound*2.;

	return std::make_tuple(dVanadiumFWHM_Q, dVanadiumFWHM_E);
}

// --------------------------------------------------------------------------------

template<class t_real = t_real_reso>
Ellipsoid3d<t_real> calc_res_ellipsoid(
	const ublas::matrix<t_real>& reso,
	const ublas::vector<t_real>& reso_vec,
	t_real reso_const,
	const ublas::vector<t_real>& Q_avg,
	int iX, int iY, int iZ, int iInt, int iRem)
{
	Ellipsoid3d<t_real> ell;

	ell.quad.SetDim(4);
	ell.quad.SetQ(reso);
	ell.quad.SetR(reso_vec);
	//ell.quad.SetS(reso_const);

	ell.x_offs = ell.y_offs = ell.z_offs = 0.;

	// labels only valid for non-rotated system
	ell.x_lab = g_strLabels[iX];
	ell.y_lab = g_strLabels[iY];
	ell.z_lab = g_strLabels[iZ];

	ublas::vector<t_real> Q_offs = Q_avg;

	if(iRem>-1)
	{
		ell.quad.RemoveElems(iRem);
		Q_offs = tl::remove_elem(Q_offs, iRem);

		if(iInt>=iRem) --iInt;
		if(iX>=iRem) --iX;
		if(iY>=iRem) --iY;
		if(iZ>=iRem) --iZ;
	}

	if(iInt>-1)
	{
		elli_gauss_int(ell.quad, iInt);
		Q_offs = tl::remove_elem(Q_offs, iInt);

		if(iX>=iInt) --iX;
		if(iY>=iInt) --iY;
		if(iZ>=iInt) --iZ;
	}

	std::vector<t_real> evals;
	tl::QuadEllipsoid<t_real> quad(3);
	ell.quad.GetPrincipalAxes(ell.rot, evals, &quad);

	//tl::log_info("Principal axes: ", quad.GetQ());
	ell.x_hwhm = tl::get_SIGMA2HWHM<t_real>() * quad.GetRadius(0);
	ell.y_hwhm = tl::get_SIGMA2HWHM<t_real>() * quad.GetRadius(1);
	ell.z_hwhm = tl::get_SIGMA2HWHM<t_real>() * quad.GetRadius(2);

	ell.x_offs = Q_offs[iX];
	ell.y_offs = Q_offs[iY];
	ell.z_offs = Q_offs[iZ];

	// linear part of quadric
	const ublas::vector<t_real> vecTrans = ublas::prod(ell.rot, quad.GetPrincipalOffset());

	if(vecTrans.size() == 3)
	{
		ell.x_offs += vecTrans[0];
		ell.y_offs += vecTrans[1];
		ell.z_offs += vecTrans[2];
	}
	else
	{
		tl::log_err("Invalid ellipsoid shift.");
	}

	ell.vol = quad.GetVolume();
	return ell;
}

// --------------------------------------------------------------------------------

template<class t_real = t_real_reso>
Ellipsoid4d<t_real> calc_res_ellipsoid4d(
	const ublas::matrix<t_real>& reso,
	const ublas::vector<t_real>& reso_vec,
	t_real reso_const,
	const ublas::vector<t_real>& Q_avg)
{
	Ellipsoid4d<t_real> ell;
	ell.quad.SetDim(4);
	ell.quad.SetQ(reso);
	ell.quad.SetR(reso_vec);
	//ell.quad.SetS(reso_const);

	std::vector<t_real> evals;
	tl::QuadEllipsoid<t_real> quad(4);
	ell.quad.GetPrincipalAxes(ell.rot, evals, &quad);

	ell.x_hwhm = tl::get_SIGMA2HWHM<t_real>() * quad.GetRadius(0);
	ell.y_hwhm = tl::get_SIGMA2HWHM<t_real>() * quad.GetRadius(1);
	ell.z_hwhm = tl::get_SIGMA2HWHM<t_real>() * quad.GetRadius(2);
	ell.w_hwhm = tl::get_SIGMA2HWHM<t_real>() * quad.GetRadius(3);

	ell.x_offs = Q_avg[0];
	ell.y_offs = Q_avg[1];
	ell.z_offs = Q_avg[2];
	ell.w_offs = Q_avg[3];

	// linear part of quadric
	const ublas::vector<t_real> vecTrans = ublas::prod(ell.rot, quad.GetPrincipalOffset());

	if(vecTrans.size() == 4)
	{
		ell.x_offs += vecTrans[0];
		ell.y_offs += vecTrans[1];
		ell.z_offs += vecTrans[2];
		ell.w_offs += vecTrans[3];
	}
	else
	{
		tl::log_err("Invalid ellipsoid shift.");
	}

	// labels only valid for non-rotated system
	ell.x_lab = g_strLabels[0];
	ell.y_lab = g_strLabels[1];
	ell.z_lab = g_strLabels[2];
	ell.w_lab = g_strLabels[3];

	ell.vol = quad.GetVolume();

	return ell;
}

#endif
