/**
 * monte carlo neutrons
 * @author tweber
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
template<class t_vec=ublas::vector<double>, class t_mat=ublas::matrix<double>, 
	class t_iter = typename std::vector<t_vec>::iterator>
void mc_neutrons(const Ellipsoid4d<typename t_vec::value_type>& ell4d,
	std::size_t iNum, const McNeutronOpts<t_mat>& opts, t_iter iterResult)
{
	static bool bInited = 0;
	if(!bInited)
	{
		tl::init_rand();
		bInited = 1;
	}

	t_vec vecTrans(4);
	vecTrans[0] = ell4d.x_offs;
	vecTrans[1] = ell4d.y_offs;
	vecTrans[2] = ell4d.z_offs;
	vecTrans[3] = ell4d.w_offs;

	const t_mat& rot = ell4d.rot;
	//if(vecResult.size() != iNum)
	//	vecResult.resize(iNum);

	//tl::log_debug("rot: ", rot);
	//tl::log_debug("Qvec0 = ", opts.dAngleQVec0/M_PI*180.);
	t_mat matQVec0 = tl::rotation_matrix_2d(-opts.dAngleQVec0);
	matQVec0.resize(4,4, true);
	matQVec0(2,2) = matQVec0(3,3) = 1.;
	matQVec0(2,0) = matQVec0(2,1) = matQVec0(2,3) = 0.;
	matQVec0(3,0) = matQVec0(3,1) = matQVec0(3,2) = 0.;
	matQVec0(0,2) = matQVec0(0,3) = 0.;
	matQVec0(1,2) = matQVec0(1,3) = 0.;

	t_mat matUBinvQVec0 = ublas::prod(opts.matUBinv, matQVec0);

	for(std::size_t iCur=0; iCur<iNum; ++iCur)
	{
		t_vec vecMC = tl::rand_norm_nd<t_vec>({0.,0.,0.,0.},
			{ell4d.x_hwhm*tl::HWHM2SIGMA, ell4d.y_hwhm*tl::HWHM2SIGMA,
			ell4d.z_hwhm*tl::HWHM2SIGMA, ell4d.w_hwhm*tl::HWHM2SIGMA});

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
