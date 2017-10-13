// gcc -O2 -march=native -DNDEBUG -o sfact sfact.cpp -std=c++11 -lstdc++ -lm -I../.. -I/usr/include/QtGui/ ../../libs/spacegroups/spacegroup.cpp ../../libs/spacegroups/crystalsys.cpp ../../libs/globals.cpp ../../libs/formfactors/formfact.cpp ../../tlibs/log/log.cpp -DNO_QT -lboost_system -lboost_filesystem -lboost_iostreams
/**
 * generates structure factors
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date nov-2015
 * @license GPLv2
 */

#include <vector>
#include <unordered_map>
#include <tuple>
#include <algorithm>
#include <sstream>
#include "tlibs/math/linalg_ops.h"
#include "tlibs/phys/atoms.h"
#include "tlibs/phys/mag.h"
#include "tlibs/phys/lattice.h"
#include "tlibs/phys/neutrons.h"
#include "tlibs/string/string.h"
#include "tlibs/file/prop.h"
#include "libs/spacegroups/spacegroup.h"
#include "libs/formfactors/formfact.h"
#include "libs/globals.h"


using t_real = t_real_glob;
typedef tl::ublas::vector<t_real> t_vec;
typedef tl::ublas::matrix<t_real> t_mat;

const std::string g_strXmlRoot("taz/");



// --------------------------------------------------------------------------------------------
// Lattice
// --------------------------------------------------------------------------------------------
static inline tl::Lattice<t_real> enter_lattice()
{
	std::cout << "\n----------------------------------------------------------------------\n" << std::endl;
	t_real a,b,c, alpha,beta,gamma;
	std::cout << "Enter unit cell lattice constants [a b c (in A)]: ";
	std::cin >> a >> b >> c;
	std::cout << "Enter unit cell angles [alpha beta gamma (in deg)]: ";
	std::cin >> alpha >> beta >> gamma;

	alpha = tl::d2r(alpha);
	beta = tl::d2r(beta);
	gamma = tl::d2r(gamma);

	return tl::Lattice<t_real>(a,b,c, alpha,beta,gamma);
}


static tl::Lattice<t_real> load_lattice(const tl::Prop<>& file)
{
	t_real a = file.Query<t_real>((g_strXmlRoot + "sample/a").c_str(), 0.);
	t_real b = file.Query<t_real>((g_strXmlRoot + "sample/b").c_str(), 0.);
	t_real c = file.Query<t_real>((g_strXmlRoot + "sample/c").c_str(), 0.);
	t_real alpha = file.Query<t_real>((g_strXmlRoot + "sample/alpha").c_str(), 0.);
	t_real beta = file.Query<t_real>((g_strXmlRoot + "sample/beta").c_str(), 0.);
	t_real gamma = file.Query<t_real>((g_strXmlRoot + "sample/gamma").c_str(), 0.);

	alpha = tl::d2r(alpha);
	beta = tl::d2r(beta);
	gamma = tl::d2r(gamma);

	return tl::Lattice<t_real>(a,b,c, alpha,beta,gamma);
}
// --------------------------------------------------------------------------------------------



// --------------------------------------------------------------------------------------------
// Space group
// --------------------------------------------------------------------------------------------
static inline std::string enter_spacegroup()
{
	std::cout << "\n----------------------------------------------------------------------\n" << std::endl;
	std::string strSg;
	std::cout << "Enter spacegroup: ";
	std::cin.ignore();
	std::getline(std::cin, strSg);

	return strSg;
}


static std::string load_spacegroup(const tl::Prop<>& file)
{
	std::string strSG = file.Query<std::string>((g_strXmlRoot + "sample/spacegroup").c_str(), "");
	tl::trim(strSG);

	return strSG;
}
// --------------------------------------------------------------------------------------------




