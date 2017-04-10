/**
 * @author Tobias Weber <tobias.weber@tum.de>
 * @license GPLv2
 */

// gcc -DUSE_LAPACK -o tst_geo tst_geo.cpp -std=c++11 -lstdc++ -lm -I../.. -I. -I/usr/include/lapacke ../../tlibs/log/log.cpp ../../tlibs/math/linalg2.cpp -llapacke -llapack

#include "tlibs/math/geo.h"
#include "tlibs/math/math.h"
#include "tlibs/math/linalg2.h"

using namespace tl;

typedef ublas::vector<double> vec;


void tst0()
{
	vec v0(3), dir0_0(3), dir0_1(3);
	vec v1(3), dir1_0(3), dir1_1(3);

	v0[0] = 0.; v0[1] = 0.; v0[2] = 0.;
	v1[0] = 3.; v1[1] = 0.; v1[2] = 0.;

	dir0_0[0] = 1.; dir0_0[1] = 0.; dir0_0[2] = 0.;
	dir0_1[0] = 0.; dir0_1[1] = 1.; dir0_1[2] = 0.;

	dir1_0[0] = 0.; dir1_0[1] = 1.; dir1_0[2] = 0.;
	dir1_1[0] = 0.; dir1_1[1] = 0.; dir1_1[2] = 1.;

	Plane<double> plane0(v0, dir0_0, dir0_1);
	Plane<double> plane1(v1, dir1_0, dir1_1);

	std::cout << "Plane 0: " << plane0 << std::endl;
	std::cout << "Plane 1: " << plane1 << std::endl;

	Line<double> line;
	if(plane0.intersect(plane1, line))
		std::cout << "Line: " << line << std::endl;
	else
		std::cout << "Error." << std::endl;
}

void tst1()
{
	ublas::vector<double> vecPos(2), vecDir(2);
	vecPos[0] = 0.;
	vecPos[1] = 0.;

	vecDir[0] = 1.;
	vecDir[1] = 1.;


	Line<double> line(vecPos, vecDir);
	ublas::vector<double> v0(2), v1(2);
	v0[0] = -1.;
	v0[1] = 1.;

	v1[0] = 2.;
	v1[1] = 1.;

	double dDist0, dDist1;
	std::cout << "side: " << line.GetSide(v0, &dDist0);
	std::cout << ", dist: " << dDist0 << std::endl;
	std::cout << "side: " << line.GetSide(v1, &dDist1);
	std::cout << ", dist: " << dDist1 << std::endl;
}

int main()
{
	//tst0();
	tst1();

	return 0;
}
