/**
 * testing reflectivity curve
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date oct-2017
 * @license GPLv2
 *
 * gcc -std=c++14 -I../../ -o refl_tst refl_curve.cpp ../../tlibs/log/log.cpp  -lstdc++ -lboost_iostreams
 */

#include "refl_curve.h"
#include <iostream>
#include <fstream>

int main(int argc, char** argv)
{
	if(argc < 2)
	{
		std::cerr << "No reflectivity file given." << std::endl;
		return -1;
	}

	ReflCurve<> curv(argv[1]);
	if(!curv)
	{
		std::cerr << "Could not load file." << std::endl;
		return -1;
	}


	std::ofstream ofstr("/tmp/tst_refl.dat");
	for(double d=0; d<4.; d+=0.05)
		ofstr << d << " " << curv(d) << "\n";

	return 0;
}
