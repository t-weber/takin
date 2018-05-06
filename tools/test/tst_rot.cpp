/**
 * tlibs test file
 * @author Tobias Weber <tobias.weber@tum.de>
 * @license GPLv2
 *
 * g++ -o tst_rot tst_rot.cpp -std=c++14
 */

#include <iostream>
#include "../../tlibs/math/math.h"
#include "../../tlibs/math/quat.h"

namespace ublas = tl::ublas;
namespace math = tl::math;

using t_real = double;
using t_vec = ublas::vector<t_real>;
using t_mat = ublas::matrix<t_real>;
using t_quat = math::quaternion<t_real>;

constexpr t_real eps = 0.0001;


int main()
{
	auto vecB = tl::make_vec<t_vec>({1,1,0});
	std::cout << "B = ";
	std::cin >> vecB[0] >> vecB[1] >> vecB[2];

	// scattering vector Q
	auto vecQ = tl::make_vec<t_vec>({1+0.1,1-0.1,0});
	std::cout << "Q = ";
	std::cin >> vecQ[0] >> vecQ[1] >> vecQ[2];

	// lattice vector G
	auto vecG = tl::make_vec<t_vec>({std::round(vecQ[0]), std::round(vecQ[1]), std::round(vecQ[2])});

	// quaternion to rotate B towards [001]
	auto quatB = tl::rotation_quat(vecB, tl::make_vec<t_vec>({0,0,1}));

	// rotate B
	auto vecRotB = tl::quat_vec_prod(quatB, vecB);
	tl::set_eps_0(vecRotB, eps);

	// rotate G
	auto vecRotG = tl::quat_vec_prod(quatB, vecG);
	tl::set_eps_0(vecRotG, eps);

	// rotate Q
	auto vecRotQ = tl::quat_vec_prod(quatB, vecQ);
	tl::set_eps_0(vecRotQ, eps);

	// reduced momentum transfer q
	t_vec vecq = vecRotQ - vecRotG;
	tl::set_eps_0(vecq, eps);

	// q in spherical coordinates
	t_real q, phi_q, theta_q;
	std::tie(q, phi_q, theta_q) = tl::cart_to_sph(vecq[0], vecq[1], vecq[2]);

	// G in spherical coordinates
	t_real G, phi_G, theta_G;
	std::tie(G, phi_G, theta_G) = tl::cart_to_sph(vecRotG[0], vecRotG[1], vecRotG[2]);


	std::cout << "B: " << vecB << " -> " << vecRotB << std::endl;
	std::cout << "G: " << vecG << " -> " << vecRotG << std::endl;
	std::cout << "Q: " << vecQ << " -> " << vecRotQ << std::endl;
	std::cout << "q: " << vecq << std::endl;
	std::cout << "|q| = " << q << ", phi_q = " << tl::r2d(phi_q) << ", theta_q = " << tl::r2d(theta_q) << std::endl;
	std::cout << "|G| = " << G << ", phi_G = " << tl::r2d(phi_G) << ", theta_G = " << tl::r2d(theta_G) << std::endl;

	return 0;
}
