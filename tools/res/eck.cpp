/**
 * implementation of the eckold-sobolev algo
 *
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date feb-2015
 * @license GPLv2
 *
 * @desc for algorithm: [eck14] G. Eckold and O. Sobolev, NIM A 752, pp. 54-64 (2014)
 */

#include "eck.h"
#include "helper.h"

#include "tlibs/math/linalg.h"
#include "tlibs/math/math.h"
#include "ellipse.h"

#include <tuple>
#include <future>
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
static const auto secs = tl::get_one_second<t_real>();
static const t_real pi = tl::get_pi<t_real>();
static const t_real sig2fwhm = tl::get_SIGMA2FWHM<t_real>();


static std::tuple<t_mat, t_vec, t_real, t_real, t_real>
get_mono_vals(const length& src_w, const length& src_h,
	const length& mono_w, const length& mono_h,
	const length& dist_src_mono, const length& dist_mono_sample,
	const wavenumber& ki, const angle& thetam,
	const angle& coll_h_pre_mono, const angle& coll_h_pre_sample,
	const angle& coll_v_pre_mono, const angle& coll_v_pre_sample,
	const angle& mono_mosaic, const angle& mono_mosaic_v,
	const inv_length& inv_mono_curvh, const inv_length& inv_mono_curvv,
	const length& pos_x , const length& pos_y, const length& pos_z,
	t_real dRefl)
{
	// A matrix: formula 26 in [eck14]
	t_mat A = ublas::identity_matrix<t_real>(3);
	{
		const auto A_t0 = t_real(1) / mono_mosaic;
		const auto A_tx = inv_mono_curvh*dist_mono_sample / units::abs(units::sin(thetam));
		const auto A_t1 = A_t0*A_tx;

		A(0,0) = t_real(0.5)*sig2fwhm*sig2fwhm / (ki*angs*ki*angs) *
			units::tan(thetam)*units::tan(thetam) *
		(
/*a*/			+ units::pow<2>(t_real(2)/coll_h_pre_mono) *rads*rads
/*b*/			+ units::pow<2>(t_real(2)*dist_src_mono/src_w)
/*c*/			+ A_t0*A_t0 *rads*rads
		);
		A(0,1) = A(1,0) = t_real(0.5)*sig2fwhm*sig2fwhm / (ki*angs*ki*angs)
			* units::tan(thetam) *
		(
/*w*/			+ t_real(2)*tl::my_units_pow2(t_real(1)/coll_h_pre_mono) *rads*rads
/*x*/			+ t_real(2)*dist_src_mono*(dist_src_mono-dist_mono_sample)/(src_w*src_w)
/*y*/			+ A_t0*A_t0 * rads*rads
/*z*/			- A_t0*A_t1 *rads*rads
		);
		A(1,1) = t_real(0.5)*sig2fwhm*sig2fwhm / (ki*angs*ki*angs) *
		(
/*1*/			+ units::pow<2>(t_real(1)/coll_h_pre_mono) *rads*rads
/*2*/			+ units::pow<2>(t_real(1)/coll_h_pre_sample) *rads*rads
/*3*/			+ units::pow<2>((dist_src_mono-dist_mono_sample)/src_w)
/*4*/			+ units::pow<2>(dist_mono_sample/(mono_w*units::abs(units::sin(thetam))))

/*5*/			+ A_t0*A_t0 *rads*rads
/*6*/			- t_real(2)*A_t0*A_t1 *rads*rads
/*7*/			+ A_t1*A_t1 *rads*rads
		);
	}

	// Av matrix: formula 38 in [eck14]
	// some typos in paper leading to the (false) result of a better Qz resolution when focusing
	// => trying to match terms in Av with corresponding terms in A
	// corresponding pre-mono terms commented out in Av, as they are not considered there
	t_mat Av(2,2);
	{
		const auto Av_t0 = t_real(0.5) / (mono_mosaic_v*units::abs(units::sin(thetam)));
		const auto Av_t1 = inv_mono_curvv*dist_mono_sample / mono_mosaic_v;

		Av(0,0) = t_real(0.5)*sig2fwhm*sig2fwhm / (ki*angs*ki*angs) *
		(
/*1*/	//		+ units::pow<2>(t_real(1)/coll_v_pre_mono) *rads*rads	// missing in paper?
/*2*/			+ units::pow<2>(t_real(1)/coll_v_pre_sample) *rads*rads
/*~3*/			+ units::pow<2>(dist_mono_sample/src_h)
/*4*/			+ units::pow<2>(dist_mono_sample/mono_h)

/*5*/			+ Av_t0*Av_t0 * rads*rads 				// typo in paper?
/*6*/			- t_real(2)*Av_t0*Av_t1 * rads*rads
/*7*/			+ Av_t1*Av_t1 * rads*rads 				// missing in paper?
		);
		Av(0,1) = Av(1,0) = t_real(0.5)*sig2fwhm*sig2fwhm / (ki*angs*ki*angs) *
		(
/*w*/	//		- units::pow<2>(1./coll_v_pre_mono) *rads*rads		// missing in paper?
/*~x*/			+ dist_src_mono*dist_mono_sample/(src_h*src_h)
/*y*/			- Av_t0*Av_t0 * rads*rads
/*z*/			+ Av_t0*Av_t1 * rads*rads
		);
		Av(1,1) = t_real(0.5)*sig2fwhm*sig2fwhm / (ki*angs*ki*angs) *
		(
/*a*/			+ units::pow<2>(t_real(1)/(coll_v_pre_mono)) *rads*rads
/*b*/			+ units::pow<2>(dist_src_mono/src_h)
/*c*/			+ Av_t0*Av_t0 *rads*rads
		);
	}

	// B vector: formula 27 in [eck14]
	t_vec B(3);
	{
		const auto B_t0 = inv_mono_curvh / (mono_mosaic*mono_mosaic*units::abs(units::sin(thetam)));

		B(0) = sig2fwhm*sig2fwhm * pos_y / (ki*angs) * units::tan(thetam) *
		(
/*i*/			+ t_real(2)*dist_src_mono / (src_w*src_w)
/*j*/			+ B_t0 *rads*rads
		);
		B(1) = sig2fwhm*sig2fwhm * pos_y / (ki*angs) *
		(
/*r*/			- dist_mono_sample / (units::pow<2>(mono_w*units::abs(units::sin(thetam))))
/*s*/			+ B_t0 * rads*rads
/*t*/			- B_t0 * rads*rads * inv_mono_curvh*dist_mono_sample /
					(units::abs(units::sin(thetam)))
/*u*/			+ (dist_src_mono-dist_mono_sample) / (src_w*src_w)
		);
	}

	// Bv vector: formula 39 in [eck14]
	t_vec Bv(2);
	{
		const auto Bv_t0 = inv_mono_curvv/(mono_mosaic_v*mono_mosaic_v);

		Bv(0) = sig2fwhm*sig2fwhm * pos_z / (ki*angs) * t_real(-1.) *
		(
/*r*/			+ dist_mono_sample / (mono_h*mono_h)	// typo in paper?
/*~s*/			- t_real(0.5)*Bv_t0 *rads*rads / units::abs(units::sin(thetam))
/*~t*/			+ Bv_t0 * rads*rads * inv_mono_curvv*dist_mono_sample
/*~u*/			+ dist_mono_sample / (src_h*src_h)		// typo in paper?
		);
		Bv(1) = sig2fwhm*sig2fwhm * pos_z / (ki*angs) * t_real(-1.) *
		(
/*i*/			+ dist_src_mono / (src_h*src_h)			// typo in paper?
/*j*/			+ t_real(0.5)*Bv_t0/units::abs(units::sin(thetam)) * rads*rads
		);
	}


	// C scalar: formula 28 in [eck14]
	t_real C = t_real(0.5)*sig2fwhm*sig2fwhm * pos_y*pos_y *
	(
		t_real(1)/(src_w*src_w) +
		units::pow<2>(t_real(1)/(mono_w*units::abs(units::sin(thetam)))) +
		units::pow<2>(inv_mono_curvh/(mono_mosaic * units::abs(units::sin(thetam)))) *rads*rads
	);

	// Cv scalar: formula 40 in [eck14]
	t_real Cv = t_real(0.5)*sig2fwhm*sig2fwhm * pos_z*pos_z *
	(
		t_real(1)/(src_h*src_h) +
		t_real(1)/(mono_h*mono_h) +
		units::pow<2>(inv_mono_curvv/mono_mosaic_v) *rads*rads
	);


	// z components, [eck14], equ. 42
	A(2,2) = Av(0,0) - Av(0,1)*Av(0,1)/Av(1,1);
	B[2] = Bv[0] - Bv[1]*Av(0,1)/Av(1,1);
	t_real D = Cv - t_real(0.25)*Bv[1]/Av(1,1);


	// [eck14], equ. 54
	t_real refl = dRefl * std::sqrt(pi / (Av(1,1) * A(1,1)));	// check: typo in paper?


	return std::make_tuple(A, B, C, D, refl);
}


