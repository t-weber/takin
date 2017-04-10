/**
 * gcc -o fcc_hhl fcc_hhl.cpp -std=c++11 -lstdc++ -lm
 * draws 1st BZ of fcc in hhl plane
 * @date 2016
 * @license GPLv2
 * @author Tobias Weber <tobias.weber@tum.de>
 */

#include <iostream>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/polygon.hpp>
namespace geo = boost::geometry;
namespace trafo = geo::strategy::transform;

using t_real = double;
using t_point = geo::model::point<t_real, 2, geo::cs::cartesian>;
using t_poly = geo::model::polygon<t_point>;

int main()
{
	t_real s2 = std::sqrt(2.);

	t_poly bz;
	bz.outer().push_back(t_point(-0.25*s2,	-1.));
	bz.outer().push_back(t_point(0.25*s2,	-1.));
	bz.outer().push_back(t_point(0.75*s2,	0.));
	bz.outer().push_back(t_point(0.25*s2,	1.));
	bz.outer().push_back(t_point(-0.25*s2,	1.));
	bz.outer().push_back(t_point(-0.75*s2,	0.));
	bz.outer().push_back(t_point(-0.25*s2,	-1.));


	geo::svg_mapper<t_point> svg(std::cout, 1000, 1000);

	svg.add(bz);
	svg.map(bz, "stroke:rgb(0,0,0); fill:rgb(0,0,255)");


	std::vector<t_point> vecPts =
	{
		t_point(0, 0),		// Gamma

		t_point(0.75*s2, 0.),	// K
		t_point(-0.75*s2, 0.),	// K

		t_point(0., 1.),		// X
		t_point(0., -1.),		// X

		t_point(0.5*s2, 0.5),		// L
		t_point(-0.5*s2, 0.5),		// L
		t_point(0.5*s2, -0.5),		// L
		t_point(-0.5*s2, -0.5),		// L
	};

	for(const t_point& pt : vecPts)
	{
		svg.add(pt);
		svg.map(pt, "stroke:rgb(0,0,0); fill:rgb(0,0,0)", 10);
	}

	return 0;
}