// --------------------------------------------------------------------------------------------
// Atom positions
// --------------------------------------------------------------------------------------------
static inline
std::tuple<std::vector<std::string>, std::vector<t_vec>, std::unordered_map<std::string, t_real>>
enter_atoms()
{
	std::cout << "\n----------------------------------------------------------------------\n" << std::endl;
	int iAtom=0;
	std::vector<std::string> vecElems;
	std::vector<t_vec> vecAtoms;
	std::unordered_map<std::string, t_real> mapMag;

	while(1)
	{
		std::cout << "Enter element name " << (++iAtom) << " name (or <Enter> to finish): ";
		std::string strElem;
		std::getline(std::cin, strElem);
		tl::trim(strElem);
		if(strElem == "")
			break;

		std::cout << "Enter atom position " << (iAtom) << " [x y z (in frac. units)]: ";
		std::string strAtom;
		std::getline(std::cin, strAtom);
		tl::trim(strAtom);
		if(strAtom == "")
			break;

		std::cout << "Enter atom " << (iAtom) << " effective magnetic moment [muB]: ";
		std::string strMag;
		std::getline(std::cin, strMag);
		tl::trim(strMag);
		if(strMag == "")
			strMag = "0";


		vecElems.push_back(strElem);

		t_vec vec(4);
		std::istringstream istrAtom(strAtom);
		istrAtom >> vec[0] >> vec[1] >> vec[2];
		vec[3] = 1.;
		vecAtoms.push_back(vec);

		t_real dMag;
		std::istringstream istrMag(strMag);
		istrMag >> dMag;
		dMag = tl::mag_scatlen_eff(dMag);
		mapMag[strElem] = dMag;
	}

	return std::make_tuple(vecElems, vecAtoms, mapMag);
}


static inline
std::tuple<std::vector<std::string>, std::vector<t_vec>, std::unordered_map<std::string, t_real>>
load_atoms(const tl::Prop<>& file)
{
	std::vector<std::string> vecElems;
	std::vector<t_vec> vecAtoms;
	std::unordered_map<std::string, t_real> mapMag;

	std::size_t iNumAtoms = file.Query<std::size_t>((g_strXmlRoot + "sample/atoms/num").c_str(), 0);
	vecElems.reserve(iNumAtoms);
	vecAtoms.reserve(iNumAtoms);

	for(std::size_t iAtom=0; iAtom<iNumAtoms; ++iAtom)
	{
		std::string strNr = tl::var_to_str(iAtom);

		std::string strAtomName =
			file.Query<std::string>((g_strXmlRoot + "sample/atoms/" + strNr + "/name").c_str(), "");

		t_vec vec(4);
		vec[0] = file.Query<t_real>((g_strXmlRoot + "sample/atoms/" + strNr + "/x").c_str(), 0.);
		vec[1] = file.Query<t_real>((g_strXmlRoot + "sample/atoms/" + strNr + "/y").c_str(), 0.);
		vec[2] = file.Query<t_real>((g_strXmlRoot + "sample/atoms/" + strNr + "/z").c_str(), 0.);
		vec[3] = 1.;

		vecElems.push_back(strAtomName);
		vecAtoms.push_back(vec);
	}

	return std::make_tuple(vecElems, vecAtoms, mapMag);
}
// --------------------------------------------------------------------------------------------



