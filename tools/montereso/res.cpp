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

#include "res.h"

#include "tlibs/phys/neutrons.h"
#include "tlibs/math/stat.h"
#include "tlibs/log/log.h"
#include "../res/helper.h"
#include "../res/ellipse.h"

#include <algorithm>
#include <boost/algorithm/minmax_element.hpp>
#include <fstream>
#include <ios>
#include <limits>
#include <memory>

using namespace ublas;
using t_real = t_real_reso;


/*
 * this function tries to be a 1:1 C++ reimplementation of the Perl function
 * 'read_mcstas_res' of the McStas 'mcresplot' program
 */
Resolution calc_res(std::vector<vector<t_real>>&& Q_vec,
	const vector<t_real>& Q_avg, const std::vector<t_real>* pp_vec)
{
	vector<t_real> Q_dir = tl::make_vec({Q_avg[0], Q_avg[1], Q_avg[2]});
	Q_dir = Q_dir / norm_2(Q_dir);
	vector<t_real> Q_perp = tl::make_vec({-Q_dir[1], Q_dir[0], Q_dir[2]});
	vector<t_real> vecUp = tl::cross_3(Q_dir, Q_perp);

	/*
	 * transformation from the <Q_x, Q_y, Q_z, E> system
	 * into the <Q_avg, Q_perp, Q_z, E> system,
	 * i.e. rotate by the (ki,Q) angle
	 */
	matrix<t_real> trafo = identity_matrix<t_real>(4);
	tl::set_column(trafo, 0, Q_dir);
	tl::set_column(trafo, 1, Q_perp);
	tl::set_column(trafo, 2, vecUp);
	//trafo(3,3) = -1;
	tl::log_info("Transformation: ", trafo);


	Resolution reso;
	reso.Q_avg_notrafo = Q_avg;
	reso.Q_avg = prod(trans(trafo), Q_avg);
	tl::log_info("Transformed average Q vector: ", reso.Q_avg);

	reso.res.resize(4,4,0);
	reso.cov.resize(4,4,0);

	std::tie(reso.cov, std::ignore) = tl::covariance(Q_vec, pp_vec);
	reso.cov = tl::transform<matrix<t_real>>(reso.cov, trafo, true);

	tl::log_info("Covariance matrix: ", reso.cov);
	if(!(reso.bHasRes = tl::inverse(reso.cov, reso.res)))
		tl::log_err("Covariance matrix could not be inverted!");

	if(reso.bHasRes)
	{
		reso.dQ = calc_bragg_fwhms(reso.res);
		reso.dQinc = get_vanadium_fwhms(reso);

		tl::log_info("Resolution matrix: ", reso.res);

		std::ostringstream ostrVals;
		ostrVals << "Coherent / Bragg FWHM values (Qx, Qy, Qz, E): ";
		std::copy(reso.dQ.begin(), reso.dQ.end(),
			std::ostream_iterator<t_real>(ostrVals, ", "));

		std::ostringstream ostrIncVals;
		ostrIncVals << "Incoherent / Vanadium FWHM values (Qy, E): "
			<< std::get<0>(reso.dQinc) << ", " << std::get<1>(reso.dQinc);

		std::ostringstream ostrElli;
		ostrElli << "Ellipsoid offsets: ";
		std::copy(reso.Q_avg.begin(), reso.Q_avg.end(),
			std::ostream_iterator<t_real>(ostrElli, ", "));

		reso.vecQ = std::move(Q_vec);

		tl::log_info(ostrVals.str());
		tl::log_info(ostrIncVals.str());
		tl::log_info(ostrElli.str());
	}

	return reso;
}


Resolution calc_res(std::vector<vector<t_real>>&& vecQ)
{
	vector<t_real> Q_avg = tl::make_vec<vector<t_real>>({0., 0., 0., 0.});

	for(const vector<t_real>& vec : vecQ)
		Q_avg += vec;

	Q_avg /= t_real(vecQ.size());
	tl::log_info("Average Q vector: ", Q_avg);

	return calc_res(std::forward<std::vector<vector<t_real>>&&>(vecQ), Q_avg);
}


/*
 * this function tries to be a 1:1 C++ reimplementation of the Perl function
 * 'read_mcstas_res' of the McStas 'mcresplot' program
 */
Resolution calc_res(const std::vector<ublas::vector<t_real>>& vecKi,
	const std::vector<ublas::vector<t_real>>& vecKf,
	const std::vector<t_real>* _p_i,
	const std::vector<t_real>* _p_f)
{
	tl::log_info("Calculating resolution...");

	std::size_t uiLen = vecKi.size();
	std::vector<vector<t_real>> Q_vec;
	std::vector<t_real> p_vec;
	Q_vec.reserve(uiLen);
	p_vec.reserve(uiLen);

	const t_real pi_max = _p_i ? *std::max_element(_p_i->begin(), _p_i->end()) : 1.;
	const t_real pf_max = _p_f ? *std::max_element(_p_f->begin(), _p_f->end()) : 1.;
	const t_real p_max = fabs(pi_max*pf_max);

	vector<t_real> Q_avg(4);
	Q_avg[0] = Q_avg[1] = Q_avg[2] = Q_avg[3] = 0.;

	t_real p_sum = 0.;
	for(std::size_t uiRow=0; uiRow<uiLen; ++uiRow)
	{
		const vector<t_real>& ki = vecKi[uiRow];
		const vector<t_real>& kf = vecKf[uiRow];

		t_real p = (_p_i && _p_f) ? fabs((*_p_i)[uiRow] * (*_p_f)[uiRow]) : 1.;
		p /= p_max;		// normalize p to 0..1
		p_sum += p;

		vector<t_real> Q = ki - kf;
		t_real Ei = tl::get_KSQ2E<t_real>() * inner_prod(ki, ki);
		t_real Ef = tl::get_KSQ2E<t_real>() * inner_prod(kf, kf);
		t_real dE = Ei - Ef;

		// insert the energy into the Q vector
		Q.resize(4, true);
		Q[3] = dE;

		Q_avg += Q*p;

		Q_vec.push_back(Q);
		p_vec.push_back(p);
	}
	Q_avg /= p_sum;
	tl::log_info("Average Q vector: ", Q_avg);

	return calc_res(std::move(Q_vec), Q_avg, &p_vec);
}


/**
 * vanadium widths
 */
std::tuple<t_real, t_real> get_vanadium_fwhms(const Resolution& reso)
{
	ublas::vector<t_real> vec0 = ublas::zero_vector<t_real>(4);
	return calc_vanadium_fwhms<t_real>(
		reso.res, vec0, t_real(0), reso.Q_avg);
}
