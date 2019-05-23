/**
 * type definitions for reso
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date mar-2016
 * @license GPLv2
 */

#ifndef __RESO_DEFS_H__
#define __RESO_DEFS_H__

#include "libs/globals.h"
#include "tlibs/helper/boost_hacks.h"

#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/io.hpp>


namespace ublas = boost::numeric::ublas;

using t_real_reso = ::t_real_glob;


enum ResoFlags : std::size_t
{
	CALC_R0     = 1<<0,
	CALC_RESVOL	= 1<<1,

	CALC_KI3    = 1<<2,
	CALC_KF3    = 1<<3,

	CALC_KFKI   = 1<<4,

	CALC_GENERAL_R0	= 1<<5,
};


struct ResoResults
{
	bool bOk;
	std::string strErr;

	ublas::matrix<t_real_reso> reso;	// quadratic part of quadric
	ublas::vector<t_real_reso> reso_v;	// linear part of quadric
	t_real_reso reso_s;					// constant part of quadric

	ublas::vector<t_real_reso> Q_avg;
	t_real_reso dR0;		// resolution prefactor
	t_real_reso dResVol;	// resolution volume in 1/A^3 * meV

	t_real_reso dBraggFWHMs[4];
};


// all algos
enum class ResoAlgo { CN=1, POP=2, ECK=3, VIOL=4, SIMPLE=100, UNKNOWN=-1 };

#endif
