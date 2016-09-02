/**
 * cooper-nathans calculation
 * @author tweber
 * @date 2013-2016
 * @license GPLv2
 *
 * @desc This is a reimplementation in C++ of the file rc_cnmat.m of the
 *		rescal5 package by Zinkin, McMorrow, Tennant, Farhi, and Wildes:
 *		http://www.ill.eu/en/instruments-support/computing-for-science/cs-software/all-software/matlab-ill/rescal-for-matlab/
 * @desc see: [cn67] M. J. Cooper and R. Nathans, Acta Cryst. 23, 357 (1967),
 * 		[ch73] N. J. Chesser and J. D. Axe, Acta Cryst. A 29, 160 (1973)
 */

#include "cn.h"
#include "ellipse.h"
#include "tlibs/math/linalg.h"
#include "tlibs/math/geo.h"
#include "tlibs/math/math.h"
#include "tlibs/log/log.h"

#include <string>
#include <iostream>


typedef t_real_reso t_real;
typedef ublas::matrix<t_real> t_mat;
typedef ublas::vector<t_real> t_vec;

using angle = tl::t_angle_si<t_real>;
using wavenumber = tl::t_wavenumber_si<t_real>;
using energy = tl::t_energy_si<t_real>;
using length = tl::t_length_si<t_real>;

static const auto angs = tl::get_one_angstrom<t_real>();
static const auto rads = tl::get_one_radian<t_real>();
static const auto meV = tl::get_one_meV<t_real>();
static const auto sec = tl::get_one_second<t_real>();
static const t_real pi = tl::get_pi<t_real>();
static const auto mn = tl::get_m_n<t_real>();
static const auto hbar = tl::get_hbar<t_real>();


// -----------------------------------------------------------------------------
// scattering factors

std::tuple<t_real, t_real> get_scatter_factors(std::size_t flags,
	const angle& thetam, const wavenumber& ki,
	const angle& thetaa, const wavenumber& kf)
{
	t_real dmono = t_real(1);
	t_real dana = t_real(1);

	if(flags & CALC_KI3)
		dmono *= tl::ana_effic_factor(ki, units::abs(thetam));
	if(flags & CALC_KF3)
		dana *= tl::ana_effic_factor(kf, units::abs(thetaa));

	return std::make_tuple(dmono, dana);
}


// -----------------------------------------------------------------------------
// R0 factor from formula (2) in [ch73]

t_real R0_P(angle theta, angle coll, angle mosaic)
{
	t_real tS = units::sin(theta);
	return std::sqrt(t_real(2)*pi) / rads *
		tl::my_units_sqrt<angle>(t_real(1) / (
		t_real(1)/(coll*coll) + t_real(1)/(t_real(4)*mosaic*mosaic*tS*tS)));
}

t_real R0_N(angle theta, angle mosaic, t_real refl)
{
	t_real tS = units::sin(theta);
	return (refl / (t_real(2)*mosaic * tS)) / std::sqrt(t_real(2)*pi) * rads;
}

t_real R0_J(wavenumber ki, wavenumber kf, angle twotheta)
{
	t_real tS = units::sin(twotheta);
	return mn/hbar / (ki*ki * kf*kf*kf * tS) / angs/angs/angs/sec;
}

t_real chess_R0(wavenumber ki, wavenumber kf,
	angle theta_m, angle theta_a, angle twotheta_s,
	angle mos_m, angle mos_a, angle coll_pre_mono_v, angle coll_post_ana_v,
	t_real refl_m, t_real refl_a)
{
	return R0_J(ki, kf, twotheta_s) *
		R0_P(theta_m, coll_pre_mono_v, mos_m) * R0_P(theta_a, coll_post_ana_v, mos_a) *
		R0_N(theta_m, mos_m, refl_m) * R0_N(theta_a, mos_a, refl_a);
}
// -----------------------------------------------------------------------------


