/**
 * r0 resolution function normalisation factor
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 2013-2019
 * @license GPLv2
 *
 * @desc see:
 *	[ch73] N. J. Chesser and J. D. Axe, Acta Cryst. A 29, 160 (1973)
 *	[mit84] P. W. Mitchell, R. A. Cowley and S. A. Higgins, Acta Cryst. Sec A, 40(2), 152-160 (1984)
 */

#include "r0.h"
#include "helper.h"

#include "tlibs/math/math.h"
#include "tlibs/math/linalg.h"
#include "tlibs/log/log.h"



// -----------------------------------------------------------------------------
namespace units = boost::units;
namespace codata = boost::units::si::constants::codata;

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
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
/**
 * scattering factors
 */
std::tuple<t_real, t_real, t_real> get_scatter_factors(std::size_t flags,
	const angle& thetam, const wavenumber& ki,
	const angle& thetaa, const wavenumber& kf)
{
	t_real dmono = t_real(1);
	t_real dana = t_real(1);
	t_real dSqwToXSec = t_real(1);

	if(flags & CALC_KI3)
		dmono *= tl::ana_effic_factor(ki, units::abs(thetam));
	if(flags & CALC_KF3)
		dana *= tl::ana_effic_factor(kf, units::abs(thetaa));
	if(flags & CALC_KFKI)
		dSqwToXSec *= kf/ki;	// see Shirane, equ. (2.7)

	return std::make_tuple(dmono, dana, dSqwToXSec);
}

// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
/**
 * R0 factor from formula (2) in [ch73]
 */
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
