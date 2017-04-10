/**
 * gcc -o hopfgen hopfgen.cpp -std=c++11 -lstdc++ -lm
 * @author Tobias Weber <tobias.weber@tum.de>
 * @license GPLv2
 */

#include "../../tlibs/math/linalg.h"
#include <iostream>

namespace ublas = boost::numeric::ublas;

int main()
{
	unsigned iMaxOrder = 3;
	double dStartAngle = 30.;
	double dIncAngle = 60.;
	double dScale = 0.028;

	double dStartInt = 0.05;
	double dIntScale = 0.01;

	double dSigQ = 0.002;
	double dSigE = 0.01;

	//ublas::vector<double> vecN = tl::make_vec({1., 0., 0.});
	//ublas::vector<double> vecY = tl::make_vec({0.,-1.,0.});
	ublas::vector<double> vecN = tl::make_vec({1., 1.,0.});
	ublas::vector<double> vecY = tl::make_vec({1.,-1.,0.});
	ublas::vector<double> vecX = vecN;

	vecX /= ublas::norm_2(vecX);
	vecY /= ublas::norm_2(vecY);


	std::cout << "# long. dir.: " << vecX << "\n";
	std::cout << "# trans. dir.: " << vecY << "\n";
	std::cout << "# nuc + long. dir. * scale: " << vecN + vecX*dScale << "\n";
	std::cout << "# nuc + trans. dir. * scale: " << vecN + vecY*dScale << "\n";
	std::cout << "# nuc + long. dir. * 2*scale: " << vecN + vecX*2.*dScale << "\n";
	std::cout << "# nuc + trans. dir. * 2*scale: " << vecN + vecY*2.*dScale << "\n";
	std::cout << "# nuc + long. dir. * 3*scale: " << vecN + vecX*3.*dScale << "\n";
	std::cout << "# nuc + trans. dir. * 3*scale: " << vecN + vecY*3.*dScale << "\n";
	std::cout << "\n";

	std::cout << "# nuclear peak\n";
	std::cout << vecN[0] << " " << vecN[1] << " " << vecN[2]
		<< "    " << dSigQ << " " << dSigE << "  1\n\n";

	for(unsigned iOrder=0; iOrder<iMaxOrder; ++iOrder)
	{
		std::cout << "# hopf peaks, order " << iOrder << "\n";

		ublas::vector<double> vecLast;
		bool bHasLast = 0;
		for(double dAngle=dStartAngle; dAngle<=dStartAngle+360.; dAngle+=dIncAngle)
		{
			ublas::vector<double> vec =
				std::cos(dAngle/180.*M_PI)*vecY +
				std::sin(dAngle/180.*M_PI)*vecX;

			vec *= dScale*double(iOrder+1);
			ublas::vector<double> vecCur = vec;

			vec += vecN;
			double dIntensity = dStartInt * std::pow(dIntScale, iOrder);

			if(dAngle < dStartAngle + 360.)
			{
				std::cout << vec[0] << " " << vec[1] << " " << vec[2]
					<< "    " << dSigQ << " " << dSigE << "  " << dIntensity 
					/*<< " "<< ublas::norm_2(vecCur)*/ << "\n";
			}

			if(bHasLast)
			for(unsigned iOrd=0; iOrd<iOrder; ++iOrd)
			{
				ublas::vector<double> vecBetween =
					vecLast + double(iOrd+1)*(vecCur - vecLast)/double(iOrder+1);
				vecBetween += vecN;

				std::cout << vecBetween[0] << " " << vecBetween[1] << " " << vecBetween[2]
					<< "    " << dSigQ << " " << dSigE << "  " << dIntensity 
					/*<< " " <<  ublas::norm_2(vecBetween-vecN)*/ << "\n";
			}

			vecLast = vecCur;
			bHasLast = 1;
		}

		std::cout << "\n";
	}

	std::cout.flush();
	return 0;
}
