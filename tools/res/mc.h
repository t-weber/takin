/**
 * monte carlo neutrons
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 2014
 * @license GPLv2
 */

#ifndef __MC_NEUTR_H__
#define __MC_NEUTR_H__


#include <string>
#include <ostream>
#include <cmath>
#include <vector>

#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/matrix.hpp>
namespace ublas = boost::numeric::ublas;

#include "tlibs/math/math.h"
#include "tlibs/math/rand.h"


enum class McNeutronCoords
{
	DIRECT = 0,
	ANGS = 1,
	RLU = 2
};

template<class t_mat = ublas::matrix<double>>
struct McNeutronOpts
{
	using real_type = typename t_mat::value_type;

	McNeutronCoords coords = McNeutronCoords::RLU;
	t_mat matU, matB, matUB;
	t_mat matUinv, matBinv, matUBinv;
	real_type dAngleQVec0;

	bool bCenter;
};



/**
 * Ellipsoid E in Q||... coord. system in 1/A
 *
 * matQVec0: trafo from Q||... to orient1, orient2 system in 1/A
 * Uinv * matQVec0: trafo from Q||... system to lab 1/A system
 * Binv * Uinv * matQVec0: trafo from Q||... system to crystal rlu system
 */
template<class t_vec = ublas::vector<double>, class t_mat = ublas::matrix<double>,
	class t_iter = typename std::vector<t_vec>::iterator>
void mc_neutrons(const Ellipsoid4d<typename t_vec::value_type>& ell4d,
	std::size_t iNum, const McNeutronOpts<t_mat>& opts, t_iter iterResult)
{
	using t_real = typename t_vec::value_type;

	t_vec vecTrans = tl::make_vec<t_vec>({ell4d.x_offs, ell4d.y_offs, ell4d.z_offs, ell4d.w_offs});
	const t_mat& rot = ell4d.rot;
	//if(vecResult.size() != iNum)
	//	vecResult.resize(iNum);

	//tl::log_debug("rot: ", rot);
	//tl::log_debug("Qvec0 = ", opts.dAngleQVec0/M_PI*180.);
	t_mat matQVec0 = tl::rotation_matrix_2d(-opts.dAngleQVec0);
	tl::resize_unity(matQVec0, 4);

	t_mat matUBinvQVec0 = ublas::prod(opts.matUBinv, matQVec0);

	for(std::size_t iCur=0; iCur<iNum; ++iCur)
	{
		t_vec vecMC = tl::make_vec<t_vec, std::vector>(
			tl::rand_norm_nd<t_real, std::vector>
				({0.,0.,0.,0.},
				{ ell4d.x_hwhm*tl::get_HWHM2SIGMA<t_real>(),
				ell4d.y_hwhm*tl::get_HWHM2SIGMA<t_real>(),
				ell4d.z_hwhm*tl::get_HWHM2SIGMA<t_real>(),
				ell4d.w_hwhm*tl::get_HWHM2SIGMA<t_real>() }));

		vecMC = ublas::prod(rot, vecMC);
		if(!opts.bCenter)
			vecMC += vecTrans;

		if(opts.coords == McNeutronCoords::ANGS)
			vecMC = ublas::prod(matQVec0, vecMC);
		else if(opts.coords == McNeutronCoords::RLU)
			vecMC = ublas::prod(matUBinvQVec0, vecMC);

		iterResult[iCur] = std::move(vecMC);
	}
}

#endif
