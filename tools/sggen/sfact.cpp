// gcc -o sfact sfact.cpp -std=c++11 -lstdc++ -lm -I../.. -I/usr/include/QtGui/ ../../libs/spacegroups/spacegroup_clp.cpp ../../libs/spacegroups/crystalsys.cpp ../../libs/globals.cpp ../../libs/formfactors/formfact.cpp ../../tlibs/log/log.cpp -DNO_QT -lclipper-core -lboost_system -lboost_filesystem -lboost_iostreams
/**
 * generates structure factors
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date nov-2015
 * @license GPLv2
 */

#include <clipper/clipper.h>
#include <vector>
#include <sstream>
#include "tlibs/math/linalg_ops.h"
#include "tlibs/phys/atoms.h"
#include "tlibs/phys/lattice.h"
#include "tlibs/phys/neutrons.h"
#include "tlibs/string/string.h"
#include "libs/spacegroups/spacegroup_clp.h"
#include "libs/formfactors/formfact.h"
#include "libs/globals.h"


using t_real = t_real_glob;
typedef tl::ublas::vector<t_real> t_vec;
typedef tl::ublas::matrix<t_real> t_mat;

void gen_atoms_sfact()
{
	std::shared_ptr<const ScatlenList<t_real>> lst = ScatlenList<t_real>::GetInstance();
	std::shared_ptr<const FormfactList<t_real>> lstff = FormfactList<t_real>::GetInstance();


	t_real a,b,c, alpha,beta,gamma;
	std::cout << "Enter unit cell lattice constants: ";
	std::cin >> a >> b >> c;
	std::cout << "Enter unit cell angles: ";
	std::cin >> alpha >> beta >> gamma;

	alpha = tl::d2r(alpha);
	beta = tl::d2r(beta);
	gamma = tl::d2r(gamma);

	const tl::Lattice<t_real> lattice(a,b,c, alpha,beta,gamma);
	const t_real dVol = lattice.GetVol();
	const t_mat matA = lattice.GetBaseMatrixCov();
	const t_mat matB = lattice.GetRecip().GetBaseMatrixCov();
	std::cout << "A = " << matA << std::endl;
	std::cout << "B = " << matB << std::endl;



	std::cout << std::endl;
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



	std::cout << std::endl;
	int iAtom=0;
	std::vector<std::string> vecElems;
	std::vector<t_vec> vecAtoms;
	while(1)
	{
		std::cout << "Enter element " << (++iAtom) << " name (or <Enter> to finish): ";
		std::string strElem;
		std::getline(std::cin, strElem);
		tl::trim(strElem);
		if(strElem == "")
			break;

		std::cout << "Enter atom position " << (iAtom) << ": ";
		std::string strAtom;
		std::getline(std::cin, strAtom);
		tl::trim(strAtom);
		if(strAtom == "")
			break;


		vecElems.push_back(strElem);

		t_vec vec(4);
		std::istringstream istrAtom(strAtom);
		istrAtom >> vec[0] >> vec[1] >> vec[2];
		vec[3] = 1.;
		vecAtoms.push_back(vec);
	}



	clipper::Spacegroup sg(dsc);
	std::vector<t_mat> vecTrafos;
	get_symtrafos(sg, vecTrafos);
	std::cout << vecTrafos.size() << " symmetry operations in spacegroup." << std::endl;

	std::vector<unsigned int> vecNumAtoms;
	std::vector<t_vec> vecAllAtoms;
	std::vector<std::complex<t_real>> vecScatlens;
	std::vector<t_real> vecFormfacts;
	std::vector<int> vecAtomIndices;

	t_real dSigAbs = 0.;

	for(int iAtom=0; iAtom<int(vecAtoms.size()); ++iAtom)
	{
		const t_vec& vecAtom = vecAtoms[iAtom];
		std::vector<t_vec> vecPos = tl::generate_atoms<t_mat, t_vec, std::vector>(vecTrafos, vecAtom);
		vecNumAtoms.push_back(vecPos.size());
		std::cout << "Generated " << vecPos.size() << " " << vecElems[iAtom] << " atoms." << std::endl;
		for(const t_vec& vec : vecPos)
			std::cout << vec << std::endl;

		const ScatlenList<t_real>::elem_type* pElem = lst->Find(vecElems[iAtom]);

		if(pElem == nullptr)
		{
			std::cerr << "Error: cannot get scattering length for "
				<< vecElems[iAtom] << "." << std::endl;
			return;
		}

		std::complex<t_real> b = pElem->GetCoherent() /*/ 10.*/;

		dSigAbs += tl::macro_xsect(pElem->GetXSecCoherent().real()*tl::get_one_barn<t_real>(), 
			vecNumAtoms[iAtom],
			dVol*tl::get_one_angstrom<t_real>()*tl::get_one_angstrom<t_real>()*tl::get_one_angstrom<t_real>()) * tl::get_one_centimeter<t_real>();
		//dSigAbs += pElem->GetXSecCoherent().real()*1e-24 * vecNumAtoms[iAtom] / (dVol*1e-24);

		for(t_vec vecThisAtom : vecPos)
		{
			vecThisAtom.resize(3,1);
			vecAllAtoms.push_back(tl::mult<t_mat, t_vec>(matA, vecThisAtom));
			vecScatlens.push_back(b);
			vecAtomIndices.push_back(iAtom);
		}
	}

	const t_real dLam0 = 1.8;	// thermal
	const t_real dLam = 4.5;
	std::cout << "\nMacroscopic absorption cross-section for lambda = 4.5 A: "
		<< dSigAbs*dLam/dLam0 << " / cm." << std::endl;

	//for(const t_vec& vecAt : vecAllAtoms) std::cout << vecAt << std::endl;
	//for(const std::complex<t_real>& cb : vecScatlens) std::cout << cb << std::endl;


	while(1)
	{
		std::cout << std::endl;

		t_real h=0., k=0., l=0.;
		std::cout << "Enter hkl: ";
		std::cin >> h >> k >> l;

		t_vec vecG = tl::mult<t_mat, t_vec>(matB, tl::make_vec({h,k,l}));
		t_real dG = ublas::norm_2(vecG);
		std::cout << "G = " << dG << " / A" << std::endl;


		vecFormfacts.clear();
		for(unsigned int iAtom=0; iAtom<vecAllAtoms.size(); ++iAtom)
		{
			//const t_vec& vecAtom = vecAllAtoms[iAtom];
			const FormfactList<t_real>::elem_type* pElemff = lstff->Find(vecElems[vecAtomIndices[iAtom]]);

			if(pElemff == nullptr)
			{
				std::cerr << "Error: cannot get form factor for "
					<< vecElems[iAtom] << "." << std::endl;
				return;
			}

			t_real dFF = pElemff->GetFormfact(dG);
			vecFormfacts.push_back(dFF);
		}


		std::complex<t_real> F = tl::structfact<t_real, std::complex<t_real>, ublas::vector<t_real>, std::vector>
			(vecAllAtoms, vecG, vecScatlens);
		std::complex<t_real> Fx = tl::structfact<t_real, t_real, ublas::vector<t_real>, std::vector>
			(vecAllAtoms, vecG, vecFormfacts);


		std::cout << std::endl;
		std::cout << "Neutron structure factor: " << std::endl;
		t_real dFsq = (std::conj(F)*F).real();
		std::cout << "F = " << F << std::endl;
		std::cout << "|F| = " << std::sqrt(dFsq) << std::endl;
		std::cout << "|F|^2 = " << dFsq << std::endl;

		std::cout << std::endl;
		std::cout << "X-ray structure factor: " << std::endl;
		t_real dFxsq = (std::conj(Fx)*Fx).real();
		std::cout << "Fx = " << Fx << std::endl;
		std::cout << "|Fx| = " << std::sqrt(dFxsq) << std::endl;
		std::cout << "|Fx|^2 = " << dFxsq << std::endl;
	}
}


int main()
{
	try
	{
		gen_atoms_sfact();
	}
	catch(const clipper::Message_fatal& err)
	{
		std::cerr << "Error in spacegroup." << std::endl;
	}
	return 0;
}
