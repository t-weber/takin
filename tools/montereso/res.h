/**
 * Monte-Carlo resolution calculation
 *
 * @author Tobias Weber
 * @date July 2012, Sep. 2014
 * @license GPLv2
 *
 * (based on Mcstas' 'mcresplot.pl' perl program: www.mcstas.org
 *  and the rescal5 matlab program: http://www.ill.eu/en/instruments-support/computing-for-science/cs-software/all-software/matlab-ill/rescal-for-matlab/)
 */

#ifndef __MONTERES_H__
#define __MONTERES_H__

#include "../res/defs.h"
#include "tlibs/math/linalg.h"
namespace ublas = boost::numeric::ublas;

struct Resolution
{
	// covariance matrix
	ublas::matrix<t_real_reso> cov;

	// resolution matrix
	bool bHasRes;
	ublas::matrix<t_real_reso> res;

	// half-widths
	ublas::vector<t_real_reso> dQ;	// in 1/A and meV

	// ellipse origin
	ublas::vector<t_real_reso> Q_avg;
};


Resolution calc_res(const std::vector<ublas::vector<t_real_reso>>& Q_vec,
	const ublas::vector<t_real_reso>& Q_avg,
	const std::vector<t_real_reso>* pp_vec = 0);

Resolution calc_res(std::size_t uiLen,
	const t_real_reso *_Q_x, const t_real_reso *_Q_y, const t_real_reso *_Q_z,
	const t_real_reso *_E);

Resolution calc_res(std::size_t uiLen,
	const t_real_reso *ki_x, const t_real_reso *ki_y, const t_real_reso *ki_z,
	const t_real_reso *kf_x, const t_real_reso *kf_y, const t_real_reso *kf_z,
	const t_real_reso *p_i=0, const t_real_reso *p_f=0);

#endif
