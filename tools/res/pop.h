/**
 * popovici calculation
 * @author tweber
 * @date 2013-2016
 * @license GPLv2
 *
 * @desc This is a reimplementation in C++ of the file rc_popma.m of the
 *		rescal5 package by Zinkin, McMorrow, Tennant, Farhi, and Wildes:
 *		http://www.ill.eu/en/instruments-support/computing-for-science/cs-software/all-software/matlab-ill/rescal-for-matlab/
 * @desc see: [pop75] M. Popovici, Acta Cryst. A 31, 507 (1975)
 */

#ifndef __TAKIN_POP_H__
#define __TAKIN_POP_H__

#include "cn.h"

struct PopParams : public CNParams
{
	tl::t_length_si<t_real_reso> mono_w;
	tl::t_length_si<t_real_reso> mono_h;
	tl::t_length_si<t_real_reso> mono_thick;
	tl::t_length_si<t_real_reso> mono_curvh;
	tl::t_length_si<t_real_reso> mono_curvv;
	bool bMonoIsCurvedH=0, bMonoIsCurvedV=0;
	bool bMonoIsOptimallyCurvedH=0, bMonoIsOptimallyCurvedV=0;
	unsigned int mono_numtiles_v, mono_numtiles_h;

	tl::t_length_si<t_real_reso> ana_w;
	tl::t_length_si<t_real_reso> ana_h;
	tl::t_length_si<t_real_reso> ana_thick;
	tl::t_length_si<t_real_reso> ana_curvh;
	tl::t_length_si<t_real_reso> ana_curvv;
	bool bAnaIsCurvedH=0, bAnaIsCurvedV=0;
	bool bAnaIsOptimallyCurvedH=0, bAnaIsOptimallyCurvedV=0;
	unsigned int ana_numtiles_v, ana_numtiles_h;

	bool bSampleCub=1;
	tl::t_length_si<t_real_reso> sample_w_q;
	tl::t_length_si<t_real_reso> sample_w_perpq;
	tl::t_length_si<t_real_reso> sample_h;

	bool bSrcRect=1;
	tl::t_length_si<t_real_reso> src_w;
	tl::t_length_si<t_real_reso> src_h;

	bool bDetRect=1;
	tl::t_length_si<t_real_reso> det_w;
	tl::t_length_si<t_real_reso> det_h;

	bool bGuide=0;
	tl::t_angle_si<t_real_reso> guide_div_h;
	tl::t_angle_si<t_real_reso> guide_div_v;

	tl::t_length_si<t_real_reso> dist_mono_sample;
	tl::t_length_si<t_real_reso> dist_sample_ana;
	tl::t_length_si<t_real_reso> dist_ana_det;
	tl::t_length_si<t_real_reso> dist_src_mono;
};


extern ResoResults calc_pop(const PopParams& pop);

#endif
