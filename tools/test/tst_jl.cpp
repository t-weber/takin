/**
 * julia test
 * gcc -I../../ -I/usr/include/julia -o tst_jl.so tst_jl.cpp -lstdc++ -std=c++11 -shared -fPIC
 *
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date 23-apr-2017
 * @license GPLv2
 */

#include <iostream>
#include "tlibs/ext/jl.h"

using namespace tl;


extern "C" void tst_simple_call()
{
	std::cout << "In " << __func__ << "." << std::endl;
}


extern "C" void tst_string(const char* pcStr)
{
	std::cout << "Got: " << pcStr << "." << std::endl;
}


extern "C" void tst_array(jl_array_t *pArr)
{
	std::size_t iArr = jl_array_len(pArr);
	std::cout << "Array size: " << iArr << ", ";

	std::cout << "elements: ";
	for(std::size_t i=0; i<iArr; ++i)
	{
		int64_t iElem = jl_traits<int64_t>::unbox(jl_arrayref(pArr, i));
		std::cout << iElem << ", ";
	}
	std::cout << std::endl;
}


extern "C" jl_array_t* tst_array2()
{
	jl_array_t *pJlArr = jl_alloc_array_1d(jl_apply_array_type(jl_traits<int64_t>::get_type(), 1), 4);
	int64_t* pArr = (int64_t*)jl_array_data(pJlArr);
	pArr[0] = 5;
	pArr[1] = 4;
	pArr[2] = -5;
	pArr[3] = 10;

	return pJlArr;
}
