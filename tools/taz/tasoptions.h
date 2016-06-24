/*
 * Scattering Triangle tool
 * @author tweber
 * @date feb-2014
 * @license GPLv2
 */

#ifndef __TASOPTIONS_H__
#define __TASOPTIONS_H__

#include "tlibs/math/linalg.h"
#include "libs/globals.h"
#include <QPointF>

namespace ublas = boost::numeric::ublas;


static inline ublas::vector<t_real_glob> qpoint_to_vec(const QPointF& pt)
{
	ublas::vector<t_real_glob> vec(2);
	vec[0] = t_real_glob(pt.x());
	vec[1] = t_real_glob(pt.y());

	return vec;
}

static inline QPointF vec_to_qpoint(const ublas::vector<t_real_glob>& vec)
{
	if(vec.size() < 2)
		return QPointF(0., 0.);

	return QPointF(vec[0], vec[1]);
}

struct TriangleOptions
{
	bool bChangedTheta = 0;
	bool bChangedTwoTheta = 0;
	bool bChangedAnaTwoTheta = 0;
	bool bChangedMonoTwoTheta = 0;

	bool bChangedMonoD = 0;
	bool bChangedAnaD = 0;

	bool bChangedAngleKiVec0 = 0;


	t_real_glob dTheta;
	t_real_glob dTwoTheta;
	t_real_glob dAnaTwoTheta;
	t_real_glob dMonoTwoTheta;

	t_real_glob dMonoD;
	t_real_glob dAnaD;

	t_real_glob dAngleKiVec0;
};

struct CrystalOptions
{
	bool bChangedLattice = 0;
	bool bChangedLatticeAngles = 0;
	bool bChangedSpacegroup = 0;
	bool bChangedPlane1 = 0;
	bool bChangedPlane2 = 0;

	t_real_glob dLattice[3];
	t_real_glob dLatticeAngles[3];
	std::string strSpacegroup;
	t_real_glob dPlane1[3];
	t_real_glob dPlane2[3];

	std::string strSampleName;
};


#endif