ResoResults calc_eck(const EckParams& eck)
{
	angle twotheta = eck.twotheta * eck.dsample_sense;
	angle thetaa = eck.thetaa * eck.dana_sense;
	angle thetam = eck.thetam * eck.dmono_sense;
	angle ki_Q = eck.angle_ki_Q * eck.dsample_sense;
	angle kf_Q = eck.angle_kf_Q * eck.dsample_sense;
	//kf_Q = ki_Q + twotheta;


	// --------------------------------------------------------------------
	// mono/ana focus
	length mono_curvh = eck.mono_curvh, mono_curvv = eck.mono_curvv;
	length ana_curvh = eck.ana_curvh, ana_curvv = eck.ana_curvv;

	if(eck.bMonoIsOptimallyCurvedH) mono_curvh = tl::foc_curv(eck.dist_src_mono, eck.dist_mono_sample, units::abs(t_real(2)*thetam), false);
	if(eck.bMonoIsOptimallyCurvedV) mono_curvv = tl::foc_curv(eck.dist_src_mono, eck.dist_mono_sample, units::abs(t_real(2)*thetam), true);
	if(eck.bAnaIsOptimallyCurvedH) ana_curvh = tl::foc_curv(eck.dist_sample_ana, eck.dist_ana_det, units::abs(t_real(2)*thetaa), false);
	if(eck.bAnaIsOptimallyCurvedV) ana_curvv = tl::foc_curv(eck.dist_sample_ana, eck.dist_ana_det, units::abs(t_real(2)*thetaa), true);

	//mono_curvh *= eck.dmono_sense; mono_curvv *= eck.dmono_sense;
	//ana_curvh *= eck.dana_sense; ana_curvv *= eck.dana_sense;

	inv_length inv_mono_curvh = t_real(0)/cm, inv_mono_curvv = t_real(0)/cm;
	inv_length inv_ana_curvh = t_real(0)/cm, inv_ana_curvv = t_real(0)/cm;

	if(eck.bMonoIsCurvedH) inv_mono_curvh = t_real(1)/mono_curvh;
	if(eck.bMonoIsCurvedV) inv_mono_curvv = t_real(1)/mono_curvv;
	if(eck.bAnaIsCurvedH) inv_ana_curvh = t_real(1)/ana_curvh;
	if(eck.bAnaIsCurvedV) inv_ana_curvv = t_real(1)/ana_curvv;

	//if(eck.bMonoIsCurvedH) tl::log_debug("mono curv h: ", mono_curvh);
	//if(eck.bMonoIsCurvedV) tl::log_debug("mono curv v: ", mono_curvv);
	//if(eck.bAnaIsCurvedH) tl::log_debug("ana curv h: ", ana_curvh);
	//if(eck.bAnaIsCurvedV) tl::log_debug("ana curv v: ", ana_curvv);
	// --------------------------------------------------------------------


	const length lam = tl::k2lam(eck.ki);

	angle coll_h_pre_mono = eck.coll_h_pre_mono;
	angle coll_v_pre_mono = eck.coll_v_pre_mono;

	if(eck.bGuide)
	{
		coll_h_pre_mono = lam*(eck.guide_div_h/angs);
		coll_v_pre_mono = lam*(eck.guide_div_v/angs);
	}


	//std::cout << "thetaM = " << t_real(thetam/rads/M_PI*180.) << " deg"<< std::endl;
	//std::cout << "thetaA = " << t_real(thetaa/rads/M_PI*180.) << " deg"<< std::endl;
	//std::cout << "ki = " << t_real(eck.ki*angs) << ", kf = " << t_real(eck.kf*angs) << std::endl;
	//std::cout << "Q = " << t_real(eck.Q*angs) << ", E = " << t_real(eck.E/meV) << std::endl;
	//std::cout << "kiQ = " << t_real(ki_Q/rads/M_PI*180.) << " deg"<< std::endl;
	//std::cout << "kfQ = " << t_real(kf_Q/rads/M_PI*180.) << " deg"<< std::endl;
	//std::cout << "2theta = " << t_real(twotheta/rads/M_PI*180.) << " deg"<< std::endl;


	ResoResults res;

	res.Q_avg.resize(4);
	res.Q_avg[0] = eck.Q*angs;
	res.Q_avg[1] = 0.;
	res.Q_avg[2] = 0.;
	res.Q_avg[3] = eck.E/meV;


	// -------------------------------------------------------------------------

	// - if the instruments works in kf=const mode and the scans are counted for
	//   or normalised to monitor counts no ki^3 or kf^3 factor is needed.
	// - if the instrument works in ki=const mode the kf^3 factor is needed.
	const auto tupScFact = get_scatter_factors(eck.flags, eck.thetam, eck.ki, eck.thetaa, eck.kf);

	t_real dmono_refl = eck.dmono_refl * std::get<0>(tupScFact);
	t_real dana_effic = eck.dana_effic * std::get<1>(tupScFact);
	if(eck.mono_refl_curve) dmono_refl *= (*eck.mono_refl_curve)(eck.ki);
	if(eck.ana_effic_curve) dana_effic *= (*eck.ana_effic_curve)(eck.kf);
	t_real dxsec = std::get<2>(tupScFact);


	//--------------------------------------------------------------------------
	// mono part

	std::launch lpol = /*std::launch::deferred |*/ std::launch::async;
	std::future<std::tuple<t_mat, t_vec, t_real, t_real, t_real>> futMono
		= std::async(lpol, get_mono_vals,
			eck.src_w, eck.src_h,
			eck.mono_w, eck.mono_h,
			eck.dist_src_mono, eck.dist_mono_sample,
			eck.ki, thetam,
			coll_h_pre_mono, eck.coll_h_pre_sample,
			coll_v_pre_mono, eck.coll_v_pre_sample,
			eck.mono_mosaic, eck.mono_mosaic_v,
			inv_mono_curvh, inv_mono_curvv,
			eck.pos_x , eck.pos_y, eck.pos_z,
			dmono_refl);

	//--------------------------------------------------------------------------


	//--------------------------------------------------------------------------
	// ana part

	// equ 43 in [eck14]
	length pos_y2 = - eck.pos_x*units::sin(twotheta)
		+ eck.pos_y*units::cos(twotheta);
	std::future<std::tuple<t_mat, t_vec, t_real, t_real, t_real>> futAna
		= std::async(lpol, get_mono_vals,
			eck.det_w, eck.det_h,
			eck.ana_w, eck.ana_h,
			eck.dist_ana_det, eck.dist_sample_ana,
			eck.kf, -thetaa,
			eck.coll_h_post_ana, eck.coll_h_post_sample,
			eck.coll_v_post_ana, eck.coll_v_post_sample,
			eck.ana_mosaic, eck.ana_mosaic_v,
			inv_ana_curvh, inv_ana_curvv,
			eck.pos_x, pos_y2, eck.pos_z,
			dana_effic);

	//--------------------------------------------------------------------------
	// get mono & ana results

	std::tuple<t_mat, t_vec, t_real, t_real, t_real> tupMono = futMono.get();
	const t_mat& A = std::get<0>(tupMono);
	const t_vec& B = std::get<1>(tupMono);
	const t_real& C = std::get<2>(tupMono);
	const t_real& D = std::get<3>(tupMono);
	const t_real& dReflM = std::get<4>(tupMono);

	std::tuple<t_mat, t_vec, t_real, t_real, t_real> tupAna = futAna.get();
	const t_mat& E = std::get<0>(tupAna);
	const t_vec& F = std::get<1>(tupAna);
	const t_real& G = std::get<2>(tupAna);
	const t_real& H = std::get<3>(tupAna);
	const t_real& dReflA = std::get<4>(tupAna);

	/*std::cout << "A = " << A << std::endl;
	std::cout << "B = " << B << std::endl;
	std::cout << "C = " << C << std::endl;
	std::cout << "D = " << D << std::endl;
	std::cout << "RM = " << dReflM << std::endl;

	std::cout << "E = " << E << std::endl;
	std::cout << "F = " << F << std::endl;
	std::cout << "G = " << G << std::endl;
	std::cout << "H = " << H << std::endl;
	std::cout << "RA = " << dReflA << std::endl;*/

	//--------------------------------------------------------------------------


	// equ 4 & equ 53 in [eck14]
	const t_real dE = (eck.ki*eck.ki - eck.kf*eck.kf) / (t_real(2)*eck.Q*eck.Q);
	const wavenumber kipara = eck.Q*(t_real(0.5)+dE);
	const wavenumber kfpara = eck.Q-kipara;
	wavenumber kperp = tl::my_units_sqrt<wavenumber>(units::abs(kipara*kipara - eck.ki*eck.ki));
	kperp *= eck.dsample_sense;

	const t_real ksq2E = tl::get_KSQ2E<t_real>();

	// trafo, equ 52 in [eck14]
	t_mat T = ublas::identity_matrix<t_real>(6);
	T(0,3) = T(1,4) = T(2,5) = -1.;
	T(3,0) = t_real(2)*ksq2E * kipara * angs;
	T(3,3) = t_real(2)*ksq2E * kfpara * angs;
	T(3,1) = t_real(2)*ksq2E * kperp * angs;
	T(3,4) = t_real(-2)*ksq2E * kperp * angs;
	T(4,1) = T(5,2) = (0.5 - dE);
	T(4,4) = T(5,5) = (0.5 + dE);
	t_mat Tinv;
	if(!tl::inverse(T, Tinv))
	{
		res.bOk = false;
		res.strErr = "Matrix T cannot be inverted.";
		return res;
	}
	//std::cout << "Trafo matrix (Eck) = " << T << std::endl;
	//std::cout << "Tinv = " << Tinv << std::endl;

	// equ 54 in [eck14]
	t_mat Dalph_i = tl::rotation_matrix_3d_z(-ki_Q/rads);
	t_mat Dalph_f = tl::rotation_matrix_3d_z(-kf_Q/rads);
	t_mat Arot = tl::transform(A, Dalph_i, 1);
	t_mat Erot = tl::transform(E, Dalph_f, 1);

	t_mat matAE = ublas::zero_matrix<t_real>(6,6);
	tl::submatrix_copy(matAE, Arot, 0,0);
	tl::submatrix_copy(matAE, Erot, 3,3);
	//std::cout << "AE = " << matAE << std::endl;

	// U1 matrix
	t_mat U1 = tl::transform(matAE, Tinv, 1);	// typo in paper in quadric trafo in equ 54 (top)?
	//std::cout << "U1 = " << U1 << std::endl;

	// V1 vector
	t_vec vecBF = ublas::zero_vector<t_real>(6);
	t_vec vecBrot = ublas::prod(ublas::trans(Dalph_i), B);
	t_vec vecFrot = ublas::prod(ublas::trans(Dalph_f), F);
	tl::subvector_copy(vecBF, vecBrot, 0);
	tl::subvector_copy(vecBF, vecFrot, 3);
	t_vec V1 = ublas::prod(vecBF, Tinv);



	//--------------------------------------------------------------------------
	// integrate last 2 vars -> equs 57 & 58 in [eck14]

	t_mat U2 = ellipsoid_gauss_int(U1, 5);
	t_mat U = ellipsoid_gauss_int(U2, 4);

	t_vec V2 = ellipsoid_gauss_int(V1, U1, 5);
	t_vec V = ellipsoid_gauss_int(V2, U2, 4);

	t_real W = (C + D + G + H) - 0.25*V1[5]/U1(5,5) - 0.25*V2[4]/U2(4,4);

	t_real Z = dReflM*dReflA
		* std::sqrt(pi/std::abs(U1(5,5)))
		* std::sqrt(pi/std::abs(U2(4,4)));

	/*std::cout << "U = " << U << std::endl;
	std::cout << "V = " << V << std::endl;
	std::cout << "W = " << W << std::endl;
	std::cout << "Z = " << Z << std::endl;*/
	//--------------------------------------------------------------------------


	// quadratic part of quadric (matrix U)
	// careful: factor -0.5*... missing in U matrix compared to normal gaussian!
	res.reso = t_real(2) * U;
	// linear (vector V) and constant (scalar W) part of quadric
	res.reso_v = V;
	res.reso_s = W;

	if(eck.dsample_sense < 0.)
	{
		// mirror Q_perp
		t_mat matMirror = tl::mirror_matrix<t_mat>(res.reso.size1(), 1);
		res.reso = tl::transform(res.reso, matMirror, true);
		res.reso_v[1] = -res.reso_v[1];
	}

	// prefactor and volume
	res.dResVol = tl::get_ellipsoid_volume(res.reso);
	res.dR0 = Z * std::exp(-W) /*/ res.dResVol*/;
	res.dR0 *= dxsec;

	// missing volume prefactor, cf. equ. 56 in [eck14] to  equ. 1 in [pop75] and equ. A.57 in [mit84]
	//res.dR0 /= std::sqrt(std::abs(tl::determinant(res.reso))) / (2.*pi*2.*pi);
	//res.dR0 *= res.dResVol * pi * t_real(3.);

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
