// gcc -o tst_phonon tst_phonon.cpp sqw.cpp ../../tlibs/helper/log.cpp -std=c++11 -I ../.. -lstdc++ -lm

#include "sqw.h"
#include "tlibs/math/linalg.h"
#include <iostream>
#include <fstream>
#include <cstdlib>

typedef ublas::vector<double> t_vec;

int main()
{
	SqwPhonon ph(tl::make_vec({4.,4.,0}),
		tl::make_vec({0.,0.,1.}),
		tl::make_vec({1.,-1.,0.}),
		40., M_PI/2., 0.5, 0.5,
		12., M_PI/2., 0.5, 0.5,
		18., M_PI/2., 0.5, 0.5);

	const t_vec& vecBragg = ph.GetBragg();
	const t_vec& vecLA = ph.GetLA();
	const t_vec& vecTA1 = ph.GetTA1();
	const t_vec& vecTA2 = ph.GetTA2();

	while(1)
	{
		std::cout << "q = ";
		double dq;
		std::cin >> dq;

		std::ofstream ofstrPlot("plt_phonon.gpl");
		ofstrPlot << "set term wxt\n";
		ofstrPlot << "plot \"-\" pt 7\n";

		const t_vec vecQ = vecBragg + vecTA2*dq;
		for(double dE=-20.; dE<20.; dE += 0.25)
		{
			double dS = ph(vecQ[0], vecQ[1], vecQ[2], dE);
			ofstrPlot << dE << " " << dS << "\n";
		}
		ofstrPlot << "e\n";
		ofstrPlot.flush();
		ofstrPlot.close();

		std::system("gnuplot -persist plt_phonon.gpl");
	}
	return 0;
}
