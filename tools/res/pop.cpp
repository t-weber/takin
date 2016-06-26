/**
 * popovici calculation
 * @author tweber
 * @date 2013-2016
 * @license GPLv2
 *
 * @desc This is a reimplementation in C++ of the file rc_popma.m of the
 *		rescal5 package by Zinkin, McMorrow, Tennant, Farhi, and Wildes:
 *		http://www.ill.eu/en/instruments-support/computing-for-science/cs-software/all-software/matlab-ill/rescal-for-matlab/
 * @desc see: [pop75] M. Popovici, Acta Cryst. A 31, 507 (1975)
 */

#include "pop.h"
#include "tlibs/math/linalg.h"
#include "tlibs/math/math.h"

#include <string>
#include <iostream>


typedef t_real_reso t_real;
typedef ublas::matrix<t_real> t_mat;
typedef ublas::vector<t_real> t_vec;

using angle = tl::t_angle_si<t_real>;
using wavenumber = tl::t_wavenumber_si<t_real>;
using energy = tl::t_energy_si<t_real>;
using length = tl::t_length_si<t_real>;
using inv_length = tl::t_length_inverse_si<t_real>;

static const auto angs = tl::get_one_angstrom<t_real>();
static const auto rads = tl::get_one_radian<t_real>();
static const auto meV = tl::get_one_meV<t_real>();
static const auto cm = tl::get_one_centimeter<t_real>();


