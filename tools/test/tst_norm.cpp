/**
 * test normalisation of random normal distribution
 * @author Tobias Weber <tobias.weber@tum.de>
 * @data may-2019
 * @license GPLv2
 *
 * g++ -I../../ -o tst_norm tst_norm.cpp ../../tlibs/math/rand.cpp ../../tlibs/log/log.cpp
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <boost/histogram.hpp>
#include "tlibs/math/linalg.h"
#include "tlibs/math/rand.h"

using t_real = double;

int main()
{
	constexpr const std::size_t ITERS = 100000;
	constexpr const std::size_t BINS = 64;

	t_real sigma = 0.25;
	t_real mu = 0.2;

	auto histo_axis = std::vector<boost::histogram::axis::regular<t_real>>
	{
		{ BINS, mu-4.*sigma /*min*/, mu+4.*sigma /*max*/ }
	};
	auto histo = boost::histogram::make_histogram(histo_axis);

	for(std::size_t i=0; i<ITERS; ++i)
	{
		t_real val = tl::rand_norm<t_real>(mu, sigma);
		histo(val);
	}

	std::ostream& ostr = std::cout;
	ostr.precision(5);
	for(const auto& val : boost::histogram::indexed(histo))
	{
		t_real x = val.bin().lower() + 0.5*(val.bin().upper() - val.bin().lower());
		t_real yMC = *val/t_real{ITERS}*t_real{BINS}*0.5;
		t_real yModel = tl::gauss_model<t_real>(x, mu, sigma, 1., 0.);

		ostr << std::left << std::setw(12) << x << " " <<
			std::left << std::setw(12) << yMC << " " <<
			std::left << std::setw(12) << yModel << " " <<
			std::left << std::setw(12) << yModel/yMC << "\n";
	}

	return 0;
}