ResoResults calc_cn(const CNParams& cn)
{
	ResoResults res;

	res.Q_avg.resize(4);
	res.Q_avg[0] = cn.Q * angs;
	res.Q_avg[1] = 0.;
	res.Q_avg[2] = 0.;
	res.Q_avg[3] = cn.E / meV;

	angle coll_h_pre_mono = cn.coll_h_pre_mono;
	angle coll_v_pre_mono = cn.coll_v_pre_mono;

/*
	const length lam = tl::k2lam(cn.ki);

	if(cn.bGuide)
	{
		coll_h_pre_mono = lam*(cn.guide_div_h/angs);
		coll_v_pre_mono = lam*(cn.guide_div_v/angs);
	}
*/

	// -------------------------------------------------------------------------
	// transformation matrix

	angle thetaa = cn.thetaa * cn.dana_sense;
	angle thetam = cn.thetam * cn.dmono_sense;
	angle ki_Q = cn.angle_ki_Q;
	angle kf_Q = cn.angle_kf_Q;

	ki_Q *= cn.dsample_sense;
	kf_Q *= cn.dsample_sense;

	t_mat Ti = tl::rotation_matrix_2d(ki_Q/rads);
	t_mat Tf = -tl::rotation_matrix_2d(kf_Q/rads);

	t_mat U = ublas::zero_matrix<t_real>(6,6);
	tl::submatrix_copy(U, Ti, 0, 0);
	tl::submatrix_copy(U, Tf, 0, 3);
	U(2,2) = 1.; U(2,5) = -1.;
	U(3,0) = +t_real(2)*cn.ki * tl::get_KSQ2E<t_real>() * angs;
	U(3,3) = -t_real(2)*cn.kf * tl::get_KSQ2E<t_real>() * angs;
	U(4,0) = 1.; U(5,2) = 1.;
	//tl::log_info("Trafo matrix (CN) = ", U);

	t_mat V(6,6);
	if(!tl::inverse(U, V))
	{
		res.bOk = false;
		res.strErr = "Transformation matrix cannot be inverted.";
		return res;
	}

	// -------------------------------------------------------------------------

	const auto tupScFact = get_scatter_factors(cn.flags, cn.thetam, cn.ki, cn.thetaa, cn.kf);

	t_real dmono_refl = cn.dmono_refl * std::get<0>(tupScFact);
	t_real dana_effic = cn.dana_effic * std::get<1>(tupScFact);

	// -------------------------------------------------------------------------
	// resolution matrix

	t_vec pm(2);
	pm[0] = units::tan(thetam);
	pm[1] = 1.;
	pm /= cn.ki*angs * cn.mono_mosaic/rads;

	t_vec pa(2);
	pa[0] = -units::tan(thetaa);
	pa[1] = 1.;
	pa /= cn.kf*angs * cn.ana_mosaic/rads;

	t_vec palf0(2);
	palf0[0] = 2.*units::tan(thetam);
	palf0[1] = 1.;
	palf0 /= (cn.ki*angs * coll_h_pre_mono/rads);

	t_vec palf1(2);
	palf1[0] = 0;
	palf1[1] = 1.;
	palf1 /= (cn.ki*angs * cn.coll_h_pre_sample/rads);

	t_vec palf2(2);
	palf2[0] = -2.*units::tan(thetaa);
	palf2[1] = 1.;
	palf2 /= (cn.kf*angs * cn.coll_h_post_ana/rads);

	t_vec palf3(2);
	palf3[0] = 0;
	palf3[1] = 1.;
	palf3 /= (cn.kf*angs * cn.coll_h_post_sample/rads);

	t_mat m01(2,2);
	m01 = ublas::outer_prod(pm,pm) +
		ublas::outer_prod(palf0,palf0) +
		ublas::outer_prod(palf1,palf1);
	t_mat m34(2,2);
	m34 = ublas::outer_prod(pa,pa) +
		ublas::outer_prod(palf2,palf2) +
		ublas::outer_prod(palf3,palf3);

	t_mat M = ublas::zero_matrix<t_real>(6,6);
	tl::submatrix_copy(M, m01, 0, 0);
	tl::submatrix_copy(M, m34, 3, 3);

	M(2,2) = t_real(1)/(cn.ki*cn.ki * angs*angs) * rads*rads *
	(
		t_real(1)/(cn.coll_v_pre_sample*cn.coll_v_pre_sample) +
		t_real(1)/((t_real(2)*units::sin(thetam)*cn.mono_mosaic)*
			(t_real(2)*units::sin(thetam)*cn.mono_mosaic) +
			coll_v_pre_mono*coll_v_pre_mono)
	);
	M(5,5) = t_real(1)/(cn.kf*cn.kf * angs*angs) * rads*rads *
	(
		t_real(1) / (cn.coll_v_post_sample*cn.coll_v_post_sample) +
		t_real(1) / ((t_real(2)*units::sin(thetaa)*cn.ana_mosaic)*
			(t_real(2)*units::sin(thetaa)*cn.ana_mosaic) +
			cn.coll_v_post_ana*cn.coll_v_post_ana)
	);
	// -------------------------------------------------------------------------

	t_mat N = tl::transform(M, V, 1);

	N = ellipsoid_gauss_int(N, 5);
	N = ellipsoid_gauss_int(N, 4);

	t_vec vec1 = tl::get_column<t_vec>(N, 1);
	res.reso = N - ublas::outer_prod(vec1,vec1)
		/ (1./((cn.sample_mosaic/rads * cn.Q*angs)
		* (cn.sample_mosaic/rads * cn.Q*angs)) + N(1,1));
	res.reso(2,2) = N(2,2);
	res.reso *= tl::get_SIGMA2FWHM<t_real>()*tl::get_SIGMA2FWHM<t_real>();

	res.reso_v = ublas::zero_vector<t_real>(4);
	res.reso_s = 0.;

	if(cn.dsample_sense < 0.)
	{
		// mirror Q_perp
		t_mat matMirror = tl::mirror_matrix<t_mat>(res.reso.size1(), 1);
		res.reso = tl::transform(res.reso, matMirror, true);
		res.reso_v[1] = -res.reso_v[1];
	}

	// -------------------------------------------------------------------------


	res.dResVol = tl::get_ellipsoid_volume(res.reso);
	res.dR0 = chess_R0(cn.ki,cn.kf, thetam, thetaa, cn.twotheta, cn.mono_mosaic,
		cn.ana_mosaic, cn.coll_v_pre_mono, cn.coll_v_post_ana, dmono_refl, dana_effic);

	// Bragg widths
	for(unsigned int i=0; i<4; ++i)
		res.dBraggFWHMs[i] = tl::get_SIGMA2FWHM<t_real>()/sqrt(res.reso(i,i));

	if(tl::is_nan_or_inf(res.dR0) || tl::is_nan_or_inf(res.reso))
	{
		res.strErr = "Invalid result.";
		res.bOk = false;
		return res;
	}

	res.bOk = true;
	return res;
}
