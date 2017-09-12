/**
 * resolution helper functions
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date nov-2016
 * @license GPLv2
 */

#ifndef __TAKIN_RES_AUX_H__
#define __TAKIN_RES_AUX_H__

#include "defs.h"
#include "tlibs/math/math.h"
#include "tlibs/math/linalg.h"
#include <tuple>


/**
 * transforms resolution ellipsoid from <Qpara Qperp Qup> 
 * to crystal hkl coordinate system
 */
template<class t_mat, class t_vec, class t_real = typename t_mat::value_type>
std::tuple<t_mat, t_vec, t_vec> 
conv_lab_to_rlu(t_real dAngleQVec0,
	const t_mat& matUB, const t_mat& matUBinv,
	const t_mat& reso, const t_vec& reso_v, const t_vec& Q_avg)
{
	// hkl crystal system:
	// Qavg system in 1/A -> rotate back to orient system in 1/A ->
	// transform to hkl rlu system
	t_mat matQVec0 = tl::rotation_matrix_2d(-dAngleQVec0);
	tl::resize_unity(matQVec0, 4);
	const t_mat matQVec0inv = ublas::trans(matQVec0);

	const t_mat matUBinvQVec0 = ublas::prod(matUBinv, matQVec0);
	const t_mat matQVec0invUB = ublas::prod(matQVec0inv, matUB);
	t_mat resoHKL = tl::transform(reso, matQVec0invUB, 1);
	t_vec Q_avgHKL = ublas::prod(matUBinvQVec0, Q_avg);

	t_vec reso_vHKL;
	if(reso_v.size() == 4)
		reso_vHKL = ublas::prod(matUBinvQVec0, reso_v);

	return std::make_tuple(resoHKL, reso_vHKL, Q_avgHKL);
}


/**
 * transforms resolution ellipsoid from <Qpara Qperp Qup> 
 * to crystal orientation vector hkl system
 */
template<class t_mat, class t_vec, class t_real = typename t_mat::value_type>
std::tuple<t_mat, t_vec, t_vec> 
conv_lab_to_rlu_orient(t_real dAngleQVec0,
	const t_mat& matUB, const t_mat& matUBinv,
	const t_mat& matUrlu, const t_mat& matUinvrlu,
	const t_mat& reso, const t_vec& reso_v, const t_vec& Q_avg)
{
	// hkl crystal system:
	// Qavg system in 1/A -> rotate back to orient system in 1/A ->
	// transform to hkl rlu system
	t_mat matQVec0 = tl::rotation_matrix_2d(-dAngleQVec0);
	tl::resize_unity(matQVec0, 4);
	const t_mat matQVec0inv = ublas::trans(matQVec0);

	const t_mat matUBinvQVec0 = ublas::prod(matUBinv, matQVec0);
	const t_mat matQVec0invUB = ublas::prod(matQVec0inv, matUB);

	// system of scattering plane: (orient1, orient2, up)
	// Qavg system in 1/A -> rotate back to orient system in 1/A ->
	// transform to hkl rlu system -> rotate forward to orient system in rlu
	const t_mat matToOrient = ublas::prod(matUrlu, matUBinvQVec0);
	const t_mat matToOrientinv = ublas::prod(matQVec0invUB, matUinvrlu);

	t_mat resoOrient = tl::transform(reso, matToOrientinv, 1);
	t_vec Q_avgOrient = ublas::prod(matToOrient, Q_avg);

	t_vec reso_vOrient;
	if(reso_v.size() == 4)
		reso_vOrient = ublas::prod(matToOrient, reso_v);

	return std::make_tuple(resoOrient, reso_vOrient, Q_avgOrient);
}


template<class t_mat, class t_real = typename t_mat::value_type>
std::vector<t_real> calc_bragg_fwhms(const t_mat& reso)
{
	static const t_real sig2fwhm = tl::get_SIGMA2FWHM<t_real>();

	std::vector<t_real> vecFwhms;
	for(std::size_t i=0; i<reso.size1(); ++i)
		vecFwhms.push_back(sig2fwhm/sqrt(reso(i,i)));

	return vecFwhms;
}

#endif
