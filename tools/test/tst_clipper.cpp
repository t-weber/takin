// gcc -o tst_clipper tst_clipper.cpp -lstdc++ -lm -lclipper-core -std=c++11

#include <clipper/clipper.h>
#include "../../tlibs/math/atoms.h"
#include "../../tlibs/helper/array.h"


void list_sgs()
{
	for(int iSg=1; iSg<=230; ++iSg)
	{
		clipper::Spgr_descr dsc(iSg);
		std::cout << "Spacegroup " << iSg << ": " << dsc.symbol_hm() << std::endl;
	}
}

void test_hkls()
{
	std::string strSg;

	std::cout << "Enter spacegroup: ";
	std::getline(std::cin, strSg);
	clipper::Spgr_descr dsc(strSg);
	int iSGNum = dsc.spacegroup_number();
	if(iSGNum <= 0)
	{
		std::cerr << "Error: Unknown Spacegroup." << std::endl;
		return;
	}

	std::cout << "Nr.: " << iSGNum << std::endl;
	std::cout << "Hall: " << dsc.symbol_hall() << std::endl;
	std::cout << "HM: " << dsc.symbol_hm() << std::endl;

	clipper::Spacegroup sg(dsc);
	std::cout << "Laue group: " << sg.symbol_laue() << std::endl;

	int iNumSymOps = sg.num_symops();
	std::cout << "Number of symmetry operations: " << iNumSymOps << std::endl;
	for(int iSymOp=0; iSymOp<iNumSymOps; ++iSymOp)
	{
		const clipper::Symop& symop = sg.symop(iSymOp);
		std::cout << "Symmetry operation " << (iSymOp+1) << ": " << symop.format() << std::endl;
	}

	std::cout << std::endl;
	while(1)
	{
		int h,k,l;
		std::cout << "Enter hkl: ";
		std::cin >> h >> k >> l;
		clipper::HKL_class hkl = sg.hkl_class(clipper::HKL(h,k,l));

		std::cout << "allowed: " << (!hkl.sys_abs()) << std::endl;
		std::cout << "centric: " << hkl.centric() << std::endl;
		std::cout << "allowed phase: " << hkl.allowed() << std::endl;
		std::cout << "epsilon: " << hkl.epsilon() << std::endl;
		std::cout << std::endl;
	}
}


// ----------------------------------------------------------------------------


// ugly, but can't be helped for the moment:
// directly link to the internal clipper coefficients table
// that lives in clipper/clipper/core/atomsf.cpp
namespace clipper { namespace data
{
	extern const struct SFData
	{
		const char atomname[8];
		const double a[5], c, b[5], d;	// d is always 0
	} sfdata[];

	const int numsfdata = 212;
}}


void ffact()
{
	/*for(int i=0; i<clipper::data::numsfdata; ++i)
	{
		std::cout << clipper::data::sfdata[i].atomname << ": c = "
			<< clipper::data::sfdata[i].c << std::endl;
	}*/

	const int iAtom = 1;	// He

	std::string strAtom = clipper::data::sfdata[iAtom].atomname;
	tl::wrapper_array<double> a = tl::wrapper_array<double>((double*)clipper::data::sfdata[iAtom].a, 5);
	tl::wrapper_array<double> b = tl::wrapper_array<double>((double*)clipper::data::sfdata[iAtom].b, 5);
	const double c = clipper::data::sfdata[iAtom].c;

	std::cout << "# " << strAtom << "\n";
	for(double G=0.; G<25.; G+=0.1)
	{
		const double ff = tl::formfact<double, tl::wrapper_array>(G, a, b, c);
		std::cout << G << "\t" << ff << "\n";
	}
	std::cout.flush();
}


// ----------------------------------------------------------------------------


int main()
{
	try
	{
		//list_sgs();
		//test_hkls();

		ffact();
	}
	catch(const clipper::Message_fatal& ex)
	{
		std::cerr << "Fatal error." << std::endl;
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error: " << ex.what() << std::endl;
	}

	return 0;
}
