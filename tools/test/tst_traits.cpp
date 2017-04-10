/**
 * @author Tobias Weber <tobias.weber@tum.de>
 * @license GPLv2
 */

#include <iostream>
#include <vector>
#include <map>
#include "tlibs/math/linalg.h"
#include "tlibs/helper/traits.h"

using namespace tl;

int main()
{
	std::cout << "vec: " << get_type_dim<std::vector<double>>::value << std::endl;
	std::cout << "ublas vec: " << get_type_dim<ublas::vector<double>>::value << std::endl;
	std::cout << "ublas mat: " << get_type_dim<ublas::matrix<double>>::value << std::endl;
	std::cout << "map: " << get_type_dim<std::map<int, double>>::value << std::endl;


	std::cout << "vec: " << int(get_linalg_type<ublas::vector<double>>::value) << std::endl;
	std::cout << "mat: " << int(get_linalg_type<ublas::matrix<double>>::value) << std::endl;
	return 0;
}
