/**
 * simple resolution calculation including only ki and kf errors
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date jun-2016
 * @license GPLv2
 */

#ifndef __SIMPLERESO_H__
#define __SIMPLERESO_H__

#include "defs.h"
#include "tlibs/phys/neutrons.h"

namespace units = boost::units;
namespace codata = boost::units::si::constants::codata;


struct SimpleResoParams
{
	// values
	tl::t_wavenumber_si<t_real_reso> ki, kf, Q;
	tl::t_energy_si<t_real_reso> E;

	tl::t_angle_si<t_real_reso> twotheta,
		angle_ki_Q, angle_kf_Q;

	// sigmas
	tl::t_wavenumber_si<t_real_reso> sig_ki, sig_kf;
	tl::t_wavenumber_si<t_real_reso> sig_ki_perp, sig_kf_perp;
	tl::t_wavenumber_si<t_real_reso> sig_ki_z, sig_kf_z;
};


extern ResoResults calc_simplereso(const SimpleResoParams& params);

#endif