void gen_atoms_sfact(const char *pcFile = nullptr)
{
	bool bHasFile = (pcFile != nullptr);
	tl::Prop<> file;
	if(bHasFile)
	{
		bHasFile = file.Load(pcFile, tl::PropType::XML);
		if(!bHasFile)
			std::cerr << "Error: Invalid file \"" << pcFile << "\"." << std::endl;
	}

	// --------------------------------------------------------------------------------------------
	// Tables
	// --------------------------------------------------------------------------------------------
	std::shared_ptr<const ScatlenList<t_real>> lst = ScatlenList<t_real>::GetInstance();
	std::shared_ptr<const FormfactList<t_real>> lstff = FormfactList<t_real>::GetInstance();
	std::shared_ptr<const MagFormfactList<t_real>> lstmff = MagFormfactList<t_real>::GetInstance();
	std::shared_ptr<const SpaceGroups<t_real>> sgs = SpaceGroups<t_real>::GetInstance();
	// --------------------------------------------------------------------------------------------


	// --------------------------------------------------------------------------------------------
	// Lattice
	// --------------------------------------------------------------------------------------------
	tl::Lattice<t_real> lattice;
	if(bHasFile)
		lattice = load_lattice(file);
	else
		lattice = enter_lattice();

	const t_real dVol = lattice.GetVol();
	const t_mat matA = lattice.GetBaseMatrixCov();
	const t_mat matB = lattice.GetRecip().GetBaseMatrixCov();
	std::cout << "Unit cell volume: " << dVol << " A^3" << std::endl;
	std::cout << "A = " << matA << std::endl;
	std::cout << "B = " << matB << std::endl;
	// --------------------------------------------------------------------------------------------



	// --------------------------------------------------------------------------------------------
	// Space group
	// --------------------------------------------------------------------------------------------
	std::string strSg;
	if(bHasFile)
		strSg = load_spacegroup(file);
	else
		strSg = enter_spacegroup();

	const auto pSg = sgs->Find(strSg);
	if(!pSg)
	{
		std::cerr << "Error: Unknown spacegroup." << std::endl;
		return;
	}
	std::cout << "Spacegroup " << pSg->GetNr() << ": " << pSg->GetName() << "." << std::endl;

	const std::vector<t_mat>& vecTrafos = pSg->GetTrafos();
	std::cout << vecTrafos.size() << " symmetry operations in spacegroup." << std::endl;
	// --------------------------------------------------------------------------------------------



	// --------------------------------------------------------------------------------------------
	// Atom positions
	// --------------------------------------------------------------------------------------------
	int iAtom=0;
	std::vector<std::string> vecElems;
	std::vector<t_vec> vecAtoms;
	std::unordered_map<std::string, t_real> mapMag;

	if(bHasFile)
		std::tie(vecElems, vecAtoms, mapMag) = load_atoms(file);
	else
		std::tie(vecElems, vecAtoms, mapMag) = enter_atoms();
	// --------------------------------------------------------------------------------------------



	// --------------------------------------------------------------------------------------------
	// Unit cell & Scattering lengths (assuming magnetic unit cell is the same as structural one)
	// --------------------------------------------------------------------------------------------
	std::vector<unsigned int> vecNumAtoms;
	std::vector<t_vec> vecAllAtoms;
	std::vector<std::complex<t_real>> vecScatlens;
	std::vector<std::complex<t_real>> vecMagScatlens;
	std::vector<int> vecAtomIndices;

	t_real dsigCoh = 0.;
	t_real dsigInc = 0.;
	t_real dsigScat = 0.;
	t_real dsigAbs = 0.;

	t_real dSigAbs = 0.;
	t_real dSigScat = 0.;

	for(int iAtom=0; iAtom<int(vecAtoms.size()); ++iAtom)
	{
		// generate symmetry-equivalent atom positions
		const t_vec& vecAtom = vecAtoms[iAtom];
		std::vector<t_vec> vecPos = tl::generate_atoms<t_mat, t_vec, std::vector>(vecTrafos, vecAtom);
		vecNumAtoms.push_back(vecPos.size());
		std::cout << "Generated " << vecPos.size() << " " << vecElems[iAtom] << " atoms." << std::endl;
		for(const t_vec& vec : vecPos)
			std::cout << vec << std::endl;


		// get scattering lengths
		const ScatlenList<t_real>::elem_type* pElem = lst->Find(vecElems[iAtom]);
		if(pElem == nullptr)
		{
			std::cerr << "Error: cannot get scattering length for "
				<< vecElems[iAtom] << "." << std::endl;
			return;
		}
		std::complex<t_real> b = pElem->GetCoherent();


		// microscopic cross-sections
		dsigCoh += pElem->GetXSecCoherent().real()*vecNumAtoms[iAtom];
		dsigInc += pElem->GetXSecIncoherent().real()*vecNumAtoms[iAtom];
		dsigScat += pElem->GetXSecScatter().real()*vecNumAtoms[iAtom];
		dsigAbs += pElem->GetXSecAbsorption().real()*vecNumAtoms[iAtom];


		// get magnetic scattering lengths
		std::complex<t_real> p(0.);
		auto iterMag = mapMag.find(vecElems[iAtom]);
		if(iterMag == mapMag.end())
		{
			std::cerr << "Warning: cannot get effective magnetic scattering length for "
				<< vecElems[iAtom] << "." << std::endl;
		}
		else
		{
			p = iterMag->second;
		}


		// macroscopic cross-sections
		dSigScat += tl::macro_xsect(pElem->GetXSecScatter().real()*tl::get_one_femtometer<t_real>()*tl::get_one_femtometer<t_real>(),
			vecNumAtoms[iAtom],
			dVol*tl::get_one_angstrom<t_real>()*tl::get_one_angstrom<t_real>()*tl::get_one_angstrom<t_real>()) * tl::get_one_centimeter<t_real>();

		dSigAbs += tl::macro_xsect(pElem->GetXSecAbsorption().real()*tl::get_one_femtometer<t_real>()*tl::get_one_femtometer<t_real>(),
			vecNumAtoms[iAtom],
			dVol*tl::get_one_angstrom<t_real>()*tl::get_one_angstrom<t_real>()*tl::get_one_angstrom<t_real>()) * tl::get_one_centimeter<t_real>();


		// store calculations
		for(t_vec vecThisAtom : vecPos)
		{
			vecThisAtom.resize(3,1);
			vecAllAtoms.push_back(tl::mult<t_mat, t_vec>(matA, vecThisAtom));
			vecScatlens.push_back(b);
			vecMagScatlens.push_back(p);
			vecAtomIndices.push_back(iAtom);
		}
	}

	std::cout << "\nMicroscopic coherent cross-section: " << dsigCoh << " fm^2." << std::endl;
	std::cout << "Microscopic incoherent cross-section: " << dsigInc << " fm^2." << std::endl;
	std::cout << "Microscopic total scattering cross-section: " << dsigScat << " fm^2." << std::endl;
	std::cout << "Microscopic absorption cross-section: " << dsigAbs << " fm^2." << std::endl;

	const t_real dLam0 = 1.8;	// thermal
	const t_real dLam = 4.5;
	std::cout << "\nMacroscopic total scattering cross-section for lambda = 4.5 A: "
		<< dSigScat*dLam/dLam0 << " / cm." << std::endl;
	std::cout << "Macroscopic absorption cross-section for lambda = 4.5 A: "
		<< dSigAbs*dLam/dLam0 << " / cm." << std::endl;

	//for(const t_vec& vecAt : vecAllAtoms) std::cout << vecAt << std::endl;
	//for(const std::complex<t_real>& cb : vecScatlens) std::cout << cb << std::endl;
	// --------------------------------------------------------------------------------------------



	// --------------------------------------------------------------------------------------------
	// Bragg peaks
	// --------------------------------------------------------------------------------------------
	std::vector<t_real> vecFormfacts;
	std::vector<t_real> vecMagFormfacts;

	while(1)
	{
		std::cout << "\n----------------------------------------------------------------------\n" << std::endl;

		t_real h=0., k=0., l=0.;
		std::cout << "Enter h k l [rlu]: ";
		std::cin >> h >> k >> l;

		t_vec vecG = tl::mult<t_mat, t_vec>(matB, tl::make_vec({h,k,l}));
		t_real dG = ublas::norm_2(vecG);
		std::cout << "G = " << dG << " / A" << std::endl;


		vecFormfacts.clear();
		vecMagFormfacts.clear();
		for(unsigned int iAtom=0; iAtom<vecAllAtoms.size(); ++iAtom)
		{
			//const t_vec& vecAtom = vecAllAtoms[iAtom];
			const FormfactList<t_real>::elem_type* pElemff = lstff->Find(vecElems[vecAtomIndices[iAtom]]);
			const MagFormfactList<t_real>::elem_type* pElemMff = lstmff->Find(vecElems[vecAtomIndices[iAtom]]);

			if(pElemff == nullptr)
			{
				std::cerr << "Error: cannot get form factor for "
					<< vecElems[vecAtomIndices[iAtom]] << "." << std::endl;
				return;
			}
			/*if(pElemMff == nullptr)
			{
				std::cerr << "Warning: cannot get magnetic form factor for "
					<< vecElems[vecAtomIndices[iAtom]] << "." << std::endl;
			}*/

			t_real dFF = pElemff ? pElemff->GetFormfact(dG) : 0.;
			vecFormfacts.push_back(dFF);

			t_real dMFF = pElemMff ? pElemMff->GetFormfact(dG) : 0.;
			vecMagFormfacts.push_back(dMFF);
		}


		std::vector<std::complex<t_real>> vecMag(vecMagScatlens.size());
		if(vecMagScatlens.size() != vecMagFormfacts.size())
		{
			std::cerr << "Size mismatch in magnetic scattering lengths and form factors."
				<< std::endl;
		}
		else
		{
			std::transform(vecMagScatlens.begin(), vecMagScatlens.end(), vecMagFormfacts.begin(),
				vecMag.begin(), [](const std::complex<t_real>& c, t_real d) -> std::complex<t_real>
			{
				// multiply eff. mag. scattering lengths with respective form factors
				return c * d;
			});
		}


		std::complex<t_real> F = tl::structfact<t_real, std::complex<t_real>, ublas::vector<t_real>, std::vector>
			(vecAllAtoms, vecG, vecScatlens);
			std::complex<t_real> Fm = tl::structfact<t_real, std::complex<t_real>, ublas::vector<t_real>, std::vector>
			(vecAllAtoms, vecG, vecMag);
		std::complex<t_real> Fx = tl::structfact<t_real, t_real, ublas::vector<t_real>, std::vector>
			(vecAllAtoms, vecG, vecFormfacts);

		std::cout << std::endl;
		std::cout << "Neutron nuclear structure factor: " << std::endl;
		t_real dFsq = (std::conj(F)*F).real();
		std::cout << "F = " << F << " fm" << std::endl;
		std::cout << "|F| = " << std::sqrt(dFsq) << " fm" << std::endl;
		std::cout << "|F|^2 = " << dFsq << " fm^2" << std::endl;

		std::cout << std::endl;
		std::cout << "Neutron magnetic structure factor: " << std::endl;
		t_real dFmsq = (std::conj(Fm)*Fm).real();
		std::cout << "Fm = " << Fm << " fm" << std::endl;
		std::cout << "|Fm| = " << std::sqrt(dFmsq) << " fm" << std::endl;
		std::cout << "|Fm|^2 = " << dFmsq << " fm^2" << std::endl;

		std::cout << std::endl;
		std::cout << "X-ray atomic structure factor: " << std::endl;
		t_real dFxsq = (std::conj(Fx)*Fx).real();
		std::cout << "Fx = " << Fx << std::endl;
		std::cout << "|Fx| = " << std::sqrt(dFxsq) << std::endl;
		std::cout << "|Fx|^2 = " << dFxsq << std::endl;
	}
	// --------------------------------------------------------------------------------------------
}


