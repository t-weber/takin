/**
 * Lattice Helpers
 * @author Tobias Weber
 * @date 2015-2016
 * @license GPLv2
 */

#ifndef __LATTICE_HELPER_H__
#define __LATTICE_HELPER_H__

#include "tlibs/math/linalg.h"
#include "tlibs/math/lattice.h"
#include "tlibs/math/geo.h"

#include "spacegroup.h"
#include "../formfactors/formfact.h"

namespace ublas = tl::ublas;


template<class t_real = double>
struct AtomPos
{
	std::string strAtomName;
	ublas::vector<t_real> vecPos;

	t_real J;		// optional: coupling
};


template<class t_real = double>
struct LatticeCommon
{
	using t_mat = ublas::matrix<t_real>;
	using t_vec = ublas::vector<t_real>;

	tl::Lattice<t_real> lattice, recip;
	tl::Plane<t_real> planeRLU, planeFrac;

	const SpaceGroup<t_real>* pSpaceGroup = nullptr;
	const std::vector<AtomPos<t_real>>* pvecAtomPos = nullptr;

	t_mat matPlane, matPlane_inv;
	t_mat matPlaneReal, matPlaneReal_inv;

	std::vector<std::string> vecAllNames;
	std::vector<t_vec> vecAllAtoms, vecAllAtomsFrac;
	std::vector<std::size_t> vecAllAtomTypes;
	std::vector<std::complex<t_real>> vecScatlens;

	t_vec dir0RLU, dir1RLU;
	t_mat matA, matB;

	tl::Plane<t_real> plane, planeReal;


	LatticeCommon() = default;
	~LatticeCommon() = default;

