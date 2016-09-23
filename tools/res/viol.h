/**
 * implementation of the Violini TOF reso algorithm
 * @author tweber
 * @date apr-2016
 * @license GPLv2
 *
 * @desc for algo, see: [viol14] N. Violini et al., NIM A 736 (2014) pp. 31-39
 */

#ifndef __TOFRESO_H__
#define __TOFRESO_H__

#include "defs.h"
#include "tlibs/math/neutrons.h"

namespace units = boost::units;
namespace codata = boost::units::si::constants::codata;


enum class TofDetShape { SPH, CYL, UNKNOWN };


/**
 * TOF parameters in sigma
 */
struct ViolParams
{
	// scattering triangle
	tl::t_wavenumber_si<t_real_reso> ki, kf, Q;
	tl::t_energy_si<t_real_reso> E;

	tl::t_angle_si<t_real_reso> twotheta,
		angle_ki_Q, angle_kf_Q;

	tl::t_angle_si<t_real_reso> angle_outplane_i, angle_outplane_f;
	tl::t_angle_si<t_real_reso> twotheta_i;


	// instrument lengths
	tl::t_length_si<t_real_reso> len_pulse_mono,
		len_mono_sample, len_sample_det;


	// instrument sigmas
	tl::t_length_si<t_real_reso> sig_len_pulse_mono,
		sig_len_mono_sample, sig_len_sample_det;

	tl::t_time_si<t_real_reso> sig_pulse, sig_mono, sig_det;

	tl::t_angle_si<t_real_reso> sig_twotheta_f, sig_outplane_f,
		sig_twotheta_i, sig_outplane_i;


	TofDetShape det_shape = TofDetShape::SPH;
};


extern ResoResults calc_viol(const ViolParams& params);

#endif
