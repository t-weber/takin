/**
 * cooper-nathans calculation
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 2013-2016
 * @license GPLv2
 *
 * @desc This is a reimplementation in C++ of the file rc_cnmat.m of the
 *		rescal5 package by Zinkin, McMorrow, Tennant, Farhi, and Wildes:
 *		http://www.ill.eu/en/instruments-support/computing-for-science/cs-software/all-software/matlab-ill/rescal-for-matlab/
 * @desc see: [cn67] M. J. Cooper and R. Nathans, Acta Cryst. 23, 357 (1967)
 *		[ch73] N. J. Chesser and J. D. Axe, Acta Cryst. A 29, 160 (1973)
 *		[mit84] P. W. Mitchell, R. A. Cowley and S. A. Higgins, Acta Cryst. Sec A, 40(2), 152-160 (1984)
 */

#include "cn.h"
#include "r0.h"
#include "ellipse.h"
#include "helper.h"

#include "tlibs/math/geo.h"
#include "tlibs/math/math.h"
#include "tlibs/log/log.h"

#include <string>
#include <future>
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
static const auto mn = tl::get_m_n<t_real>();
static const auto hbar = tl::get_hbar<t_real>();
static const t_real pi = tl::get_pi<t_real>();
static const t_real sig2fwhm = tl::get_SIGMA2FWHM<t_real>();


/**
 * transformation matrix -> [mit84], equ. A.15
 *
 * (  Ti11   Ti12      0   Tf11   Tf12      0 )   ( dki_x )   ( dQ_x  )
 * (  Ti12   Ti22      0   Tf12   Tf22      0 )   ( dki_y )   ( dQ_y  )
 * (     0      0      1      0      0     -1 ) * ( dki_z ) = ( dQ_z  )
 * ( 2ki*c      0      0 -2kf*c      0      0 )   ( dkf_x )   ( dE    )
 * (     1      0      0      0      0      0 )   ( dkf_y )   ( dki_x )
 * (     0      0      1      0      0      0 )   ( dkf_z )   ( dki_z )
 *
 * e.g. E ~ ki^2 - kf^2
 * dE ~ 2ki*dki - 2kf*dkf
 */
