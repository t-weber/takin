/**
 * Monte-Carlo resolution calculation
 *
 * @author Tobias Weber <tobias.weber@tum.de>
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
#include <utility>
namespace ublas = boost::numeric::ublas;


struct Resolution
{
	// covariance matrix
	ublas::matrix<t_real_reso> cov;

	// resolution matrix
	bool bHasRes = 0;
	ublas::matrix<t_real_reso> res;

	// full-widths (coh)
	std::vector<t_real_reso> dQ;	// in 1/A and meV

	// full-widths (inc)
	std::tuple<t_real_reso, t_real_reso> dQinc;

	// ellipse origin
	ublas::vector<t_real_reso> Q_avg, Q_avg_notrafo;

	// all MC events
	std::vector<ublas::vector<t_real_reso>> vecQ;
};

std::tuple<t_real_reso, t_real_reso> get_vanadium_fwhms(const Resolution& reso);

Resolution calc_res(std::vector<ublas::vector<t_real_reso>>&& Q_vec,
	const ublas::vector<t_real_reso>& Q_avg,
	const std::vector<t_real_reso>* pp_vec = 0);

Resolution calc_res(std::vector<ublas::vector<t_real_reso>>&&);

Resolution calc_res(const std::vector<ublas::vector<t_real_reso>>& vecKi,
	const std::vector<ublas::vector<t_real_reso>>& vecKf,
	const std::vector<t_real_reso>* p_i = nullptr,
	const std::vector<t_real_reso>* p_f = nullptr);

#endif
