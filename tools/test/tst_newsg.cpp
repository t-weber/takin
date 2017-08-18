/**
 * @author Tobias Weber <tobias.weber@tum.de>
 * @license GPLv2
 */

// gcc -DNO_QT -I../.. -o tst_newsg tst_newsg.cpp ../../libs/spacegroups/spacegroup.cpp ../../libs/spacegroups/crystalsys.cpp ../../libs/globals.cpp ../../tlibs/log/log.cpp -lstdc++ -std=c++11 -lboost_iostreams -lboost_filesystem -lboost_system -lm


#include <iostream>
#define _SGR_NO_SINGLETON
#include "libs/spacegroups/spacegroup.h"

using t_real = double;
t_real g_eps = 0.001;

#define RED "\033[1;31m"
#define GRN "\033[1;32m"
#define CLR "\033[0m"


void chk_sgs()
{
	SpaceGroups<t_real> sgs1("../../res/data/sgroups.xml");
	SpaceGroups<t_real> sgs2("../../res/data/sgroups_old.xml");

	const auto* pMapSGs1 = sgs1.get_space_groups();

	for(const auto& _sg : *sgs2.get_space_groups())
	{
		auto& sg2 = _sg.second;
		std::cout << "(" << sg2.GetNr() << ") " << sg2.GetName() << ": ";
		std::vector<ublas::matrix<double>> vecTrafos = sg2.GetTrafos();
		std::cout << vecTrafos.size() << " trafos: ";

		auto iter = pMapSGs1->find(sg2.GetName());
		if(iter == pMapSGs1->end())
		{
			std::cerr << RED "NOT FOUND" CLR << std::endl;
			continue;
		}

		auto& sg1 = iter->second;

		if(sg1.GetTrafos().size() != sg2.GetTrafos().size())
		{
			std::cerr << RED "TRAFO SIZE MISMATCH." CLR << std::endl;
			continue;
		}

		if(sg1.GetCenterTrafos().size() != sg2.GetCenterTrafos().size())
		{
			std::cerr << RED "CENTERING TRAFO SIZE MISMATCH." CLR << std::endl;
			continue;
		}

		bool bMismatch = 0;
		for(const auto& trafo2 : sg2.GetTrafos())
		{
			bool bFound = 0;

			for(const auto& trafo1 : sg1.GetTrafos())
			{
				if(tl::mat_equal(trafo1, trafo2, g_eps))
				{
					bFound = 1;
					break;
				}
			}

			if(!bFound)
			{
				bMismatch = 1;
				std::cerr << RED "TRAFO MISMATCH." CLR << std::endl;
				break;
			}
		}

		if(bMismatch)
			continue;


		for(unsigned int iIdx2 : sg2.GetCenterTrafos())
		{
			const auto& trafo2 = sg2.GetTrafos()[iIdx2];
			bool bFound = 0;

			for(unsigned int iIdx1 : sg1.GetCenterTrafos())
			{
				const auto& trafo1 = sg1.GetTrafos()[iIdx1];

				if(tl::mat_equal(trafo1, trafo2, g_eps))
				{
					bFound = 1;
					break;
				}
			}

			if(!bFound)
			{
				bMismatch = 1;
				std::cerr << RED "CENTERING TRAFO MISMATCH." CLR << std::endl;
				break;
			}
		}

		if(bMismatch)
			continue;


		std::cout << GRN "OK." CLR << std::endl;
	}
}


int main()
{
	chk_sgs();
	return 0;
}