t_mat get_trafo_dkidkf_dQdE(const angle& ki_Q, const angle& kf_Q,
	const wavenumber& ki, const wavenumber& kf)
{
	t_mat Ti = tl::rotation_matrix_2d(ki_Q/rads);
	t_mat Tf = -tl::rotation_matrix_2d(kf_Q/rads);

	t_mat U = ublas::zero_matrix<t_real>(6,6);
	tl::submatrix_copy(U, Ti, 0, 0);
	tl::submatrix_copy(U, Tf, 0, 3);
	U(2,2) = 1.; U(2,5) = -1.;
	U(3,0) = +t_real(2)*ki * tl::get_KSQ2E<t_real>() * angs;
	U(3,3) = -t_real(2)*kf * tl::get_KSQ2E<t_real>() * angs;
	U(4,0) = 1.; U(5,2) = 1.;
	//tl::log_info("Trafo matrix (CN) = ", U);

	return U;
}


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

	// use the same as the horizontal mosaics for now
	angle mono_mosaic_v = cn.mono_mosaic;
	angle ana_mosaic_v = cn.ana_mosaic;

	/*const length lam = tl::k2lam(cn.ki);
	if(cn.bGuide)
	{
		coll_h_pre_mono = lam*(cn.guide_div_h/angs);
		coll_v_pre_mono = lam*(cn.guide_div_v/angs);
	}*/

	angle thetaa = cn.thetaa * cn.dana_sense;
	angle thetam = cn.thetam * cn.dmono_sense;
	angle ki_Q = cn.angle_ki_Q;
	angle kf_Q = cn.angle_kf_Q;

	ki_Q *= cn.dsample_sense;
	kf_Q *= cn.dsample_sense;

	t_mat U = get_trafo_dkidkf_dQdE(ki_Q, kf_Q, cn.ki, cn.kf);

	// V matrix -> [mit84], equ. A.16
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
	if(cn.mono_refl_curve) dmono_refl *= (*cn.mono_refl_curve)(cn.ki);
	if(cn.ana_effic_curve) dana_effic *= (*cn.ana_effic_curve)(cn.kf);
	t_real dxsec = std::get<2>(tupScFact);


	// -------------------------------------------------------------------------
	// resolution matrix, [mit84], equ. A.5
	t_mat M = ublas::zero_matrix<t_real>(6,6);

	auto calc_mono_ana_res =
		[](angle theta, wavenumber k,
		angle mosaic, angle mosaic_v,
		angle coll1, angle coll2,
		angle coll1_v, angle coll2_v) -> std::pair<t_mat, t_real>
	{
		// horizontal part
		t_vec vecMos(2);
		vecMos[0] = units::tan(theta);
		vecMos[1] = 1.;
		vecMos /= k*angs * mosaic/rads;

		t_vec vecColl1(2);
		vecColl1[0] = 2.*units::tan(theta);
		vecColl1[1] = 1.;
		vecColl1 /= (k*angs * coll1/rads);

		t_vec vecColl2(2);
		vecColl2[0] = 0;
		vecColl2[1] = 1.;
		vecColl2 /= (k*angs * coll2/rads);

		t_mat matHori = ublas::outer_prod(vecMos, vecMos) +
			ublas::outer_prod(vecColl1, vecColl1) +
			ublas::outer_prod(vecColl2, vecColl2);

		// vertical part, [mit84], equ. A.9 & A.13
		t_real dVert = t_real(1)/(k*k * angs*angs) * rads*rads *
		(
			t_real(1) / (coll2_v * coll2_v) +
			t_real(1) / ((t_real(2)*units::sin(theta) * mosaic_v) *
				(t_real(2)*units::sin(theta) * mosaic_v) +
				coll1_v * coll1_v)
		);

		return std::pair<t_mat, t_real>(matHori, dVert);
	};

	std::launch lpol = /*std::launch::deferred |*/ std::launch::async;
	std::future<std::pair<t_mat, t_real>> futMono
		= std::async(lpol, calc_mono_ana_res,
			thetam, cn.ki,
			cn.mono_mosaic, mono_mosaic_v,
			cn.coll_h_pre_mono, cn.coll_h_pre_sample,
			cn.coll_v_pre_mono, cn.coll_v_pre_sample);
	std::future<std::pair<t_mat, t_real>> futAna
		= std::async(lpol, calc_mono_ana_res,
			-thetaa, cn.kf,
			cn.ana_mosaic, ana_mosaic_v,
			cn.coll_h_post_ana, cn.coll_h_post_sample,
			cn.coll_v_post_ana, cn.coll_v_post_sample);

	t_mat matMonoH, matAnaH;
	t_real dMonoV, dAnaV;
	std::tie(matMonoH, dMonoV) = futMono.get();
	std::tie(matAnaH, dAnaV) = futAna.get();

	tl::submatrix_copy(M, matMonoH, 0, 0);
	tl::submatrix_copy(M, matAnaH, 3, 3);
	M(2,2) = dMonoV;
	M(5,5) = dAnaV;
	// -------------------------------------------------------------------------


	t_mat N = tl::transform(M, V, 1);

	N = ellipsoid_gauss_int(N, 5);
	N = ellipsoid_gauss_int(N, 4);

	t_vec vec1 = tl::get_column<t_vec>(N, 1);
	res.reso = N - ublas::outer_prod(vec1,vec1)
		/ (1./((cn.sample_mosaic/rads * cn.Q*angs)
		* (cn.sample_mosaic/rads * cn.Q*angs)) + N(1,1));
	res.reso(2,2) = N(2,2);
	res.reso *= sig2fwhm*sig2fwhm;

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
	res.dR0 *= dxsec;

	// Bragg widths
	const std::vector<t_real> vecFwhms = calc_bragg_fwhms(res.reso);
	std::copy(vecFwhms.begin(), vecFwhms.end(), res.dBraggFWHMs);

	if(tl::is_nan_or_inf(res.dR0) || tl::is_nan_or_inf(res.reso))
	{
		res.strErr = "Invalid result.";
		res.bOk = false;
		return res;
	}

	res.bOk = true;
	return res;
}
