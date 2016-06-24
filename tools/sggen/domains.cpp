// gcc -o domains domains.cpp -std=c++11 -lstdc++ -lm -I../.. -I/usr/include/QtGui/ ../../helper/spacegroup_clp.cpp ../../helper/crystalsys.cpp -DNO_QT -lclipper-core ../../tlibs/log/log.cpp

/**
 * generates positions based on space group
 * @author tweber
 * @date nov-2015
 * @license GPLv2
 */

#include <clipper/clipper.h>
#include <vector>
#include <sstream>
#include "tlibs/math/linalg.h"
#include "tlibs/math/linalg_ops.h"
#include "tlibs/string/string.h"
#include "libs/spacegroups/spacegroup_clp.h"


typedef tl::ublas::vector<double> t_vec;
typedef tl::ublas::matrix<double> t_mat;

void gen_dirs()
{
	std::string strSg;
	std::cout << "Enter spacegroup: ";
	std::getline(std::cin, strSg);
	tl::trim(strSg);
	clipper::Spgr_descr dsc(strSg);

	const int iSGNum = dsc.spacegroup_number();
	if(iSGNum <= 0)
	{
		std::cerr << "Error: Unknown spacegroup." << std::endl;
		return;
	}
	std::cout << "Spacegroup number: " << iSGNum << std::endl;


	clipper::Spacegroup sg(dsc);
	std::vector<t_mat> vecTrafos;
	get_symtrafos(sg, vecTrafos);
	std::cout << vecTrafos.size() << " symmetry operations." << std::endl;


	t_vec vecDir(4);
	std::cout << "Enter direction: ";
	std::cin >> vecDir[0] >> vecDir[1] >> vecDir[2];
	vecDir[3] = 0.;		// no translations, only point groups


	std::vector<t_vec> vecNewDirs;
	std::cout << "\nall transformations:" << std::endl;
	for(const t_mat& matTrafo : vecTrafos)
	{
		t_vec vecDirNew = ublas::prod(matTrafo, vecDir);
		std::cout << vecDirNew << " (from trafo " << matTrafo << ")" << std::endl;

		bool bHasDir = 0;
		for(const t_vec& vec : vecNewDirs)
			if(tl::vec_equal(vec, vecDirNew))
			{
				bHasDir = 1;
				break;
			}
		if(!bHasDir)
			vecNewDirs.push_back(vecDirNew);
	}

	std::cout << "\nunique transformations:" << std::endl;
	for(const t_vec& vec : vecNewDirs)
	{
		std::cout << vec << std::endl;
	}
}


int main()
{
	try
	{
		gen_dirs();
	}
	catch(const clipper::Message_fatal& err)
	{
		std::cerr << "Error in spacegroup." << std::endl;
	}
	return 0;
}
