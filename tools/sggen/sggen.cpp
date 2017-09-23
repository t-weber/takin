/**
 * generates atom positions based on space group
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date nov-2015
 * @license GPLv2
 */

// gcc -o sggen sggen.cpp -std=c++11 -lstdc++ -lm -I../.. -I/usr/include/QtGui/ ../../libs/spacegroups/spacegroup_clp.cpp ../../libs/spacegroups/crystalsys.cpp -DNO_QT -lclipper-core ../../tlibs/file/x3d.cpp ../../tlibs/log/log.cpp

#include <clipper/clipper.h>
#include <vector>
#include <sstream>
#include "tlibs/phys/atoms.h"
#include "tlibs/phys/lattice.h"
#include "tlibs/math/linalg.h"
#include "tlibs/math/linalg_ops.h"
#include "tlibs/file/x3d.h"
#include "tlibs/string/string.h"
#include "libs/spacegroups/spacegroup_clp.h"


typedef double t_real;
typedef tl::ublas::vector<t_real> t_vec;
typedef tl::ublas::matrix<t_real> t_mat;

void gen_atoms()
{
	const std::vector<t_vec> vecColors =
	{
		tl::make_vec({1., 0., 0.}),
		tl::make_vec({0., 1., 0.}),
		tl::make_vec({0., 0., 1.}),
		tl::make_vec({1., 1., 0.}),
		tl::make_vec({0., 1., 1.}),
		tl::make_vec({1., 0., 1.}),
	};

	// to transform into program-specific coordinate systems
	const t_mat matGlobal = tl::make_mat(
	{	{-1., 0., 0., 0.},
		{ 0., 0., 1., 0.},
		{ 0., 1., 0., 0.},
		{ 0., 0., 0., 1.}	});


	t_real a,b,c, alpha,beta,gamma;
	std::cout << "Enter unit cell lattice constants: ";
	std::cin >> a >> b >> c;
	std::cout << "Enter unit cell angles: ";
	std::cin >> alpha >> beta >> gamma;

	alpha = tl::d2r(alpha);
	beta = tl::d2r(beta);
	gamma = tl::d2r(gamma);

	const tl::Lattice<t_real> lattice(a,b,c, alpha,beta,gamma);
	const t_mat matA = lattice.GetMetric();


	std::string strSg;

	std::cout << "Enter spacegroup: ";
	std::cin.ignore();
	std::getline(std::cin, strSg);
	clipper::Spgr_descr dsc(strSg);
	const int iSGNum = dsc.spacegroup_number();
	if(iSGNum <= 0)
	{
		std::cerr << "Error: Unknown spacegroup." << std::endl;
		return;
	}
	std::cout << "Spacegroup number: " << iSGNum << std::endl;



	int iAtom=0;
	std::vector<t_vec> vecAtoms;
	while(1)
	{
		std::cout << "Enter atom position " << (++iAtom) << ": ";
		std::string strAtom;
		std::getline(std::cin, strAtom);
		tl::trim(strAtom);
		if(strAtom == "")
			break;

		t_vec vec(4);
		std::istringstream istrAtom(strAtom);
		istrAtom >> vec[0] >> vec[1] >> vec[2];
		vec[3] = 1.;
		vecAtoms.push_back(vec);
	}



	clipper::Spacegroup sg(dsc);
	std::vector<t_mat> vecTrafos;
	get_symtrafos(sg, vecTrafos);



	tl::X3d<t_real> x3d;

	std::cout << std::endl;
	//const t_vec vecOffs = tl::make_vec({0.5, 0.5, 0.5, 0.});
	for(int iAtom=0; iAtom<vecAtoms.size(); ++iAtom)
	{
		const t_vec& vecAtom = vecAtoms[iAtom];
		std::vector<t_vec> vecPos = tl::generate_atoms<t_mat, t_vec, std::vector>(vecTrafos, vecAtom);

		std::cout << "Atom " << (iAtom+1) << ":\n";
		for(int iPos=0; iPos<vecPos.size(); ++iPos)
		{
			const t_vec& vec = vecPos[iPos] /*+ vecOffs*/;

			//std::cout << "\t(" << (iPos+1) << ") " << vec << "\n";

			// map back to unit cell
			t_vec vecCoord = vec;
			const t_real dUCSize = 1.;
			for(int iComp=0; iComp<vecCoord.size(); ++iComp)
			{
				while(vecCoord[iComp] > dUCSize*0.5)
					vecCoord[iComp] -= dUCSize;
				while(vecCoord[iComp] < -dUCSize*0.5)
					vecCoord[iComp] += dUCSize;
			}
			std::cout << "\t(" << (iPos+1) << ") " << vecCoord << "\n";

			vecCoord.resize(3,1);
			vecCoord = matA * vecCoord;
			vecCoord.resize(4,1); vecCoord[3] = 1.;

			tl::X3dTrafo<t_real> *pTrafo = new tl::X3dTrafo<t_real>();
			pTrafo->SetTrans(matGlobal * vecCoord);
			tl::X3dSphere<t_real> *pSphere = new tl::X3dSphere<t_real>(0.1);
			pSphere->SetColor(vecColors[iAtom % vecColors.size()]);
			pTrafo->AddChild(pSphere);

			x3d.GetScene().AddChild(pTrafo);
		}
		std::cout << std::endl;
	}


	// test: only for cubic unit cells!
	tl::X3dCube<t_real> *pCube = new tl::X3dCube<t_real>(a,b,c);
	pCube->SetColor(tl::make_vec({1., 1., 1., 0.75}));
	x3d.GetScene().AddChild(pCube);

	if(x3d.Save("out.x3d"))
		std::cout << "Wrote out.x3d." << std::endl;
}


int main()
{
	try
	{
		gen_atoms();
	}
	catch(const clipper::Message_fatal& err)
	{
		std::cerr << "Error in spacegroup." << std::endl;
	}
	return 0;
}
