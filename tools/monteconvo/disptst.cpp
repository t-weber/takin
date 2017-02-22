/**
 * Testing dispersion plotting
 * @author tweber
 * @date feb-2017
 * @license GPLv2
 */

// gcc -I. -o disptst -std=c++11 tools/monteconvo/disptst.cpp tools/monteconvo/sqw.cpp tools/monteconvo/sqwbase.cpp tlibs/log/log.cpp tlibs/string/eval.cpp tlibs/math/rand.cpp -lstdc++ -lm -lrt -lpthread

#include "sqw.h"
#include "sqw_proc.h"
#include "sqw_proc_impl.h"
#include "tlibs/math/math.h"
#include "tlibs/math/rand.h"
#include <fstream>

using t_real = double;

int main()
{
	tl::init_rand();

	//SqwBase* pSqw = new SqwMagnon("");
	SqwBase* pSqw = new SqwProc<SqwMagnon>("");

	tl::log_info("Setting vars.");
	pSqw->SetVarIfAvail("G", "1 1 0");
	pSqw->SetVarIfAvail("D", "10");


	std::ofstream ofstr("out.dat");

	t_real hkl_0[] = {1.-0.05, 1.+0.05, 0.};
	t_real hkl_1[] = {1.+0.05, 1.-0.05, 0.};
	std::size_t iSteps = 256;

	tl::log_info("Querying dispersion.");
	for(std::size_t iStep=0; iStep<iSteps; ++iStep)
	{
		t_real dFrac = t_real(iStep)/t_real(iSteps-1);
		t_real hkl[] =
		{
			tl::lerp(hkl_0[0], hkl_1[0], dFrac),
			tl::lerp(hkl_0[1], hkl_1[1], dFrac),
			tl::lerp(hkl_0[2], hkl_1[2], dFrac)
		};

		std::vector<t_real> vecE, vecW;
		std::tie(vecE, vecW) = pSqw->disp(hkl[0], hkl[1], hkl[2]);

		ofstr << std::left << std::setw(10) << hkl[0] << " "
			<< std::left << std::setw(10) << hkl[1] << " "
			<< std::left << std::setw(10) << hkl[2] << " ";

		for(std::size_t iE=0; iE<vecE.size(); ++iE)
			ofstr << std::left << std::setw(10) << vecE[iE] << " "
				<< std::left << std::setw(10) << vecW[iE] << " ";
		ofstr << "\n";
	}

	delete pSqw;
	return 0;
}
