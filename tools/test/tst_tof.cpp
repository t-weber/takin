/**
 * @author Tobias Weber <tobias.weber@tum.de>
 * @license GPLv2
 */

// gcc -DNO_QT -I. -I../.. -o tst_tof ../../tools/test/tst_tof.cpp ../../tools/res/viol.cpp ../../tlibs/log/log.cpp -lstdc++ -std=c++11 -lstdc++ -lm

#include "tools/res/viol.h"
#include <iostream>

int main()
{
	ViolParams parms;

	parms.ki = 1.4 / tl::get_one_angstrom<double>();
	parms.kf = 1.4 / tl::get_one_angstrom<double>();
	//parms.E = 0. * tl::get_one_meV<double>();

	parms.len_pulse_mono = 10. * tl::get_one_meter<double>();
	parms.len_mono_sample = 1. * tl::get_one_meter<double>();
	parms.len_sample_det = 5. * tl::get_one_meter<double>();

	parms.twotheta = tl::d2r(75.) * tl::get_one_radian<double>();
	parms.twotheta_i = 0. * tl::get_one_radian<double>();
	parms.angle_outplane_i = 0. * tl::get_one_radian<double>();
	parms.angle_outplane_f = 0. * tl::get_one_radian<double>();

	parms.sig_len_pulse_mono = 0.01 * tl::get_one_meter<double>();
	parms.sig_len_mono_sample = 0.01 * tl::get_one_meter<double>();
	parms.sig_len_sample_det = 0.01 * tl::get_one_meter<double>();

	parms.sig_pulse = 50e-6 * tl::get_one_second<double>();
	parms.sig_mono = 5e-6 * tl::get_one_second<double>();
	parms.sig_det = 5e-6 * tl::get_one_second<double>();

	parms.sig_twotheta_i = tl::m2r(30.) * tl::get_one_radian<double>();
	parms.sig_twotheta_f = tl::m2r(30.) * tl::get_one_radian<double>();
	parms.sig_outplane_i = tl::m2r(30.) * tl::get_one_radian<double>();
	parms.sig_outplane_f = tl::m2r(30.) * tl::get_one_radian<double>();

	ResoResults res = calc_viol(parms);

	return 0;
}