ResoResults calc_pop(const PopParams& pop)
{
	ResoResults res;

	res.Q_avg.resize(4);
	res.Q_avg[0] = pop.Q*angs;
	res.Q_avg[1] = 0.;
	res.Q_avg[2] = 0.;
	res.Q_avg[3] = pop.E / meV;


	length lam = tl::k2lam(pop.ki);
	angle twotheta = pop.twotheta;
	angle thetaa = pop.thetaa * pop.dana_sense;
	angle thetam = pop.thetam * pop.dmono_sense;
	angle ki_Q = pop.angle_ki_Q;
	angle kf_Q = pop.angle_kf_Q;
	//kf_Q = ki_Q + twotheta;

	twotheta *= pop.dsample_sense;
	ki_Q *= pop.dsample_sense;
	kf_Q *= pop.dsample_sense;


	t_mat Ti = tl::rotation_matrix_2d(ki_Q/rads);
	t_mat Tf = -tl::rotation_matrix_2d(kf_Q/rads);

	// B matrix, [pop75], Appendix 1 -> U matrix in CN
	t_mat B = ublas::zero_matrix<t_real>(4,6);
	tl::submatrix_copy(B, Ti, 0, 0);
	tl::submatrix_copy(B, Tf, 0, 3);
	B(2,2) = 1.; B(2,5) = -1.;
	B(3,0) = t_real(2)*pop.ki*angs * tl::get_KSQ2E<t_real>();
	B(3,3) = t_real(-2)*pop.kf*angs * tl::get_KSQ2E<t_real>();
	//std::cout << "k^2 to E factor: " << tl::KSQ2E << std::endl;
	//std::cout << "B = " << B << std::endl;


	angle coll_h_pre_mono = pop.coll_h_pre_mono;
	angle coll_v_pre_mono = pop.coll_v_pre_mono;

	if(pop.bGuide)
	{
		coll_h_pre_mono = lam*(pop.guide_div_h/angs);
		coll_v_pre_mono = lam*(pop.guide_div_v/angs);
	}


	// collimator covariance matrix G, [pop75], Appendix 1
	t_mat G = tl::diag_matrix({
		t_real(1)/(coll_h_pre_mono*coll_h_pre_mono /rads/rads),
		t_real(1)/(pop.coll_h_pre_sample*pop.coll_h_pre_sample /rads/rads),

		t_real(1)/(coll_v_pre_mono*coll_v_pre_mono /rads/rads),
		t_real(1)/(pop.coll_v_pre_sample*pop.coll_v_pre_sample /rads/rads),

		t_real(1)/(pop.coll_h_post_sample*pop.coll_h_post_sample /rads/rads),
		t_real(1)/(pop.coll_h_post_ana*pop.coll_h_post_ana /rads/rads),

		t_real(1)/(pop.coll_v_post_sample*pop.coll_v_post_sample /rads/rads),
		t_real(1)/(pop.coll_v_post_ana*pop.coll_v_post_ana /rads/rads)
	});


	const angle mono_mosaic_spread = pop.mono_mosaic;
	const angle ana_mosaic_spread = pop.ana_mosaic;
	const angle sample_mosaic_spread = pop.sample_mosaic;

	// crystal mosaic covariance matrix F, [pop75], Appendix 1
	t_mat F = tl::diag_matrix(
	{
		t_real(1)/(pop.mono_mosaic*pop.mono_mosaic /rads/rads),
		t_real(1)/(mono_mosaic_spread*mono_mosaic_spread /rads/rads),
		t_real(1)/(pop.ana_mosaic*pop.ana_mosaic /rads/rads),
		t_real(1)/(ana_mosaic_spread*ana_mosaic_spread /rads/rads)
	});

	// C matrix, [pop75], Appendix 1
	t_mat C = ublas::zero_matrix<t_real>(4,8);
	C(2,5) = C(2,4) = C(0,1) = C(0,0) = 0.5;
	C(1,2) = 0.5/units::sin(thetam);
	C(1,3) /*C(2,2)*/ = t_real(-0.5)/units::sin(thetam);	// Popovici says C(2,2), not C(1,3)
	C(3,6) = t_real(0.5)/units::sin(thetaa);
	C(3,7) = t_real(-0.5)/units::sin(thetaa);

	// A matrix, [pop75], Appendix 1
	t_mat A = ublas::zero_matrix<t_real>(6,8);
	A(0,0) = t_real(0.5) * pop.ki*angs * units::cos(thetam)/units::sin(thetam);
	A(0,1) = t_real(-0.5) * pop.ki*angs * units::cos(thetam)/units::sin(thetam);
	A(2,3) = A(1,1) = pop.ki * angs;
	A(3,4) = t_real(0.5) * pop.kf*angs * units::cos(thetaa)/units::sin(thetaa);
	A(3,5) = t_real(-0.5) * pop.kf*angs * units::cos(thetaa)/units::sin(thetaa);
	A(5,6) = A(4,4) = pop.kf * angs;



	// S matrix, [pop75], Appendix 2
	// source
	t_real dMult = 1./12.;
	if(!pop.bSrcRect) dMult = 1./16.;
	t_real dSiSrc[] =
	{
		dMult * pop.src_w*pop.src_w /cm/cm,
		dMult * pop.src_h*pop.src_h /cm/cm
	};

	// mono
	t_real dSiMono[] =
	{
		t_real(1./12.) * pop.mono_thick*pop.mono_thick /cm/cm,
		t_real(1./12.) * pop.mono_w*pop.mono_w /cm/cm,
		t_real(1./12.) * pop.mono_h*pop.mono_h /cm/cm
	};

	// sample
	dMult = 1./12.;
	if(!pop.bSampleCub) dMult = 1./16.;
	t_real dSiSample[] =
	{
		dMult * pop.sample_w_perpq *pop.sample_w_perpq /cm/cm,
		dMult * pop.sample_w_q*pop.sample_w_q /cm/cm,
		t_real(1./12.) * pop.sample_h*pop.sample_h /cm/cm
	};

	// ana
	t_real dSiAna[] =
	{
		t_real(1./12.) * pop.ana_thick*pop.ana_thick /cm/cm,
		t_real(1./12.) * pop.ana_w*pop.ana_w /cm/cm,
		t_real(1./12.) * pop.ana_h*pop.ana_h /cm/cm
	};

	// det
	dMult = 1./12.;
	if(!pop.bDetRect) dMult = 1./16.;
	t_real dSiDet[] =
	{
		dMult * pop.det_w*pop.det_w /cm/cm,
		dMult * pop.det_h*pop.det_h /cm/cm
	};

	t_mat SI = tl::diag_matrix({dSiSrc[0], dSiSrc[1],
		dSiMono[0], dSiMono[1], dSiMono[2],
		dSiSample[0], dSiSample[1], dSiSample[2],
		dSiAna[0], dSiAna[1], dSiAna[2],
		dSiDet[0], dSiDet[1]});
	SI *= tl::get_SIGMA2FWHM<t_real>()*tl::get_SIGMA2FWHM<t_real>();

	t_mat S;
	if(!tl::inverse(SI, S))
	{
		res.bOk = false;
		res.strErr = "S matrix cannot be inverted.";
		return res;
	}


	// --------------------------------------------------------------------
	// mono/ana focus
	length mono_curvh = pop.mono_curvh, mono_curvv = pop.mono_curvv;
	length ana_curvh = pop.ana_curvh, ana_curvv = pop.ana_curvv;

	if(pop.bMonoIsOptimallyCurvedH) mono_curvh = tl::foc_curv(pop.dist_src_mono, pop.dist_mono_sample, units::abs(t_real(2)*thetam), false);
	if(pop.bMonoIsOptimallyCurvedV) mono_curvv = tl::foc_curv(pop.dist_src_mono, pop.dist_mono_sample, units::abs(t_real(2)*thetam), true);
	if(pop.bAnaIsOptimallyCurvedH) ana_curvh = tl::foc_curv(pop.dist_sample_ana, pop.dist_ana_det, units::abs(t_real(2)*thetaa), false);
	if(pop.bAnaIsOptimallyCurvedV) ana_curvv = tl::foc_curv(pop.dist_sample_ana, pop.dist_ana_det, units::abs(t_real(2)*thetaa), true);

	mono_curvh *= pop.dmono_sense; mono_curvv *= pop.dmono_sense;
	ana_curvh *= pop.dana_sense; ana_curvv *= pop.dana_sense;

	inv_length inv_mono_curvh = t_real(0)/cm, inv_mono_curvv = t_real(0)/cm;
	inv_length inv_ana_curvh = t_real(0)/cm, inv_ana_curvv = t_real(0)/cm;

	if(pop.bMonoIsCurvedH) inv_mono_curvh = t_real(1)/mono_curvh;
	if(pop.bMonoIsCurvedV) inv_mono_curvv = t_real(1)/mono_curvv;
	if(pop.bAnaIsCurvedH) inv_ana_curvh = t_real(1)/ana_curvh;
	if(pop.bAnaIsCurvedV) inv_ana_curvv = t_real(1)/ana_curvv;

	//if(pop.bMonoIsCurvedH) tl::log_debug("mono curv h: ", mono_curvh);
	//if(pop.bMonoIsCurvedV) tl::log_debug("mono curv v: ", mono_curvv);
	//if(pop.bAnaIsCurvedH) tl::log_debug("ana curv h: ", ana_curvh);
	//if(pop.bAnaIsCurvedV) tl::log_debug("ana curv v: ", ana_curvv);
	// --------------------------------------------------------------------



	// T matrix, [pop75], Appendix 2
	t_mat T = ublas::zero_matrix<t_real>(4,13);
	T(0,0) = t_real(-0.5) / (pop.dist_src_mono / cm);
	T(0,2) = t_real(0.5) * units::cos(thetam) *
		(t_real(1)/(pop.dist_mono_sample/cm) - t_real(1)/(pop.dist_src_mono/cm));
	T(0,3) = t_real(0.5) * units::sin(thetam) *
		(t_real(1)/(pop.dist_src_mono/cm) + t_real(1)/(pop.dist_mono_sample/cm) -
		t_real(2)*inv_mono_curvh*cm/(units::sin(thetam)));
	T(0,5) = t_real(0.5) * units::sin(t_real(0.5)*twotheta) / (pop.dist_mono_sample/cm);
	T(0,6) = t_real(0.5) * units::cos(t_real(0.5)*twotheta) / (pop.dist_mono_sample/cm);
	T(1,1) = t_real(-0.5)/(pop.dist_src_mono/cm * units::sin(thetam));
	T(1,4) = t_real(0.5) * (t_real(1)/(pop.dist_src_mono/cm) +
		t_real(1)/(pop.dist_mono_sample/cm) -
		t_real(2)*units::sin(thetam)*inv_mono_curvv*cm)
		/ (units::sin(thetam));
	T(1,7) = t_real(-0.5)/(pop.dist_mono_sample/cm * units::sin(thetam));
	T(2,5) = t_real(0.5)*units::sin(t_real(0.5)*twotheta) / (pop.dist_sample_ana/cm);
	T(2,6) = t_real(-0.5)*units::cos(t_real(0.5)*twotheta) / (pop.dist_sample_ana/cm);
	T(2,8) = t_real(0.5)*units::cos(thetaa) * (t_real(1)/(pop.dist_ana_det/cm) -
		t_real(1)/(pop.dist_sample_ana/cm));
	T(2,9) = t_real(0.5)*units::sin(thetaa) * (
		t_real(1)/(pop.dist_sample_ana/cm) +
		t_real(1)/(pop.dist_ana_det/cm) -
		t_real(2)*inv_ana_curvh*cm / (units::sin(thetaa)));
	T(2,11) = t_real(0.5)/(pop.dist_ana_det/cm);
	T(3,7) = t_real(-0.5)/(pop.dist_sample_ana/cm*units::sin(thetaa));
	T(3,10) = t_real(0.5)*(1./(pop.dist_sample_ana/cm) +
		t_real(1)/(pop.dist_ana_det/cm) -
		t_real(2)*units::sin(thetaa)*inv_ana_curvv*cm)
		/ (units::sin(thetaa));
	T(3,12) = t_real(-0.5)/(pop.dist_ana_det/cm*units::sin(thetaa));


	// D matrix, [pop75], Appendix 2
	t_mat D = ublas::zero_matrix<t_real>(8,13);
	D(0,0) = t_real(-1) / (pop.dist_src_mono/cm);
	D(0,2) = -cos(thetam) / (pop.dist_src_mono/cm);
	D(0,3) = sin(thetam) / (pop.dist_src_mono/cm);
	D(1,2) = cos(thetam) / (pop.dist_mono_sample/cm);
	D(1,3) = sin(thetam) / (pop.dist_mono_sample/cm);
	D(1,5) = sin(t_real(0.5)*twotheta) / (pop.dist_mono_sample/cm);
	D(1,6) = cos(t_real(0.5)*twotheta) / (pop.dist_mono_sample/cm);
	D(2,1) = t_real(-1) / (pop.dist_src_mono/cm);
	D(2,4) = t_real(1) / (pop.dist_src_mono/cm);
	D(3,4) = t_real(-1) / (pop.dist_mono_sample/cm);
	D(3,7) = t_real(1) / (pop.dist_mono_sample/cm);
	D(4,5) = sin(t_real(0.5)*twotheta) / (pop.dist_sample_ana/cm);
	D(4,6) = -cos(t_real(0.5)*twotheta) / (pop.dist_sample_ana/cm);
	D(4,8) = -cos(thetaa) / (pop.dist_sample_ana/cm);
	D(4,9) = sin(thetaa) / (pop.dist_sample_ana/cm);
	D(5,8) = cos(thetaa) / (pop.dist_ana_det/cm);
	D(5,9) = sin(thetaa) / (pop.dist_ana_det/cm);
	D(5,11) = t_real(1) / (pop.dist_ana_det/cm);
	D(6,7) = t_real(-1) / (pop.dist_sample_ana/cm);
	D(6,10) = t_real(1) / (pop.dist_sample_ana/cm);
	D(7,10) = t_real(-1) / (pop.dist_ana_det/cm);
	D(7,12) = t_real(1) / (pop.dist_ana_det/cm);


	// [pop75], equ. 20
	t_mat M0 = S + tl::transform(F, T, 1);
	t_mat M0i;
	if(!tl::inverse(M0, M0i))
	{
		res.bOk = false;
		res.strErr = "Matrix M0 cannot be inverted.";
		return res;
	}

	t_mat M1 = tl::transform_inv(M0i, D, 1);
	t_mat M1i;
	if(!tl::inverse(M1, M1i))
	{
		res.bOk = false;
		res.strErr = "Matrix M1 cannot be inverted.";
		return res;
	}

	t_mat M2 = M1i + G;
	t_mat M2i;
	if(!tl::inverse(M2, M2i))
	{
		res.bOk = false;
		res.strErr = "Matrix M2 cannot be inverted.";
		return res;
	}

	t_mat BA = ublas::prod(B,A);
	t_mat ABt = ublas::prod(ublas::trans(A), ublas::trans(B));
	t_mat M2iABt = ublas::prod(M2i, ABt);
	t_mat MI = ublas::prod(BA, M2iABt);

	MI(1,1) += pop.Q*pop.Q*angs*angs * pop.sample_mosaic*pop.sample_mosaic /rads/rads;
	MI(2,2) += pop.Q*pop.Q*angs*angs * sample_mosaic_spread*sample_mosaic_spread /rads/rads;

	if(!tl::inverse(MI, res.reso))
	{
		res.bOk = false;
		res.strErr = "Covariance matrix cannot be inverted.";
		return res;
	}


	// -------------------------------------------------------------------------


	res.reso *= tl::get_SIGMA2FWHM<t_real>()*tl::get_SIGMA2FWHM<t_real>();
	res.reso_v = ublas::zero_vector<t_real>(4);
	res.reso_s = 0.;

	if(pop.dsample_sense < 0.)
	{
		// mirror Q_perp
		t_mat matMirror = tl::mirror_matrix<t_mat>(res.reso.size1(), 1);
		res.reso = tl::transform(res.reso, matMirror, true);
		res.reso_v[1] = -res.reso_v[1];
	}


	res.dResVol = tl::get_ellipsoid_volume(res.reso);
	res.dR0 = 0.;
	const t_real pi = tl::get_pi<t_real>();
	if(pop.bCalcR0)
	{
		//SI /= tl::SIGMA2FWHM*tl::SIGMA2FWHM;
		//S *= tl::SIGMA2FWHM*tl::SIGMA2FWHM;

		// resolution volume, [pop75], equ. 13a & 16
		// [D] = 1/cm, [SI] = cm^2
		t_mat DSiDt = tl::transform_inv(SI, D, 1);
		t_mat DSiDti;
		if(!tl::inverse(DSiDt, DSiDti))
		{
			res.bOk = false;
			res.strErr = "Resolution volume cannot be calculated.";
			return res;
		}
		DSiDti += G;
		t_real dP0 = pop.dmono_refl*pop.dana_effic *
			t_real((2.*pi)*(2.*pi)*(2.*pi)*(2.*pi)) /
			std::sqrt(tl::determinant(DSiDti));

		// [T] = 1/cm, [F] = 1/rad^2, [pop75], equ. 15
		t_mat K = S + tl::transform(F, T, 1);

		t_real dDetS = tl::determinant(S);
		t_real dDetF = tl::determinant(F);
		t_real dDetK = tl::determinant(K);

		// [pop75], equ. 16
		res.dR0 = dP0 / (t_real(8.*pi*8.*pi) * units::sin(thetam)*units::sin(thetaa));
		res.dR0 *= std::sqrt(dDetS*dDetF/dDetK);

		// rest of the prefactors, equ. 1 in [pop75]
		//res.dR0 *= std::sqrt(tl::determinant(res.reso)) / (2.*pi*2.*pi);
		//res.dR0 *= res.dResVol;		// TODO: check
	}

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