	bool Calc(const tl::Lattice<t_real>& lat, const tl::Lattice<t_real>& rec,
		const tl::Plane<t_real>& plRLU, const tl::Plane<t_real>& plFrac,
		const SpaceGroup<t_real> *pSG, const std::vector<AtomPos<t_real>>* pvecAt)
	{
		lattice = lat;
		recip = rec;
		planeRLU = plRLU;
		planeFrac = plFrac;
		pSpaceGroup = pSG;
		pvecAtomPos = pvecAt;

		{	// reciprocal lattice
			t_vec vecX0 = ublas::zero_vector<t_real>(3);
			t_vec vecPlaneX = planeRLU.GetDir0()[0] * recip.GetVec(0) +
				planeRLU.GetDir0()[1] * recip.GetVec(1) +
				planeRLU.GetDir0()[2] * recip.GetVec(2);
			t_vec vecPlaneY = planeRLU.GetDir1()[0] * recip.GetVec(0) +
				planeRLU.GetDir1()[1] * recip.GetVec(1) +
				planeRLU.GetDir1()[2] * recip.GetVec(2);
			plane = tl::Plane<t_real>(vecX0, vecPlaneX, vecPlaneY);

			std::vector<t_vec> vecOrth =
				tl::gram_schmidt<t_vec>(
					{plane.GetDir0(), plane.GetDir1(), plane.GetNorm()}, 1);
			matPlane = tl::column_matrix(vecOrth);
			t_real dDir0Len = ublas::norm_2(vecOrth[0]), dDir1Len = ublas::norm_2(vecOrth[1]);

			if(tl::float_equal<t_real>(dDir0Len, 0.) || tl::float_equal<t_real>(dDir1Len, 0.)
				|| tl::is_nan_or_inf<t_real>(dDir0Len) || tl::is_nan_or_inf<t_real>(dDir1Len))
			{
				tl::log_err("Invalid scattering plane.");
				return false;
			}

			bool bInv = tl::inverse(matPlane, matPlane_inv);
			if(!bInv)
			{
				tl::log_err("Cannot invert scattering plane metric.");
				return false;
			}
		}

		{	// real lattice
			t_vec vecX0 = ublas::zero_vector<t_real>(3);
			t_vec vecPlaneX = planeFrac.GetDir0()[0] * lattice.GetVec(0) +
				planeFrac.GetDir0()[1] * lattice.GetVec(1) +
				planeFrac.GetDir0()[2] * lattice.GetVec(2);
			t_vec vecPlaneY = planeFrac.GetDir1()[0] * lattice.GetVec(0) +
				planeFrac.GetDir1()[1] * lattice.GetVec(1) +
				planeFrac.GetDir1()[2] * lattice.GetVec(2);
			planeReal = tl::Plane<t_real>(vecX0, vecPlaneX, vecPlaneY);
			
			std::vector<t_vec> vecOrth =
				tl::gram_schmidt<t_vec>(
					{planeReal.GetDir0(), planeReal.GetDir1(), planeReal.GetNorm()}, 1);
			matPlaneReal = tl::column_matrix(vecOrth);
			t_real dDir0Len = ublas::norm_2(vecOrth[0]), dDir1Len = ublas::norm_2(vecOrth[1]);

			if(tl::float_equal<t_real>(dDir0Len, 0.) || tl::float_equal<t_real>(dDir1Len, 0.)
				|| tl::is_nan_or_inf<t_real>(dDir0Len) || tl::is_nan_or_inf<t_real>(dDir1Len))
			{
				tl::log_err("Invalid plane in real lattice.");
				return false;
			}

			bool bInv = tl::inverse(matPlaneReal, matPlaneReal_inv);
			if(!bInv)
			{
				tl::log_err("Cannot invert plane metric in real lattice.");
				return false;
			}
		}


		matA = lattice.GetMetric();
		matB = recip.GetMetric();

		dir0RLU = planeRLU.GetDir0();
		dir1RLU = planeRLU.GetDir1();


		// --------------------------------------------------------------------
		// structure factors
		std::shared_ptr<const ScatlenList<t_real>> lstsl
			= ScatlenList<t_real>::GetInstance();

		const std::vector<t_mat>* pvecSymTrafos = nullptr;
		if(pSpaceGroup)
			pvecSymTrafos = &pSpaceGroup->GetTrafos();

		if(pvecSymTrafos && pvecSymTrafos->size() && /*g_bHasFormfacts &&*/
			g_bHasScatlens && pvecAtomPos && pvecAtomPos->size())
		{
			std::vector<t_vec> vecAtoms;
			std::vector<std::string> vecNames;
			for(std::size_t iAtom=0; iAtom<pvecAtomPos->size(); ++iAtom)
			{
				vecAtoms.push_back((*pvecAtomPos)[iAtom].vecPos);
				vecNames.push_back((*pvecAtomPos)[iAtom].strAtomName);
			}

			const t_real dUCSize = 1.;
			std::tie(vecAllNames, vecAllAtoms, vecAllAtomsFrac, vecAllAtomTypes) =
			tl::generate_all_atoms<t_mat, t_vec, std::vector>
				(*pvecSymTrafos, vecAtoms, &vecNames, matA,
				/*t_real(0), t_real(1)*/ -dUCSize/t_real(2), dUCSize/t_real(2),
				 g_dEps);

			for(std::size_t iAtom=0; iAtom<vecAllAtoms.size(); ++iAtom)
			{
				tl::set_eps_0(vecAllAtoms[iAtom]);
				tl::set_eps_0(vecAllAtomsFrac[iAtom]);
			}

			for(const std::string& strElem : vecAllNames)
			{
				const typename ScatlenList<t_real>::elem_type* pElem = lstsl->Find(strElem);
				vecScatlens.push_back(pElem ? pElem->GetCoherent() : std::complex<t_real>(0.,0.));
				if(!pElem)
					tl::log_err("Element \"", strElem, "\" not found in scattering length table.",
						" Using b=0.");
			}
		}
		// --------------------------------------------------------------------

		return true;
	}


	bool CanCalcStructFact() const { return vecScatlens.size() != 0; }

	std::tuple<std::complex<t_real>, t_real, t_real> GetStructFact(const t_vec& vecPeak) const
	{
		std::complex<t_real> cF =
			tl::structfact<t_real, std::complex<t_real>, t_vec, std::vector>
				(vecAllAtoms, vecPeak, vecScatlens);
		t_real dFsq = (std::conj(cF)*cF).real();
		t_real dF = std::sqrt(dFsq);

		return std::make_tuple(cF, dF, dFsq);
	}
};


#endif
