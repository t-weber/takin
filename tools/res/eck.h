/*
 * implementation of the eckold-sobolev algo
 *
 * @author tweber
 * @date feb-2015
 * @license GPLv2
 *
 * @desc for algorithm: [eck14] G. Eckold and O. Sobolev, NIM A 752, pp. 54-64 (2014)
 */

#ifndef __TAKIN_ECK_H__
#define __TAKIN_ECK_H__

#include "pop.h"

struct EckParams : public PopParams
{
	tl::t_angle_si<t_real_reso> mono_mosaic_v;
	tl::t_angle_si<t_real_reso> ana_mosaic_v;

	tl::t_length_si<t_real_reso> pos_x, pos_y, pos_z;
};


extern ResoResults calc_eck(const EckParams& eck);


#endif
