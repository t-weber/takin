/**
 * @author Tobias Weber <tobias.weber@tum.de>
 * @license GPLv2
 */

// gcc -I../.. -o tst_str tst_str.cpp -std=c++11 -lstdc++

#include <iostream>
#include "tlibs/string/string.h"

int main()
{
	std::string strFile = "/home/tw/test.txt";

	std::cout << tl::get_file_noext(tl::get_file(strFile)) << std::endl;
	return 0;
}
