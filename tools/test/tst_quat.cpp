/**
 * @author Tobias Weber <tobias.weber@tum.de>
 * @license GPLv2
 */

// gcc -I../.. -o tst_quat tst_quat.cpp -lstdc++ -lm -std=c++11

#include <iostream>

#include "tlibs/math/linalg.h"
#include "tlibs/math/quat.h"

using namespace tl;

int main()
{
	//ublas::matrix<double> rot = rotation_matrix(make_vec({1.,1.,1.}), 1.23);
	ublas::matrix<double> rot = rotation_matrix_3d_x(1.23);
	math::quaternion<double> quat = rot3_to_quat(rot);
	ublas::matrix<double> rot2 = quat_to_rot3(quat);

	std::cout << "rot: " << rot << std::endl;
	std::cout << "quat: " << quat << std::endl;
	std::cout << "rot2: " << rot2 << std::endl;


	double dAngle = 1.23;
	math::quaternion<double> quata = rotation_quat_x(dAngle);
	ublas::matrix<double> rota = quat_to_rot3(quata);
	std::cout << std::endl;
	std::cout << "quat a: " << quata << std::endl;
	std::cout << "rot a: " << rota << std::endl;
	std::cout << "angle a (quat): " << rotation_angle(quata) << std::endl;
	std::cout << "axis a (quat): " << rotation_axis(quata) << std::endl;
	std::cout << "angle a (mat): " << std::acos(rota(2,2)) << std::endl;


	ublas::vector<double> vecAxis = make_vec({1., 0., 0.});
	math::quaternion<double> quatb = rotation_quat(vecAxis, 1.23);
	ublas::matrix<double> rotb = quat_to_rot3(quatb);
	std::cout << std::endl;
	std::cout << "quat b: " << quatb << std::endl;
	std::cout << "rot b: " << rotb << std::endl;
	std::cout << "angle b (quat): " << rotation_angle(quatb) << std::endl;
	std::cout << "axis b (quat): " << rotation_axis(quatb) << std::endl;
	std::cout << "angle b (mat): " << std::acos(rotb(2,2)) << std::endl;


	ublas::vector<double> vecAxis2 = make_vec({1., 1., 0.});
	math::quaternion<double> quatc = rotation_quat(vecAxis2, 1.23);
	ublas::matrix<double> matc = rotation_matrix(vecAxis2, 1.23);
	ublas::matrix<double> rotc = quat_to_rot3(quatc);
	std::cout << std::endl;
	std::cout << "quat c: " << quatc << std::endl;
	std::cout << "rot c: " << rotc << std::endl;
	std::cout << "mat c: " << matc << std::endl;
	std::cout << "angle c (quat): " << rotation_angle(quatc) << std::endl;
	std::cout << "axis c (quat): " << rotation_axis(quatc) << std::endl;



	math::quaternion<double> q1(0,1,0,0);
	math::quaternion<double> q2(0,0,1,0);

	std::cout << std::endl;
	std::cout << "slerp 0: " << slerp(q1, q2, 0.) << std::endl;
	std::cout << "slerp 0.5: " << slerp(q1, q2, 0.5) << std::endl;
	std::cout << "slerp 1: " << slerp(q1, q2, 1.) << std::endl;

	std::cout << std::endl;
	std::cout << "lerp 0: " << lerp(q1, q2, 0.) << std::endl;
	std::cout << "lerp 0.5: " << lerp(q1, q2, 0.5) << std::endl;
	std::cout << "lerp 1: " << lerp(q1, q2, 1.) << std::endl;


	std::cout << std::endl;
	math::quaternion<double> quat_s = stereo_proj(quat);
	std::cout << "stereo proj: " << quat_s << std::endl;
	std::cout << "inv stereo proj: " << stereo_proj_inv(quat_s) << std::endl;
	return 0;
}
