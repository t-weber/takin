/**
 * @author Tobias Weber <tobias.weber@tum.de>
 * @license GPLv2
 */

// gcc -I../.. -I/usr/include/lapacke -o tst_quad tst_quad.cpp ../../tlibs/math/linalg2.cpp ../../tlibs/log/log.cpp -lstdc++ -lm -llapacke -llapack -std=c++11

#include "tlibs/math/linalg.h"
#include "tlibs/math/geo.h"
#include <iostream>

using namespace tl;

int main()
{
	QuadEllipsoid<double> elli(1.,2.,3.,4.);
	std::cout << "quad: " << elli << std::endl;

	Quadric<double> q2 = elli;
	q2.RemoveElems(1);
	std::cout << "removed 1: " << q2 << std::endl;

	ublas::matrix<double> matRot = rotation_matrix_3d_y(0.5);
	matRot.resize(4,4,1);
	matRot(3,0)=matRot(3,1)=matRot(3,2)=matRot(0,3)=matRot(1,3)=matRot(2,3)=0.;
	matRot(3,3)=1.;
	std::cout << "trafo: " << matRot << std::endl;

	elli.transform(matRot);
	std::cout << "quad: " << elli << std::endl;


	ublas::matrix<double> matEvecs;
	std::vector<double> vecEvals;
	elli.GetPrincipalAxes(matEvecs, vecEvals);
	ublas::matrix<double> matEvals = diag_matrix(vecEvals);

	std::cout << "evecs: " << matEvecs << std::endl;
	std::cout << "evals: " << matEvals << std::endl;


	ublas::matrix<double> mat0 = ublas::prod(matEvals, ublas::trans(matEvecs));
	std::cout << "reconstr. quad: " << ublas::prod(matEvecs, mat0) << std::endl;

	return 0;
}