#include "libs/version.h"
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

int main(int argc, char** argv)
{
#ifdef NO_TERM_CMDS
	tl::Log::SetUseTermCmds(0);
#endif

	// plain C locale
	/*std::*/setlocale(LC_ALL, "C");
	std::locale::global(std::locale::classic());


	// possible resource paths
	const char* pcProg = argv[0];
	fs::path path(pcProg);
	path.remove_filename();
	tl::log_info("Program path: ", path.string());

	add_resource_path(path.string());
	add_resource_path((path / "..").string());
	add_resource_path((path / "resources").string());
	add_resource_path((path / "Resources").string());
	add_resource_path((path / ".." / "resources").string());
	add_resource_path((path / ".." / "Resources").string());


	// Header
	tl::log_info("This is the Takin structure factor calculator, version " TAKIN_VER ".");
	tl::log_info("Written by Tobias Weber <tobias.weber@tum.de>, 2014-2017.");
	tl::log_info(TAKIN_LICENSE("Takin/Sfact"));


	// load a taz file if given
	const char *pcFile = nullptr;
	if(argc >= 2)
	{
		pcFile = argv[1];
		std::cout << "Using crystal file \"" << pcFile << "\"." << std::endl;
	}

	try
	{
		gen_atoms_sfact(pcFile);
	}
	catch(const std::exception& err)
	{
		std::cerr << err.what() << std::endl;
	}

	return 0;
}
