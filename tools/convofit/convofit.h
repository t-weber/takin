/**
 * Convolution fitting
 * @author tweber
 * @date dec-2015
 * @license GPLv2
 */

#ifndef __TAKIN_CONVOFIT_H__
#define __TAKIN_CONVOFIT_H__

#include "scan.h"
#include "model.h"


static inline void set_tasreso_params_from_scan(TASReso& reso, const Scan& sc)
{
	reso.SetLattice(sc.sample.a, sc.sample.b, sc.sample.c,
		sc.sample.alpha, sc.sample.beta, sc.sample.gamma,
		tl::make_vec({sc.plane.vec1[0], sc.plane.vec1[1], sc.plane.vec1[2]}), 
		tl::make_vec({sc.plane.vec2[0], sc.plane.vec2[1], sc.plane.vec2[2]}));
	reso.SetKiFix(sc.bKiFixed);
	reso.SetKFix(sc.dKFix);
}

static inline void set_model_params_from_scan(SqwFuncModel& mod, const Scan& sc)
{
	mod.SetScanOrigin(sc.vecScanOrigin[0], sc.vecScanOrigin[1], 
		sc.vecScanOrigin[2], sc.vecScanOrigin[3]);
	mod.SetScanDir(sc.vecScanDir[0], sc.vecScanDir[1], 
		sc.vecScanDir[2], sc.vecScanDir[3]);

	mod.SetOtherParams(sc.dTemp, sc.dField);
}

#endif
